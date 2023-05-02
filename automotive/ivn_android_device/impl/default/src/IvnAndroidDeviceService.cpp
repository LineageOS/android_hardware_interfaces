/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "IvnAndroidDeviceService.h"

#include <aidl/android/hardware/automotive/ivn/ConnectProtocol.h>
#include <aidl/android/hardware/automotive/ivn/HardwareIdentifiers.h>
#include <aidl/android/hardware/automotive/ivn/OccupantType.h>
#include <android-base/logging.h>
#include <android/binder_status.h>
#include <json/json.h>

#include <fstream>

namespace android {
namespace hardware {
namespace automotive {
namespace ivn {

namespace {

using ::aidl::android::hardware::automotive::ivn::ConnectProtocol;
using ::aidl::android::hardware::automotive::ivn::EndpointInfo;
using ::aidl::android::hardware::automotive::ivn::HardwareIdentifiers;
using ::aidl::android::hardware::automotive::ivn::OccupantType;
using ::aidl::android::hardware::automotive::ivn::OccupantZoneInfo;
using ::ndk::ScopedAStatus;

constexpr int IVN_ERROR_GENERIC = -1;

}  // namespace

IvnAndroidDeviceService::IvnAndroidDeviceService(std::string_view configPath) {
    mConfigPath = configPath;
}

bool IvnAndroidDeviceService::init() {
    std::ifstream configStream(mConfigPath);
    if (!configStream) {
        LOG(ERROR) << "couldn't open " << mConfigPath << " for parsing.";
        return false;
    }
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, configStream, &mConfigRootNode, &errs)) {
        LOG(ERROR) << "Failed to parse config JSON stream, error: " << errs;
        return false;
    }
    if (!mConfigRootNode.isObject()) {
        LOG(ERROR) << "Root must be an object";
        return false;
    }
    if (!mConfigRootNode.isMember("MyDeviceId")) {
        LOG(ERROR) << "Must contain 'MyDeviceId' field";
        return false;
    }
    mMyDeviceId = mConfigRootNode["MyDeviceId"].asInt();
    if (!mConfigRootNode.isMember("Devices") || !mConfigRootNode["Devices"].isArray()) {
        LOG(ERROR) << "Must contain 'Devices' field as array";
        return false;
    }
    Json::Value& devices = mConfigRootNode["Devices"];
    for (unsigned int i = 0; i < devices.size(); i++) {
        Json::Value& device = devices[i];
        int deviceId = device["DeviceId"].asInt();
        DeviceInfo deviceInfo = {};
        Json::Value& occupantZones = device["OccupantZones"];
        for (unsigned int j = 0; j < occupantZones.size(); j++) {
            Json::Value& occupantZone = occupantZones[j];
            int zoneId = occupantZone["ZoneId"].asInt();
            std::string occupantTypeStr = occupantZone["OccupantType"].asString();
            int seat = occupantZone["Seat"].asInt();
            OccupantType occupantType;
            if (occupantTypeStr == "DRIVER") {
                occupantType = OccupantType::DRIVER;
            } else if (occupantTypeStr == "FRONT_PASSENGER") {
                occupantType = OccupantType::FRONT_PASSENGER;
            } else if (occupantTypeStr == "REAR_PASSENGER") {
                occupantType = OccupantType::REAR_PASSENGER;
            } else {
                LOG(ERROR) << "Unknown occupant type: " << occupantTypeStr;
                return false;
            }
            OccupantZoneInfo occupantZoneInfo = {
                    .zoneId = zoneId, .occupantType = occupantType, .seat = seat};
            deviceInfo.occupantZones.push_back(std::move(occupantZoneInfo));
        }
        Json::Value& ep = device["EndpointInfo"];
        EndpointInfo endpointInfo = {};
        endpointInfo.connectProtocol = ConnectProtocol::TCP_IP;
        endpointInfo.ipAddress = ep["IpAddress"].asString();
        endpointInfo.portNumber = ep["PortNumber"].asInt();
        HardwareIdentifiers hardwareId = {};
        if (ep.isMember("BrandName")) {
            hardwareId.brandName = ep["BrandName"].asString();
        }
        if (ep.isMember("DeviceName")) {
            hardwareId.deviceName = ep["DeviceName"].asString();
        }
        if (ep.isMember("ProductName")) {
            hardwareId.productName = ep["ProductName"].asString();
        }
        if (ep.isMember("ManufacturerName")) {
            hardwareId.manufacturerName = ep["ManufacturerName"].asString();
        }
        if (ep.isMember("ModelName")) {
            hardwareId.modelName = ep["ModelName"].asString();
        }
        if (ep.isMember("SerialNumber")) {
            hardwareId.serialNumber = ep["SerialNumber"].asString();
        }
        endpointInfo.hardwareId = hardwareId;
        deviceInfo.endpointInfo = endpointInfo;
        mDeviceInfoById[deviceId] = deviceInfo;
    }
    if (mDeviceInfoById.find(mMyDeviceId) == mDeviceInfoById.end()) {
        LOG(ERROR) << "My device ID is not in the device info list";
        return false;
    }
    return true;
}

ScopedAStatus IvnAndroidDeviceService::getMyDeviceId(int* deviceId) {
    *deviceId = mMyDeviceId;
    return ScopedAStatus::ok();
}

ScopedAStatus IvnAndroidDeviceService::getOtherDeviceIds(std::vector<int>* deviceIds) {
    deviceIds->clear();
    for (const auto& [deviceId, _] : mDeviceInfoById) {
        if (deviceId == mMyDeviceId) {
            continue;
        }
        deviceIds->push_back(deviceId);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus IvnAndroidDeviceService::getDeviceIdForOccupantZone(int zoneId, int* outDeviceId) {
    for (const auto& [deviceId, deviceInfo] : mDeviceInfoById) {
        for (const auto& occupantZoneInfo : deviceInfo.occupantZones) {
            if (occupantZoneInfo.zoneId == zoneId) {
                *outDeviceId = deviceId;
                return ScopedAStatus::ok();
            }
        }
    }
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(IVN_ERROR_GENERIC,
                                                              "Occupant zone not found");
}

ScopedAStatus IvnAndroidDeviceService::getOccupantZonesForDevice(
        int androidDeviceId, std::vector<OccupantZoneInfo>* occupantZones) {
    if (mDeviceInfoById.find(androidDeviceId) == mDeviceInfoById.end()) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(IVN_ERROR_GENERIC,
                                                                  "Android device ID not found");
    }
    for (const auto& occupantZoneInfo : mDeviceInfoById[androidDeviceId].occupantZones) {
        occupantZones->push_back(occupantZoneInfo);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus IvnAndroidDeviceService::getMyEndpointInfo(EndpointInfo* endpointInfo) {
    *endpointInfo = mDeviceInfoById[mMyDeviceId].endpointInfo;
    return ScopedAStatus::ok();
}

ScopedAStatus IvnAndroidDeviceService::getEndpointInfoForDevice(int androidDeviceId,
                                                                EndpointInfo* endpointInfo) {
    if (mDeviceInfoById.find(androidDeviceId) == mDeviceInfoById.end()) {
        return ScopedAStatus::fromServiceSpecificErrorWithMessage(IVN_ERROR_GENERIC,
                                                                  "Android device ID not found");
    }
    *endpointInfo = mDeviceInfoById[androidDeviceId].endpointInfo;
    return ScopedAStatus::ok();
}

binder_status_t IvnAndroidDeviceService::dump(int fd, [[maybe_unused]] const char** args,
                                              [[maybe_unused]] uint32_t numArgs) {
    dprintf(fd, "IVN Android Device debug interface, Config: \n%s\n",
            mConfigRootNode.toStyledString().c_str());
    return STATUS_OK;
}

}  // namespace ivn
}  // namespace automotive
}  // namespace hardware
}  // namespace android
