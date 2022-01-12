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

#define LOG_TAG "FakeVehicleHardware"
#define FAKE_VEHICLEHARDWARE_DEBUG false  // STOPSHIP if true.

#include "FakeVehicleHardware.h"

#include <DefaultConfig.h>
#include <FakeObd2Frame.h>
#include <JsonFakeValueGenerator.h>
#include <PropertyUtils.h>
#include <VehicleHalTypes.h>
#include <VehicleUtils.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <dirent.h>
#include <sys/types.h>
#include <fstream>
#include <regex>
#include <unordered_set>
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

using ::android::base::EqualsIgnoreCase;
using ::android::base::Error;
using ::android::base::Result;
using ::android::base::StringPrintf;

const char* VENDOR_OVERRIDE_DIR = "/vendor/etc/automotive/vhaloverride/";
const char* OVERRIDE_PROPERTY = "persist.vendor.vhal_init_value_override";

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
                  getErrorMsg(result).c_str(), getIntErrorCode(result));
        }
    }
}

FakeVehicleHardware::FakeVehicleHardware()
    : mValuePool(new VehiclePropValuePool),
      mServerSidePropStore(new VehiclePropertyStore(mValuePool)),
      mFakeObd2Frame(new obd2frame::FakeObd2Frame(mServerSidePropStore)),
      mFakeUserHal(new FakeUserHal(mValuePool)) {
    init();
}

FakeVehicleHardware::FakeVehicleHardware(std::unique_ptr<VehiclePropValuePool> valuePool)
    : mValuePool(std::move(valuePool)),
      mServerSidePropStore(new VehiclePropertyStore(mValuePool)),
      mFakeObd2Frame(new obd2frame::FakeObd2Frame(mServerSidePropStore)),
      mFakeUserHal(new FakeUserHal(mValuePool)) {
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

Result<void> FakeVehicleHardware::setApPowerStateReport(const VehiclePropValue& value) {
    auto updatedValue = mValuePool->obtain(value);
    updatedValue->timestamp = elapsedRealtimeNano();

    if (auto writeResult = mServerSidePropStore->writeValue(std::move(updatedValue));
        !writeResult.ok()) {
        return Error(getIntErrorCode(writeResult))
               << "failed to write value into property store, error: " << getErrorMsg(writeResult);
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
                return Error(getIntErrorCode(writeResult))
                       << "failed to write AP_POWER_STATE_REQ into property store, error: "
                       << getErrorMsg(writeResult);
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
                return Error(getIntErrorCode(writeResult))
                       << "failed to write AP_POWER_STATE_REQ into property store, error: "
                       << getErrorMsg(writeResult);
            }
            break;
        default:
            ALOGE("Unknown VehicleApPowerStateReport: %d", state);
            break;
    }
    return {};
}

bool FakeVehicleHardware::isHvacPropAndHvacNotAvailable(int32_t propId) {
    std::unordered_set<int32_t> powerProps(std::begin(HVAC_POWER_PROPERTIES),
                                           std::end(HVAC_POWER_PROPERTIES));
    if (powerProps.count(propId)) {
        auto hvacPowerOnResult =
                mServerSidePropStore->readValue(toInt(VehicleProperty::HVAC_POWER_ON), HVAC_ALL);

        if (hvacPowerOnResult.ok() && hvacPowerOnResult.value()->value.int32Values.size() == 1 &&
            hvacPowerOnResult.value()->value.int32Values[0] == 0) {
            return true;
        }
    }
    return false;
}

Result<void> FakeVehicleHardware::setUserHalProp(const VehiclePropValue& value) {
    auto result = mFakeUserHal->onSetProperty(value);
    if (!result.ok()) {
        return Error(getIntErrorCode(result))
               << "onSetProperty(): HAL returned error: " << getErrorMsg(result);
    }
    auto& updatedValue = result.value();
    if (updatedValue != nullptr) {
        ALOGI("onSetProperty(): updating property returned by HAL: %s",
              updatedValue->toString().c_str());
        if (auto writeResult = mServerSidePropStore->writeValue(std::move(result.value()));
            !writeResult.ok()) {
            return Error(getIntErrorCode(writeResult))
                   << "failed to write value into property store, error: "
                   << getErrorMsg(writeResult);
        }
    }
    return {};
}

Result<VehiclePropValuePool::RecyclableType> FakeVehicleHardware::getUserHalProp(
        const VehiclePropValue& value) const {
    auto propId = value.prop;
    ALOGI("get(): getting value for prop %d from User HAL", propId);

    auto result = mFakeUserHal->onGetProperty(value);
    if (!result.ok()) {
        return Error(getIntErrorCode(result))
               << "get(): User HAL returned error: " << getErrorMsg(result);
    } else {
        auto& gotValue = result.value();
        if (gotValue != nullptr) {
            ALOGI("get(): User HAL returned value: %s", gotValue->toString().c_str());
            gotValue->timestamp = elapsedRealtimeNano();
            return result;
        } else {
            return Error(toInt(StatusCode::INTERNAL_ERROR))
                   << "get(): User HAL returned null value";
        }
    }
}

Result<VehiclePropValuePool::RecyclableType> FakeVehicleHardware::maybeGetSpecialValue(
        const VehiclePropValue& value, bool* isSpecialValue) const {
    *isSpecialValue = false;
    int32_t propId = value.prop;
    Result<VehiclePropValuePool::RecyclableType> result;

    if (mFakeUserHal->isSupported(propId)) {
        *isSpecialValue = true;
        return getUserHalProp(value);
    }

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

Result<void> FakeVehicleHardware::maybeSetSpecialValue(const VehiclePropValue& value,
                                                       bool* isSpecialValue) {
    *isSpecialValue = false;
    VehiclePropValuePool::RecyclableType updatedValue;
    int32_t propId = value.prop;

    if (mFakeUserHal->isSupported(propId)) {
        *isSpecialValue = true;
        return setUserHalProp(value);
    }

    if (isHvacPropAndHvacNotAvailable(propId)) {
        *isSpecialValue = true;
        return Error(toInt(StatusCode::NOT_AVAILABLE)) << "hvac not available";
    }

    switch (propId) {
        case toInt(VehicleProperty::AP_POWER_STATE_REPORT):
            *isSpecialValue = true;
            return setApPowerStateReport(value);
        case toInt(VehicleProperty::VEHICLE_MAP_SERVICE):
            // Placeholder for future implementation of VMS property in the default hal. For
            // now, just returns OK; otherwise, hal clients crash with property not supported.
            *isSpecialValue = true;
            return {};
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
                return Error(getIntErrorCode(writeResult))
                       << "failed to write value into property store, error: "
                       << getErrorMsg(writeResult);
            }
            return {};
#endif  // ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING

        default:
            break;
    }
    return {};
}

StatusCode FakeVehicleHardware::setValues(std::shared_ptr<const SetValuesCallback> callback,
                                          const std::vector<SetValueRequest>& requests) {
    std::vector<VehiclePropValue> updatedValues;
    std::vector<SetValueResult> results;
    for (auto& request : requests) {
        const VehiclePropValue& value = request.value;
        int propId = value.prop;

        if (FAKE_VEHICLEHARDWARE_DEBUG) {
            ALOGD("Set value for property ID: %d", propId);
        }

        SetValueResult setValueResult;
        setValueResult.requestId = request.requestId;
        setValueResult.status = StatusCode::OK;

        bool isSpecialValue = false;
        auto setSpecialValueResult = maybeSetSpecialValue(value, &isSpecialValue);

        if (isSpecialValue) {
            if (!setSpecialValueResult.ok()) {
                ALOGE("failed to set special value for property ID: %d, error: %s, status: %d",
                      propId, getErrorMsg(setSpecialValueResult).c_str(),
                      getIntErrorCode(setSpecialValueResult));
                setValueResult.status = getErrorCode(setSpecialValueResult);
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
            ALOGE("failed to write value into property store, error: %s, code: %d",
                  getErrorMsg(writeResult).c_str(), getIntErrorCode(writeResult));
            setValueResult.status = getErrorCode(writeResult);
        }
        results.push_back(std::move(setValueResult));
    }

    // In the real vhal, the values will be sent to Car ECU. We just pretend it is done here and
    // send back the updated property values to client.
    (*callback)(std::move(results));

    return StatusCode::OK;
}

StatusCode FakeVehicleHardware::getValues(std::shared_ptr<const GetValuesCallback> callback,
                                          const std::vector<GetValueRequest>& requests) const {
    std::vector<GetValueResult> results;
    for (auto& request : requests) {
        const VehiclePropValue& value = request.prop;

        if (FAKE_VEHICLEHARDWARE_DEBUG) {
            ALOGD("getValues(%d)", value.prop);
        }

        GetValueResult getValueResult;
        getValueResult.requestId = request.requestId;
        bool isSpecialValue = false;

        auto result = maybeGetSpecialValue(value, &isSpecialValue);
        if (isSpecialValue) {
            if (!result.ok()) {
                ALOGE("failed to get special value: %d, error: %s, code: %d", value.prop,
                      getErrorMsg(result).c_str(), getIntErrorCode(result));
                getValueResult.status = getErrorCode(result);
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
                      toInt(errorCode));
            }
            getValueResult.status = errorCode;
        } else {
            getValueResult.status = StatusCode::OK;
            getValueResult.prop = *readResult.value();
        }
        results.push_back(std::move(getValueResult));
    }

    (*callback)(std::move(results));

    return StatusCode::OK;
}

DumpResult FakeVehicleHardware::dump(const std::vector<std::string>& options) {
    DumpResult result;
    result.callerShouldDumpState = false;
    if (options.size() == 0) {
        // We only want caller to dump default state when there is no options.
        result.callerShouldDumpState = true;
        result.buffer = dumpAllProperties();
        return result;
    }
    std::string option = options[0];
    if (EqualsIgnoreCase(option, "--help")) {
        result.buffer = dumpHelp();
        return result;
    } else if (EqualsIgnoreCase(option, "--list")) {
        result.buffer = dumpListProperties();
    } else if (EqualsIgnoreCase(option, "--get")) {
        result.buffer = dumpSpecificProperty(options);
    } else if (EqualsIgnoreCase(option, "--set")) {
        // TODO(b/214613918): Support debug set values.
    } else {
        result.buffer = StringPrintf("Invalid option: %s\n", option.c_str());
    }
    return result;
}

std::string FakeVehicleHardware::dumpHelp() {
    return "Usage: \n\n"
           "[no args]: dumps (id and value) all supported properties \n"
           "--help: shows this help\n"
           "--list: lists the ids of all supported properties\n"
           "--get <PROP1> [PROP2] [PROPN]: dumps the value of specific properties \n"
           "--set <PROP> [-i INT_VALUE [INT_VALUE ...]] [-i64 INT64_VALUE [INT64_VALUE ...]] "
           "[-f FLOAT_VALUE [FLOAT_VALUE ...]] [-s STR_VALUE] "
           "[-b BYTES_VALUE] [-a AREA_ID] : sets the value of property PROP. "
           "Notice that the string, bytes and area value can be set just once, while the other can"
           " have multiple values (so they're used in the respective array), "
           "BYTES_VALUE is in the form of 0xXXXX, e.g. 0xdeadbeef.\n";
}

std::string FakeVehicleHardware::dumpAllProperties() {
    auto configs = mServerSidePropStore->getAllConfigs();
    if (configs.size() == 0) {
        return "no properties to dump\n";
    }
    std::string msg = StringPrintf("dumping %zu properties\n", configs.size());
    int rowNumber = 1;
    for (const VehiclePropConfig& config : configs) {
        msg += dumpOnePropertyByConfig(rowNumber++, config);
    }
    return msg;
}

std::string FakeVehicleHardware::dumpOnePropertyByConfig(int rowNumber,
                                                         const VehiclePropConfig& config) {
    size_t numberAreas = config.areaConfigs.size();
    std::string msg = "";
    if (numberAreas == 0) {
        msg += StringPrintf("%d: ", rowNumber);
        msg += dumpOnePropertyById(config.prop, /* areaId= */ 0);
        return msg;
    }
    for (size_t j = 0; j < numberAreas; ++j) {
        if (numberAreas > 1) {
            msg += StringPrintf("%d-%zu: ", rowNumber, j);
        } else {
            msg += StringPrintf("%d: ", rowNumber);
        }
        msg += dumpOnePropertyById(config.prop, config.areaConfigs[j].areaId);
    }
    return msg;
}

std::string FakeVehicleHardware::dumpOnePropertyById(int32_t propId, int32_t areaId) {
    VehiclePropValue value = {
            .prop = propId,
            .areaId = areaId,
    };
    bool isSpecialValue = false;
    auto result = maybeGetSpecialValue(value, &isSpecialValue);
    if (!isSpecialValue) {
        result = mServerSidePropStore->readValue(value);
    }
    if (!result.ok()) {
        return StringPrintf("failed to read property value: %d, error: %s, code: %d\n", propId,
                            getErrorMsg(result).c_str(), getIntErrorCode(result));

    } else {
        return result.value()->toString() + "\n";
    }
}

std::string FakeVehicleHardware::dumpListProperties() {
    auto configs = mServerSidePropStore->getAllConfigs();
    if (configs.size() == 0) {
        return "no properties to list\n";
    }
    int rowNumber = 1;
    std::string msg = StringPrintf("listing %zu properties\n", configs.size());
    for (const auto& config : configs) {
        msg += StringPrintf("%d: %d\n", rowNumber++, config.prop);
    }
    return msg;
}

Result<void> FakeVehicleHardware::checkArgumentsSize(const std::vector<std::string>& options,
                                                     size_t minSize) {
    size_t size = options.size();
    if (size >= minSize) {
        return {};
    }
    return Error() << StringPrintf("Invalid number of arguments: required at least %zu, got %zu\n",
                                   minSize, size);
}

std::string FakeVehicleHardware::dumpSpecificProperty(const std::vector<std::string>& options) {
    if (auto result = checkArgumentsSize(options, /*minSize=*/2); !result.ok()) {
        return getErrorMsg(result);
    }

    // options[0] is the command itself...
    int rowNumber = 1;
    size_t size = options.size();
    std::string msg = "";
    for (size_t i = 1; i < size; ++i) {
        auto propResult = safelyParseInt<int32_t>(i, options[i]);
        if (!propResult.ok()) {
            msg += getErrorMsg(propResult);
            continue;
        }
        int32_t prop = propResult.value();
        auto result = mServerSidePropStore->getConfig(prop);
        if (!result.ok()) {
            msg += StringPrintf("No property %d\n", prop);
            continue;
        }
        msg += dumpOnePropertyByConfig(rowNumber++, *result.value());
    }
    return msg;
}

StatusCode FakeVehicleHardware::checkHealth() {
    // Always return OK for checkHealth.
    return StatusCode::OK;
}

void FakeVehicleHardware::registerOnPropertyChangeEvent(
        std::unique_ptr<const PropertyChangeCallback> callback) {
    std::scoped_lock<std::mutex> lockGuard(mCallbackLock);
    mOnPropertyChangeCallback = std::move(callback);
}

void FakeVehicleHardware::registerOnPropertySetErrorEvent(
        std::unique_ptr<const PropertySetErrorCallback> callback) {
    std::scoped_lock<std::mutex> lockGuard(mCallbackLock);
    mOnPropertySetErrorCallback = std::move(callback);
}

void FakeVehicleHardware::onValueChangeCallback(const VehiclePropValue& value) {
    std::scoped_lock<std::mutex> lockGuard(mCallbackLock);

    if (mOnPropertyChangeCallback == nullptr) {
        return;
    }

    std::vector<VehiclePropValue> updatedValues;
    updatedValues.push_back(value);
    (*mOnPropertyChangeCallback)(std::move(updatedValues));
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
                          prop.prop, getErrorMsg(result).c_str(), getIntErrorCode(result));
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
