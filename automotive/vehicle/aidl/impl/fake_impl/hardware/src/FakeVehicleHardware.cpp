/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FakeVehicleHardware.h"

#include <DefaultConfig.h>
#include <FakeObd2Frame.h>
#include <JsonFakeValueGenerator.h>
#include <PropertyUtils.h>
#include <VehicleHalTypes.h>
#include <VehicleUtils.h>
#include <android-base/properties.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <dirent.h>
#include <sys/types.h>
#include <fstream>
#include <regex>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

namespace {

using ::aidl::android::hardware::automotive::vehicle::GetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::RawPropValues;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReport;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReq;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyStatus;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

using ::android::base::Result;

const char* VENDOR_OVERRIDE_DIR = "/vendor/etc/automotive/vhaloverride/";
const char* OVERRIDE_PROPERTY = "persist.vendor.vhal_init_value_override";

template <class T>
StatusCode getErrorCode(const Result<T>& result) {
    if (result.ok()) {
        return StatusCode::OK;
    }
    return static_cast<StatusCode>(result.error().code());
}

template <class T>
std::string getErrorMsg(const Result<T>& result) {
    if (result.ok()) {
        return "";
    }
    return result.error().message();
}

}  // namespace

void FakeVehicleHardware::storePropInitialValue(const defaultconfig::ConfigDeclaration& config) {
    const VehiclePropConfig& vehiclePropConfig = config.config;
    int propId = vehiclePropConfig.prop;

    // A global property will have only a single area
    bool globalProp = isGlobalProp(propId);
    size_t numAreas = globalProp ? 1 : vehiclePropConfig.areaConfigs.size();

    for (size_t i = 0; i < numAreas; i++) {
        int32_t curArea = globalProp ? 0 : vehiclePropConfig.areaConfigs[i].areaId;

        // Create a separate instance for each individual zone
        VehiclePropValue prop = {
                .areaId = curArea,
                .prop = propId,
                .timestamp = elapsedRealtimeNano(),
        };

        if (config.initialAreaValues.empty()) {
            if (config.initialValue == RawPropValues{}) {
                // Skip empty initial values.
                continue;
            }
            prop.value = config.initialValue;
        } else if (auto valueForAreaIt = config.initialAreaValues.find(curArea);
                   valueForAreaIt != config.initialAreaValues.end()) {
            prop.value = valueForAreaIt->second;
        } else {
            ALOGW("failed to get default value for prop 0x%x area 0x%x", propId, curArea);
            continue;
        }

        auto result =
                mServerSidePropStore->writeValue(mValuePool->obtain(prop), /*updateStatus=*/true);
        if (!result.ok()) {
            ALOGE("failed to write default config value, error: %s, status: %d",
                  getErrorMsg(result).c_str(), getErrorCode(result));
        }
    }
}

FakeVehicleHardware::FakeVehicleHardware()
    : mValuePool(new VehiclePropValuePool),
      mServerSidePropStore(new VehiclePropertyStore(mValuePool)),
      mFakeObd2Frame(new obd2frame::FakeObd2Frame(mServerSidePropStore)) {
    init();
}

FakeVehicleHardware::FakeVehicleHardware(std::unique_ptr<VehiclePropValuePool> valuePool)
    : mValuePool(std::move(valuePool)),
      mServerSidePropStore(new VehiclePropertyStore(mValuePool)),
      mFakeObd2Frame(new obd2frame::FakeObd2Frame(mServerSidePropStore)) {
    init();
}

void FakeVehicleHardware::init() {
    for (auto& it : defaultconfig::getDefaultConfigs()) {
        VehiclePropConfig cfg = it.config;
        VehiclePropertyStore::TokenFunction tokenFunction = nullptr;

        if (cfg.prop == OBD2_FREEZE_FRAME) {
            tokenFunction = [](const VehiclePropValue& propValue) { return propValue.timestamp; };
        }

        mServerSidePropStore->registerProperty(cfg, tokenFunction);
        if (obd2frame::FakeObd2Frame::isDiagnosticProperty(cfg)) {
            // Ignore storing default value for diagnostic property. They have special get/set
            // logic.
            continue;
        }
        storePropInitialValue(it);
    }

    maybeOverrideProperties(VENDOR_OVERRIDE_DIR);

    // OBD2_LIVE_FRAME and OBD2_FREEZE_FRAME must be configured in default configs.
    mFakeObd2Frame->initObd2LiveFrame(*mServerSidePropStore->getConfig(OBD2_LIVE_FRAME).value());
    mFakeObd2Frame->initObd2FreezeFrame(
            *mServerSidePropStore->getConfig(OBD2_FREEZE_FRAME).value());

    mServerSidePropStore->setOnValueChangeCallback(
            [this](const VehiclePropValue& value) { return onValueChangeCallback(value); });
}

std::vector<VehiclePropConfig> FakeVehicleHardware::getAllPropertyConfigs() const {
    return mServerSidePropStore->getAllConfigs();
}

VehiclePropValuePool::RecyclableType FakeVehicleHardware::createApPowerStateReq(
        VehicleApPowerStateReq state) {
    auto req = mValuePool->obtain(VehiclePropertyType::INT32_VEC, 2);
    req->prop = toInt(VehicleProperty::AP_POWER_STATE_REQ);
    req->areaId = 0;
    req->timestamp = elapsedRealtimeNano();
    req->status = VehiclePropertyStatus::AVAILABLE;
    req->value.int32Values[0] = toInt(state);
    // Param = 0.
    req->value.int32Values[1] = 0;
    return req;
}

StatusCode FakeVehicleHardware::setApPowerStateReport(const VehiclePropValue& value) {
    auto updatedValue = mValuePool->obtain(value);
    updatedValue->timestamp = elapsedRealtimeNano();

    if (auto writeResult = mServerSidePropStore->writeValue(std::move(updatedValue));
        !writeResult.ok()) {
        StatusCode errorCode = getErrorCode(writeResult);
        ALOGE("failed to write value into property store, error: %s, code: %d",
              getErrorMsg(writeResult).c_str(), errorCode);
        return errorCode;
    }

    VehiclePropValuePool::RecyclableType prop;
    int32_t state = value.value.int32Values[0];
    switch (state) {
        case toInt(VehicleApPowerStateReport::DEEP_SLEEP_EXIT):
            [[fallthrough]];
        case toInt(VehicleApPowerStateReport::HIBERNATION_EXIT):
            [[fallthrough]];
        case toInt(VehicleApPowerStateReport::SHUTDOWN_CANCELLED):
            [[fallthrough]];
        case toInt(VehicleApPowerStateReport::WAIT_FOR_VHAL):
            // CPMS is in WAIT_FOR_VHAL state, simply move to ON
            // Send back to HAL
            // ALWAYS update status for generated property value
            prop = createApPowerStateReq(VehicleApPowerStateReq::ON);
            if (auto writeResult =
                        mServerSidePropStore->writeValue(std::move(prop), /*updateStatus=*/true);
                !writeResult.ok()) {
                StatusCode errorCode = getErrorCode(writeResult);
                ALOGE("failed to write AP_POWER_STATE_REQ into property store, error: %s, code: %d",
                      getErrorMsg(writeResult).c_str(), errorCode);
                return errorCode;
            }
            break;
        case toInt(VehicleApPowerStateReport::DEEP_SLEEP_ENTRY):
            [[fallthrough]];
        case toInt(VehicleApPowerStateReport::HIBERNATION_ENTRY):
            [[fallthrough]];
        case toInt(VehicleApPowerStateReport::SHUTDOWN_START):
            // CPMS is in WAIT_FOR_FINISH state, send the FINISHED command
            // Send back to HAL
            // ALWAYS update status for generated property value
            prop = createApPowerStateReq(VehicleApPowerStateReq::FINISHED);
            if (auto writeResult =
                        mServerSidePropStore->writeValue(std::move(prop), /*updateStatus=*/true);
                !writeResult.ok()) {
                StatusCode errorCode = getErrorCode(writeResult);
                ALOGE("failed to write AP_POWER_STATE_REQ into property store, error: %s, code: %d",
                      getErrorMsg(writeResult).c_str(), errorCode);
                return errorCode;
            }
            break;
        default:
            ALOGE("Unknown VehicleApPowerStateReport: %d", state);
            break;
    }
    return StatusCode::OK;
}

Result<VehiclePropValuePool::RecyclableType> FakeVehicleHardware::maybeGetSpecialValue(
        const VehiclePropValue& value, bool* isSpecialValue) const {
    *isSpecialValue = false;
    int32_t propId = value.prop;
    Result<VehiclePropValuePool::RecyclableType> result;

    switch (propId) {
        case OBD2_FREEZE_FRAME:
            *isSpecialValue = true;
            result = mFakeObd2Frame->getObd2FreezeFrame(value);
            if (result.ok()) {
                result.value()->timestamp = elapsedRealtimeNano();
            }
            return result;
        case OBD2_FREEZE_FRAME_INFO:
            *isSpecialValue = true;
            result = mFakeObd2Frame->getObd2DtcInfo();
            if (result.ok()) {
                result.value()->timestamp = elapsedRealtimeNano();
            }
            return result;
        default:
            // Do nothing.
            break;
    }

    return nullptr;
}

StatusCode FakeVehicleHardware::maybeSetSpecialValue(const VehiclePropValue& value,
                                                     bool* isSpecialValue) {
    *isSpecialValue = false;
    VehiclePropValuePool::RecyclableType updatedValue;

    switch (value.prop) {
        case toInt(VehicleProperty::AP_POWER_STATE_REPORT):
            *isSpecialValue = true;
            return setApPowerStateReport(value);
        case toInt(VehicleProperty::VEHICLE_MAP_SERVICE):
            // Placeholder for future implementation of VMS property in the default hal. For
            // now, just returns OK; otherwise, hal clients crash with property not supported.
            *isSpecialValue = true;
            return StatusCode::OK;
        case OBD2_FREEZE_FRAME_CLEAR:
            *isSpecialValue = true;
            return mFakeObd2Frame->clearObd2FreezeFrames(value);

#ifdef ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING
        case toInt(VehicleProperty::CLUSTER_REPORT_STATE):
            [[fallthrough]];
        case toInt(VehicleProperty::CLUSTER_REQUEST_DISPLAY):
            [[fallthrough]];
        case toInt(VehicleProperty::CLUSTER_NAVIGATION_STATE):
            [[fallthrough]];
        case VENDOR_CLUSTER_SWITCH_UI:
            [[fallthrough]];
        case VENDOR_CLUSTER_DISPLAY_STATE:
            *isSpecialValue = true;
            updatedValue = mValuePool->obtain(getPropType(value.prop));
            updatedValue->prop = value.prop & ~toInt(VehiclePropertyGroup::MASK);
            if (getPropGroup(value.prop) == VehiclePropertyGroup::SYSTEM) {
                updatedValue->prop |= toInt(VehiclePropertyGroup::VENDOR);
            } else {
                updatedValue->prop |= toInt(VehiclePropertyGroup::SYSTEM);
            }
            updatedValue->value = value.value;
            updatedValue->timestamp = elapsedRealtimeNano();
            updatedValue->areaId = value.areaId;
            if (auto writeResult = mServerSidePropStore->writeValue(std::move(updatedValue));
                !writeResult.ok()) {
                StatusCode errorCode = getErrorCode(writeResult);
                ALOGE("failed to write value into property store, error: %s, code: %d",
                      getErrorMsg(writeResult).c_str(), errorCode);
                return errorCode;
            }
            return StatusCode::OK;
#endif  // ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING

        default:
            break;
    }
    return StatusCode::OK;
}

StatusCode FakeVehicleHardware::setValues(FakeVehicleHardware::SetValuesCallback&& callback,
                                          const std::vector<SetValueRequest>& requests) {
    std::vector<VehiclePropValue> updatedValues;
    std::vector<SetValueResult> results;
    for (auto& request : requests) {
        const VehiclePropValue& value = request.value;
        int propId = value.prop;

        ALOGD("Set value for property ID: %d", propId);

        SetValueResult setValueResult;
        setValueResult.requestId = request.requestId;
        setValueResult.status = StatusCode::OK;

        bool isSpecialValue = false;
        StatusCode status = maybeSetSpecialValue(value, &isSpecialValue);

        if (isSpecialValue) {
            if (status != StatusCode::OK) {
                ALOGE("failed to set special value for property ID: %d, status: %d", propId,
                      status);
                setValueResult.status = status;
            }

            // Special values are already handled.
            results.push_back(std::move(setValueResult));
            continue;
        }

        auto updatedValue = mValuePool->obtain(value);
        int64_t timestamp = elapsedRealtimeNano();
        updatedValue->timestamp = timestamp;

        auto writeResult = mServerSidePropStore->writeValue(std::move(updatedValue));
        if (!writeResult.ok()) {
            StatusCode errorCode = getErrorCode(writeResult);
            ALOGE("failed to write value into property store, error: %s, code: %d",
                  getErrorMsg(writeResult).c_str(), errorCode);
            setValueResult.status = errorCode;
        }
        results.push_back(std::move(setValueResult));
    }

    // In the real vhal, the values will be sent to Car ECU. We just pretend it is done here and
    // send back the updated property values to client.
    callback(std::move(results));

    return StatusCode::OK;
}

StatusCode FakeVehicleHardware::getValues(FakeVehicleHardware::GetValuesCallback&& callback,
                                          const std::vector<GetValueRequest>& requests) const {
    std::vector<GetValueResult> results;
    for (auto& request : requests) {
        const VehiclePropValue& value = request.prop;
        ALOGD("getValues(%d)", value.prop);

        GetValueResult getValueResult;
        getValueResult.requestId = request.requestId;
        bool isSpecialValue = false;

        auto result = maybeGetSpecialValue(value, &isSpecialValue);
        if (isSpecialValue) {
            if (!result.ok()) {
                StatusCode errorCode = getErrorCode(result);
                ALOGE("failed to get special value: %d, error: %s, code: %d", value.prop,
                      getErrorMsg(result).c_str(), errorCode);
                getValueResult.status = errorCode;
            } else {
                getValueResult.status = StatusCode::OK;
                getValueResult.prop = *result.value();
            }
            results.push_back(std::move(getValueResult));
            continue;
        }

        auto readResult = mServerSidePropStore->readValue(value);
        if (!readResult.ok()) {
            StatusCode errorCode = getErrorCode(readResult);
            if (errorCode == StatusCode::NOT_AVAILABLE) {
                ALOGW("%s", "value has not been set yet");
            } else {
                ALOGE("failed to get value, error: %s, code: %d", getErrorMsg(readResult).c_str(),
                      errorCode);
            }
            getValueResult.status = errorCode;
        } else {
            getValueResult.status = StatusCode::OK;
            getValueResult.prop = *readResult.value();
        }
        results.push_back(std::move(getValueResult));
    }

    callback(std::move(results));

    return StatusCode::OK;
}

DumpResult FakeVehicleHardware::dump(const std::vector<std::string>&) {
    DumpResult result;
    // TODO(b/201830716): Implement this.
    return result;
}

StatusCode FakeVehicleHardware::checkHealth() {
    // TODO(b/201830716): Implement this.
    return StatusCode::OK;
}

void FakeVehicleHardware::registerOnPropertyChangeEvent(OnPropertyChangeCallback&& callback) {
    std::scoped_lock<std::mutex> lockGuard(mCallbackLock);
    mOnPropertyChangeCallback = std::move(callback);
}

void FakeVehicleHardware::registerOnPropertySetErrorEvent(OnPropertySetErrorCallback&& callback) {
    std::scoped_lock<std::mutex> lockGuard(mCallbackLock);
    mOnPropertySetErrorCallback = std::move(callback);
}

void FakeVehicleHardware::onValueChangeCallback(const VehiclePropValue& value) {
    std::scoped_lock<std::mutex> lockGuard(mCallbackLock);
    if (mOnPropertyChangeCallback != nullptr) {
        std::vector<VehiclePropValue> updatedValues;
        updatedValues.push_back(value);
        mOnPropertyChangeCallback(std::move(updatedValues));
    }
}

void FakeVehicleHardware::maybeOverrideProperties(const char* overrideDir) {
    if (android::base::GetBoolProperty(OVERRIDE_PROPERTY, false)) {
        overrideProperties(overrideDir);
    }
}

void FakeVehicleHardware::overrideProperties(const char* overrideDir) {
    ALOGI("loading vendor override properties from %s", overrideDir);
    if (auto dir = opendir(overrideDir); dir != NULL) {
        std::regex regJson(".*[.]json", std::regex::icase);
        while (auto f = readdir(dir)) {
            if (!std::regex_match(f->d_name, regJson)) {
                continue;
            }
            std::string file = overrideDir + std::string(f->d_name);
            JsonFakeValueGenerator tmpGenerator(file);

            std::vector<VehiclePropValue> propValues = tmpGenerator.getAllEvents();
            for (const VehiclePropValue& prop : propValues) {
                auto propToStore = mValuePool->obtain(prop);
                propToStore->timestamp = elapsedRealtimeNano();
                if (auto result = mServerSidePropStore->writeValue(std::move(propToStore),
                                                                   /*updateStatus=*/true);
                    !result.ok()) {
                    ALOGW("failed to write vendor override properties: %d, error: %s, code: %d",
                          prop.prop, getErrorMsg(result).c_str(), getErrorCode(result));
                }
            }
        }
        closedir(dir);
    }
}

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
