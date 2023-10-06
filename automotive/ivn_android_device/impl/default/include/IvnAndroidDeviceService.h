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

#pragma once

#include <aidl/android/hardware/automotive/ivn/BnIvnAndroidDevice.h>
#include <aidl/android/hardware/automotive/ivn/EndpointInfo.h>
#include <aidl/android/hardware/automotive/ivn/OccupantZoneInfo.h>
#include <android/binder_auto_utils.h>
#include <json/json.h>
#include <vector>

#include <unordered_map>

namespace android {
namespace hardware {
namespace automotive {
namespace ivn {

struct DeviceInfo {
    std::vector<aidl::android::hardware::automotive::ivn::OccupantZoneInfo> occupantZones;
    aidl::android::hardware::automotive::ivn::EndpointInfo endpointInfo;
};

class IvnAndroidDeviceService
    : public aidl::android::hardware::automotive::ivn::BnIvnAndroidDevice {
  public:
    explicit IvnAndroidDeviceService(std::string_view configPath);

    // Initialize the service, returns true on success.
    bool init();

    ndk::ScopedAStatus getMyDeviceId(int* deviceId) override;

    ndk::ScopedAStatus getOtherDeviceIds(std::vector<int>* deviceIds) override;

    ndk::ScopedAStatus getDeviceIdForOccupantZone(int zoneId, int* deviceId) override;

    ndk::ScopedAStatus getOccupantZonesForDevice(
            int androidDeviceId,
            std::vector<aidl::android::hardware::automotive::ivn::OccupantZoneInfo>* occupantZones)
            override;

    ndk::ScopedAStatus getMyEndpointInfo(
            aidl::android::hardware::automotive::ivn::EndpointInfo* endpointInfo) override;

    ndk::ScopedAStatus getEndpointInfoForDevice(
            int androidDeviceId,
            aidl::android::hardware::automotive::ivn::EndpointInfo* endpointInfo) override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

  private:
    Json::Value mConfigRootNode;
    int mMyDeviceId;
    std::unordered_map<int, DeviceInfo> mDeviceInfoById;
    std::string_view mConfigPath;
};

}  // namespace ivn
}  // namespace automotive
}  // namespace hardware
}  // namespace android
