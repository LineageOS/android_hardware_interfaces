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
#define ATRACE_TAG ATRACE_TAG_HAL
#define FAKE_VEHICLEHARDWARE_DEBUG false  // STOPSHIP if true.

#include "FakeVehicleHardware.h"

#include <FakeObd2Frame.h>
#include <JsonFakeValueGenerator.h>
#include <LinearFakeValueGenerator.h>
#include <PropertyUtils.h>
#include <TestPropertyUtils.h>
#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

#include <android-base/file.h>
#include <android-base/parsedouble.h>
#include <android-base/properties.h>
#include <android-base/strings.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>
#include <utils/Trace.h>

#include <dirent.h>
#include <inttypes.h>
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

using ::aidl::android::hardware::automotive::vehicle::ErrorState;
using ::aidl::android::hardware::automotive::vehicle::GetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::GetValueResult;
using ::aidl::android::hardware::automotive::vehicle::RawPropValues;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequest;
using ::aidl::android::hardware::automotive::vehicle::SetValueResult;
using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReport;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReq;
using ::aidl::android::hardware::automotive::vehicle::VehicleHwKeyInputAction;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyAccess;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyStatus;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;
using ::aidl::android::hardware::automotive::vehicle::VehicleUnit;

using ::android::base::EqualsIgnoreCase;
using ::android::base::Error;
using ::android::base::GetIntProperty;
using ::android::base::ParseFloat;
using ::android::base::Result;
using ::android::base::ScopedLockAssertion;
using ::android::base::StartsWith;
using ::android::base::StringPrintf;

// In order to test large number of vehicle property configs, we might generate additional fake
// property config start from this ID. These fake properties are for getPropertyList,
//  getPropertiesAsync, and setPropertiesAsync.
// 0x21403000
constexpr int32_t STARTING_VENDOR_CODE_PROPERTIES_FOR_TEST =
        0x3000 | toInt(testpropertyutils_impl::VehiclePropertyGroup::VENDOR) |
        toInt(testpropertyutils_impl::VehicleArea::GLOBAL) |
        toInt(testpropertyutils_impl::VehiclePropertyType::INT32);
// 0x21405000
constexpr int32_t ENDING_VENDOR_CODE_PROPERTIES_FOR_TEST =
        0x5000 | toInt(testpropertyutils_impl::VehiclePropertyGroup::VENDOR) |
        toInt(testpropertyutils_impl::VehicleArea::GLOBAL) |
        toInt(testpropertyutils_impl::VehiclePropertyType::INT32);
// The directory for default property configuration file.
// For config file format, see impl/default_config/config/README.md.
constexpr char DEFAULT_CONFIG_DIR[] = "/vendor/etc/automotive/vhalconfig/";
// The directory for property configuration file that overrides the default configuration file.
// For config file format, see impl/default_config/config/README.md.
constexpr char OVERRIDE_CONFIG_DIR[] = "/vendor/etc/automotive/vhaloverride/";
// If OVERRIDE_PROPERTY is set, we will use the configuration files from OVERRIDE_CONFIG_DIR to
// overwrite the default configs.
constexpr char OVERRIDE_PROPERTY[] = "persist.vendor.vhal_init_value_override";
constexpr char POWER_STATE_REQ_CONFIG_PROPERTY[] = "ro.vendor.fake_vhal.ap_power_state_req.config";
// The value to be returned if VENDOR_PROPERTY_ID is set as the property
constexpr int VENDOR_ERROR_CODE = 0x00ab0005;
// A list of supported options for "--set" command.
const std::unordered_set<std::string> SET_PROP_OPTIONS = {
        // integer.
        "-i",
        // 64bit integer.
        "-i64",
        // float.
        "-f",
        // string.
        "-s",
        // bytes in hex format, e.g. 0xDEADBEEF.
        "-b",
        // Area id in integer.
        "-a",
        // Timestamp in int64.
        "-t"};

// ADAS _ENABLED property to list of ADAS state properties using ErrorState enum.
const std::unordered_map<int32_t, std::vector<int32_t>> mAdasEnabledPropToAdasPropWithErrorState = {
        // AEB
        {
                toInt(VehicleProperty::AUTOMATIC_EMERGENCY_BRAKING_ENABLED),
                {
                        toInt(VehicleProperty::AUTOMATIC_EMERGENCY_BRAKING_STATE),
                },
        },
        // FCW
        {
                toInt(VehicleProperty::FORWARD_COLLISION_WARNING_ENABLED),
                {
                        toInt(VehicleProperty::FORWARD_COLLISION_WARNING_STATE),
                },
        },
        // BSW
        {
                toInt(VehicleProperty::BLIND_SPOT_WARNING_ENABLED),
                {
                        toInt(VehicleProperty::BLIND_SPOT_WARNING_STATE),
                },
        },
        // LDW
        {
                toInt(VehicleProperty::LANE_DEPARTURE_WARNING_ENABLED),
                {
                        toInt(VehicleProperty::LANE_DEPARTURE_WARNING_STATE),
                },
        },
        // LKA
        {
                toInt(VehicleProperty::LANE_KEEP_ASSIST_ENABLED),
                {
                        toInt(VehicleProperty::LANE_KEEP_ASSIST_STATE),
                },
        },
        // LCA
        {
                toInt(VehicleProperty::LANE_CENTERING_ASSIST_ENABLED),
                {
                        toInt(VehicleProperty::LANE_CENTERING_ASSIST_STATE),
                },
        },
        // ELKA
        {
                toInt(VehicleProperty::EMERGENCY_LANE_KEEP_ASSIST_ENABLED),
                {
                        toInt(VehicleProperty::EMERGENCY_LANE_KEEP_ASSIST_STATE),
                },
        },
        // CC
        {
                toInt(VehicleProperty::CRUISE_CONTROL_ENABLED),
                {
                        toInt(VehicleProperty::CRUISE_CONTROL_TYPE),
                        toInt(VehicleProperty::CRUISE_CONTROL_STATE),
                },
        },
        // HOD
        {
                toInt(VehicleProperty::HANDS_ON_DETECTION_ENABLED),
                {
                        toInt(VehicleProperty::HANDS_ON_DETECTION_DRIVER_STATE),
                        toInt(VehicleProperty::HANDS_ON_DETECTION_WARNING),
                },
        },
};
}  // namespace

void FakeVehicleHardware::storePropInitialValue(const ConfigDeclaration& config) {
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
    : FakeVehicleHardware(DEFAULT_CONFIG_DIR, OVERRIDE_CONFIG_DIR, false) {}

FakeVehicleHardware::FakeVehicleHardware(std::string defaultConfigDir,
                                         std::string overrideConfigDir, bool forceOverride)
    : mValuePool(std::make_unique<VehiclePropValuePool>()),
      mServerSidePropStore(new VehiclePropertyStore(mValuePool)),
      mFakeObd2Frame(new obd2frame::FakeObd2Frame(mServerSidePropStore)),
      mFakeUserHal(new FakeUserHal(mValuePool)),
      mRecurrentTimer(new RecurrentTimer()),
      mGeneratorHub(new GeneratorHub(
              [this](const VehiclePropValue& value) { eventFromVehicleBus(value); })),
      mPendingGetValueRequests(this),
      mPendingSetValueRequests(this),
      mDefaultConfigDir(defaultConfigDir),
      mOverrideConfigDir(overrideConfigDir),
      mForceOverride(forceOverride) {
    init();
}

FakeVehicleHardware::~FakeVehicleHardware() {
    mPendingGetValueRequests.stop();
    mPendingSetValueRequests.stop();
    mGeneratorHub.reset();
}

std::unordered_map<int32_t, ConfigDeclaration> FakeVehicleHardware::loadConfigDeclarations() {
    std::unordered_map<int32_t, ConfigDeclaration> configsByPropId;
    loadPropConfigsFromDir(mDefaultConfigDir, &configsByPropId);
    if (mForceOverride ||
        android::base::GetBoolProperty(OVERRIDE_PROPERTY, /*default_value=*/false)) {
        loadPropConfigsFromDir(mOverrideConfigDir, &configsByPropId);
    }
    return configsByPropId;
}

void FakeVehicleHardware::init() {
    for (auto& [_, configDeclaration] : loadConfigDeclarations()) {
        VehiclePropConfig cfg = configDeclaration.config;
        VehiclePropertyStore::TokenFunction tokenFunction = nullptr;

        if (cfg.prop == toInt(VehicleProperty::AP_POWER_STATE_REQ)) {
            int config = GetIntProperty(POWER_STATE_REQ_CONFIG_PROPERTY, /*default_value=*/0);
            cfg.configArray[0] = config;
        } else if (cfg.prop == OBD2_FREEZE_FRAME) {
            tokenFunction = [](const VehiclePropValue& propValue) { return propValue.timestamp; };
        }

        mServerSidePropStore->registerProperty(cfg, tokenFunction);
        if (obd2frame::FakeObd2Frame::isDiagnosticProperty(cfg)) {
            // Ignore storing default value for diagnostic property. They have special get/set
            // logic.
            continue;
        }
        storePropInitialValue(configDeclaration);
    }

    // OBD2_LIVE_FRAME and OBD2_FREEZE_FRAME must be configured in default configs.
    auto maybeObd2LiveFrame = mServerSidePropStore->getConfig(OBD2_LIVE_FRAME);
    if (maybeObd2LiveFrame.has_value()) {
        mFakeObd2Frame->initObd2LiveFrame(*maybeObd2LiveFrame.value());
    }
    auto maybeObd2FreezeFrame = mServerSidePropStore->getConfig(OBD2_FREEZE_FRAME);
    if (maybeObd2FreezeFrame.has_value()) {
        mFakeObd2Frame->initObd2FreezeFrame(*maybeObd2FreezeFrame.value());
    }

    mServerSidePropStore->setOnValueChangeCallback(
            [this](const VehiclePropValue& value) { return onValueChangeCallback(value); });
}

std::vector<VehiclePropConfig> FakeVehicleHardware::getAllPropertyConfigs() const {
    std::vector<VehiclePropConfig> allConfigs = mServerSidePropStore->getAllConfigs();
    if (mAddExtraTestVendorConfigs) {
        generateVendorConfigs(/* outAllConfigs= */ allConfigs);
    }
    return allConfigs;
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

VehiclePropValuePool::RecyclableType FakeVehicleHardware::createAdasStateReq(int32_t propertyId,
                                                                             int32_t areaId,
                                                                             int32_t state) {
    auto req = mValuePool->obtain(VehiclePropertyType::INT32);
    req->prop = propertyId;
    req->areaId = areaId;
    req->timestamp = elapsedRealtimeNano();
    req->status = VehiclePropertyStatus::AVAILABLE;
    req->value.int32Values[0] = state;
    return req;
}

VhalResult<void> FakeVehicleHardware::setApPowerStateReport(const VehiclePropValue& value) {
    auto updatedValue = mValuePool->obtain(value);
    updatedValue->timestamp = elapsedRealtimeNano();

    if (auto writeResult = mServerSidePropStore->writeValue(std::move(updatedValue));
        !writeResult.ok()) {
        return StatusError(getErrorCode(writeResult))
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
            // CPMS is in WAIT_FOR_VHAL state, simply move to ON and send back to HAL.
            prop = createApPowerStateReq(VehicleApPowerStateReq::ON);

            // ALWAYS update status for generated property value, and force a property update event
            // because in the case when Car Service crashes, the power state would already be ON
            // when we receive WAIT_FOR_VHAL and thus new property change event would be generated.
            // However, Car Service always expect a property change event even though there is no
            // actual state change.
            if (auto writeResult =
                        mServerSidePropStore->writeValue(std::move(prop), /*updateStatus=*/true,
                                                         VehiclePropertyStore::EventMode::ALWAYS);
                !writeResult.ok()) {
                return StatusError(getErrorCode(writeResult))
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
                return StatusError(getErrorCode(writeResult))
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

int FakeVehicleHardware::getHvacTempNumIncrements(int requestedTemp, int minTemp, int maxTemp,
                                                  int increment) {
    requestedTemp = std::max(requestedTemp, minTemp);
    requestedTemp = std::min(requestedTemp, maxTemp);
    int numIncrements = (requestedTemp - minTemp) / increment;
    return numIncrements;
}

void FakeVehicleHardware::updateHvacTemperatureValueSuggestionInput(
        const std::vector<int>& hvacTemperatureSetConfigArray,
        std::vector<float>* hvacTemperatureValueSuggestionInput) {
    int minTempInCelsius = hvacTemperatureSetConfigArray[0];
    int maxTempInCelsius = hvacTemperatureSetConfigArray[1];
    int incrementInCelsius = hvacTemperatureSetConfigArray[2];

    int minTempInFahrenheit = hvacTemperatureSetConfigArray[3];
    int maxTempInFahrenheit = hvacTemperatureSetConfigArray[4];
    int incrementInFahrenheit = hvacTemperatureSetConfigArray[5];

    // The HVAC_TEMPERATURE_SET config array values are temperature values that have been multiplied
    // by 10 and converted to integers. Therefore, requestedTemp must also be multiplied by 10 and
    // converted to an integer in order for them to be the same units.
    int requestedTemp = static_cast<int>((*hvacTemperatureValueSuggestionInput)[0] * 10.0f);
    int numIncrements =
            (*hvacTemperatureValueSuggestionInput)[1] == toInt(VehicleUnit::CELSIUS)
                    ? getHvacTempNumIncrements(requestedTemp, minTempInCelsius, maxTempInCelsius,
                                               incrementInCelsius)
                    : getHvacTempNumIncrements(requestedTemp, minTempInFahrenheit,
                                               maxTempInFahrenheit, incrementInFahrenheit);

    int suggestedTempInCelsius = minTempInCelsius + incrementInCelsius * numIncrements;
    int suggestedTempInFahrenheit = minTempInFahrenheit + incrementInFahrenheit * numIncrements;
    // HVAC_TEMPERATURE_VALUE_SUGGESTION specifies the temperature values to be in the original
    // floating point form so we divide by 10 and convert to float.
    (*hvacTemperatureValueSuggestionInput)[2] = static_cast<float>(suggestedTempInCelsius) / 10.0f;
    (*hvacTemperatureValueSuggestionInput)[3] =
            static_cast<float>(suggestedTempInFahrenheit) / 10.0f;
}

VhalResult<void> FakeVehicleHardware::setHvacTemperatureValueSuggestion(
        const VehiclePropValue& hvacTemperatureValueSuggestion) {
    auto hvacTemperatureSetConfigResult =
            mServerSidePropStore->getConfig(toInt(VehicleProperty::HVAC_TEMPERATURE_SET));

    if (!hvacTemperatureSetConfigResult.ok()) {
        return StatusError(getErrorCode(hvacTemperatureSetConfigResult)) << StringPrintf(
                       "Failed to set HVAC_TEMPERATURE_VALUE_SUGGESTION because"
                       " HVAC_TEMPERATURE_SET could not be retrieved. Error: %s",
                       getErrorMsg(hvacTemperatureSetConfigResult).c_str());
    }

    const auto& originalInput = hvacTemperatureValueSuggestion.value.floatValues;
    if (originalInput.size() != 4) {
        return StatusError(StatusCode::INVALID_ARG) << StringPrintf(
                       "Failed to set HVAC_TEMPERATURE_VALUE_SUGGESTION because float"
                       " array value is not size 4.");
    }

    bool isTemperatureUnitSpecified = originalInput[1] == toInt(VehicleUnit::CELSIUS) ||
                                      originalInput[1] == toInt(VehicleUnit::FAHRENHEIT);
    if (!isTemperatureUnitSpecified) {
        return StatusError(StatusCode::INVALID_ARG) << StringPrintf(
                       "Failed to set HVAC_TEMPERATURE_VALUE_SUGGESTION because float"
                       " value at index 1 is not any of %d or %d, which corresponds to"
                       " VehicleUnit#CELSIUS and VehicleUnit#FAHRENHEIT respectively.",
                       toInt(VehicleUnit::CELSIUS), toInt(VehicleUnit::FAHRENHEIT));
    }

    auto updatedValue = mValuePool->obtain(hvacTemperatureValueSuggestion);
    const auto& hvacTemperatureSetConfigArray = hvacTemperatureSetConfigResult.value()->configArray;
    auto& hvacTemperatureValueSuggestionInput = updatedValue->value.floatValues;

    updateHvacTemperatureValueSuggestionInput(hvacTemperatureSetConfigArray,
                                              &hvacTemperatureValueSuggestionInput);

    updatedValue->timestamp = elapsedRealtimeNano();
    auto writeResult = mServerSidePropStore->writeValue(std::move(updatedValue),
                                                        /* updateStatus = */ true,
                                                        VehiclePropertyStore::EventMode::ALWAYS);
    if (!writeResult.ok()) {
        return StatusError(getErrorCode(writeResult))
               << StringPrintf("failed to write value into property store, error: %s",
                               getErrorMsg(writeResult).c_str());
    }

    return {};
}

bool FakeVehicleHardware::isHvacPropAndHvacNotAvailable(int32_t propId, int32_t areaId) const {
    std::unordered_set<int32_t> powerProps(std::begin(HVAC_POWER_PROPERTIES),
                                           std::end(HVAC_POWER_PROPERTIES));
    if (powerProps.count(propId)) {
        auto hvacPowerOnResults =
                mServerSidePropStore->readValuesForProperty(toInt(VehicleProperty::HVAC_POWER_ON));
        if (!hvacPowerOnResults.ok()) {
            ALOGW("failed to get HVAC_POWER_ON 0x%x, error: %s",
                  toInt(VehicleProperty::HVAC_POWER_ON), getErrorMsg(hvacPowerOnResults).c_str());
            return false;
        }
        auto& hvacPowerOnValues = hvacPowerOnResults.value();
        for (size_t j = 0; j < hvacPowerOnValues.size(); j++) {
            auto hvacPowerOnValue = std::move(hvacPowerOnValues[j]);
            if ((hvacPowerOnValue->areaId & areaId) == areaId) {
                if (hvacPowerOnValue->value.int32Values.size() == 1 &&
                    hvacPowerOnValue->value.int32Values[0] == 0) {
                    return true;
                }
                break;
            }
        }
    }
    return false;
}

VhalResult<void> FakeVehicleHardware::isAdasPropertyAvailable(int32_t adasStatePropertyId) const {
    auto adasStateResult = mServerSidePropStore->readValue(adasStatePropertyId);
    if (!adasStateResult.ok()) {
        ALOGW("Failed to get ADAS ENABLED property 0x%x, error: %s", adasStatePropertyId,
              getErrorMsg(adasStateResult).c_str());
        return {};
    }

    if (adasStateResult.value()->value.int32Values.size() == 1 &&
        adasStateResult.value()->value.int32Values[0] < 0) {
        auto errorState = adasStateResult.value()->value.int32Values[0];
        switch (errorState) {
            case toInt(ErrorState::NOT_AVAILABLE_DISABLED):
                return StatusError(StatusCode::NOT_AVAILABLE_DISABLED)
                       << "ADAS feature is disabled.";
            case toInt(ErrorState::NOT_AVAILABLE_SPEED_LOW):
                return StatusError(StatusCode::NOT_AVAILABLE_SPEED_LOW)
                       << "ADAS feature is disabled because the vehicle speed is too low.";
            case toInt(ErrorState::NOT_AVAILABLE_SPEED_HIGH):
                return StatusError(StatusCode::NOT_AVAILABLE_SPEED_HIGH)
                       << "ADAS feature is disabled because the vehicle speed is too high.";
            case toInt(ErrorState::NOT_AVAILABLE_POOR_VISIBILITY):
                return StatusError(StatusCode::NOT_AVAILABLE_POOR_VISIBILITY)
                       << "ADAS feature is disabled because the visibility is too poor.";
            case toInt(ErrorState::NOT_AVAILABLE_SAFETY):
                return StatusError(StatusCode::NOT_AVAILABLE_SAFETY)
                       << "ADAS feature is disabled because of safety reasons.";
            default:
                return StatusError(StatusCode::NOT_AVAILABLE) << "ADAS feature is not available.";
        }
    }

    return {};
}

VhalResult<void> FakeVehicleHardware::setUserHalProp(const VehiclePropValue& value) {
    auto result = mFakeUserHal->onSetProperty(value);
    if (!result.ok()) {
        return StatusError(getErrorCode(result))
               << "onSetProperty(): HAL returned error: " << getErrorMsg(result);
    }
    auto& updatedValue = result.value();
    if (updatedValue != nullptr) {
        ALOGI("onSetProperty(): updating property returned by HAL: %s",
              updatedValue->toString().c_str());
        // Update timestamp otherwise writeValue might fail because the timestamp is outdated.
        updatedValue->timestamp = elapsedRealtimeNano();
        if (auto writeResult = mServerSidePropStore->writeValue(
                    std::move(result.value()),
                    /*updateStatus=*/true, VehiclePropertyStore::EventMode::ALWAYS);
            !writeResult.ok()) {
            return StatusError(getErrorCode(writeResult))
                   << "failed to write value into property store, error: "
                   << getErrorMsg(writeResult);
        }
    }
    return {};
}

FakeVehicleHardware::ValueResultType FakeVehicleHardware::getUserHalProp(
        const VehiclePropValue& value) const {
    auto propId = value.prop;
    ALOGI("get(): getting value for prop %d from User HAL", propId);

    auto result = mFakeUserHal->onGetProperty(value);
    if (!result.ok()) {
        return StatusError(getErrorCode(result))
               << "get(): User HAL returned error: " << getErrorMsg(result);
    } else {
        auto& gotValue = result.value();
        if (gotValue != nullptr) {
            ALOGI("get(): User HAL returned value: %s", gotValue->toString().c_str());
            gotValue->timestamp = elapsedRealtimeNano();
            return result;
        } else {
            return StatusError(StatusCode::INTERNAL_ERROR) << "get(): User HAL returned null value";
        }
    }
}

FakeVehicleHardware::ValueResultType FakeVehicleHardware::maybeGetSpecialValue(
        const VehiclePropValue& value, bool* isSpecialValue) const {
    *isSpecialValue = false;
    int32_t propId = value.prop;
    ValueResultType result;

    if (propId >= STARTING_VENDOR_CODE_PROPERTIES_FOR_TEST &&
        propId < ENDING_VENDOR_CODE_PROPERTIES_FOR_TEST) {
        *isSpecialValue = true;
        result = mValuePool->obtainInt32(/* value= */ 5);

        result.value()->prop = propId;
        result.value()->areaId = 0;
        result.value()->timestamp = elapsedRealtimeNano();
        return result;
    }

    if (mFakeUserHal->isSupported(propId)) {
        *isSpecialValue = true;
        return getUserHalProp(value);
    }

    if (isHvacPropAndHvacNotAvailable(propId, value.areaId)) {
        *isSpecialValue = true;
        return StatusError(StatusCode::NOT_AVAILABLE_DISABLED) << "hvac not available";
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
        case ECHO_REVERSE_BYTES:
            *isSpecialValue = true;
            return getEchoReverseBytes(value);
        case VENDOR_PROPERTY_ID:
            *isSpecialValue = true;
            return StatusError((StatusCode)VENDOR_ERROR_CODE);
        case toInt(VehicleProperty::CRUISE_CONTROL_TARGET_SPEED):
            [[fallthrough]];
        case toInt(VehicleProperty::ADAPTIVE_CRUISE_CONTROL_TARGET_TIME_GAP):
            [[fallthrough]];
        case toInt(VehicleProperty::ADAPTIVE_CRUISE_CONTROL_LEAD_VEHICLE_MEASURED_DISTANCE): {
            auto isAdasPropertyAvailableResult =
                    isAdasPropertyAvailable(toInt(VehicleProperty::CRUISE_CONTROL_STATE));
            if (!isAdasPropertyAvailableResult.ok()) {
                *isSpecialValue = true;
                return isAdasPropertyAvailableResult.error();
            }
            return nullptr;
        }
        default:
            // Do nothing.
            break;
    }

    return nullptr;
}

FakeVehicleHardware::ValueResultType FakeVehicleHardware::getEchoReverseBytes(
        const VehiclePropValue& value) const {
    auto readResult = mServerSidePropStore->readValue(value);
    if (!readResult.ok()) {
        return readResult;
    }
    auto& gotValue = readResult.value();
    gotValue->timestamp = elapsedRealtimeNano();
    std::vector<uint8_t> byteValues = gotValue->value.byteValues;
    size_t byteSize = byteValues.size();
    for (size_t i = 0; i < byteSize; i++) {
        gotValue->value.byteValues[i] = byteValues[byteSize - 1 - i];
    }
    return std::move(gotValue);
}

void FakeVehicleHardware::sendHvacPropertiesCurrentValues(int32_t areaId) {
    for (size_t i = 0; i < sizeof(HVAC_POWER_PROPERTIES) / sizeof(int32_t); i++) {
        int powerPropId = HVAC_POWER_PROPERTIES[i];
        auto powerPropResults = mServerSidePropStore->readValuesForProperty(powerPropId);
        if (!powerPropResults.ok()) {
            ALOGW("failed to get power prop 0x%x, error: %s", powerPropId,
                  getErrorMsg(powerPropResults).c_str());
            continue;
        }
        auto& powerPropValues = powerPropResults.value();
        for (size_t j = 0; j < powerPropValues.size(); j++) {
            auto powerPropValue = std::move(powerPropValues[j]);
            if ((powerPropValue->areaId & areaId) == powerPropValue->areaId) {
                powerPropValue->status = VehiclePropertyStatus::AVAILABLE;
                powerPropValue->timestamp = elapsedRealtimeNano();
                // This will trigger a property change event for the current hvac property value.
                mServerSidePropStore->writeValue(std::move(powerPropValue), /*updateStatus=*/true,
                                                 VehiclePropertyStore::EventMode::ALWAYS);
            }
        }
    }
}

void FakeVehicleHardware::sendAdasPropertiesState(int32_t propertyId, int32_t state) {
    auto& adasDependentPropIds = mAdasEnabledPropToAdasPropWithErrorState.find(propertyId)->second;
    for (auto dependentPropId : adasDependentPropIds) {
        auto dependentPropConfigResult = mServerSidePropStore->getConfig(dependentPropId);
        if (!dependentPropConfigResult.ok()) {
            ALOGW("Failed to get config for ADAS property 0x%x, error: %s", dependentPropId,
                  getErrorMsg(dependentPropConfigResult).c_str());
            continue;
        }
        auto& dependentPropConfig = dependentPropConfigResult.value();
        for (auto& areaConfig : dependentPropConfig->areaConfigs) {
            auto propValue = createAdasStateReq(dependentPropId, areaConfig.areaId, state);
            // This will trigger a property change event for the current ADAS property value.
            mServerSidePropStore->writeValue(std::move(propValue), /*updateStatus=*/true,
                                             VehiclePropertyStore::EventMode::ALWAYS);
        }
    }
}

VhalResult<void> FakeVehicleHardware::maybeSetSpecialValue(const VehiclePropValue& value,
                                                           bool* isSpecialValue) {
    *isSpecialValue = false;
    VehiclePropValuePool::RecyclableType updatedValue;
    int32_t propId = value.prop;

    if (propId >= STARTING_VENDOR_CODE_PROPERTIES_FOR_TEST &&
        propId < ENDING_VENDOR_CODE_PROPERTIES_FOR_TEST) {
        *isSpecialValue = true;
        return {};
    }

    if (mFakeUserHal->isSupported(propId)) {
        *isSpecialValue = true;
        return setUserHalProp(value);
    }

    if (propId == toInt(VehicleProperty::HVAC_POWER_ON) && value.value.int32Values.size() == 1 &&
        value.value.int32Values[0] == 1) {
        // If we are turning HVAC power on, send current hvac property values through on change
        // event.
        sendHvacPropertiesCurrentValues(value.areaId);
    }

    if (isHvacPropAndHvacNotAvailable(propId, value.areaId)) {
        *isSpecialValue = true;
        return StatusError(StatusCode::NOT_AVAILABLE_DISABLED) << "hvac not available";
    }

    if (mAdasEnabledPropToAdasPropWithErrorState.count(propId) &&
        value.value.int32Values.size() == 1) {
        if (value.value.int32Values[0] == 1) {
            // Set default state to 1 when ADAS feature is enabled.
            sendAdasPropertiesState(propId, /* state = */ 1);
        } else {
            sendAdasPropertiesState(propId, toInt(ErrorState::NOT_AVAILABLE_DISABLED));
        }
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
        case VENDOR_PROPERTY_ID:
            *isSpecialValue = true;
            return StatusError((StatusCode)VENDOR_ERROR_CODE);
        case toInt(VehicleProperty::HVAC_TEMPERATURE_VALUE_SUGGESTION):
            *isSpecialValue = true;
            return setHvacTemperatureValueSuggestion(value);
        case toInt(VehicleProperty::LANE_CENTERING_ASSIST_COMMAND): {
            auto isAdasPropertyAvailableResult =
                    isAdasPropertyAvailable(toInt(VehicleProperty::LANE_CENTERING_ASSIST_STATE));
            if (!isAdasPropertyAvailableResult.ok()) {
                *isSpecialValue = true;
            }
            return isAdasPropertyAvailableResult;
        }
        case toInt(VehicleProperty::CRUISE_CONTROL_COMMAND):
            [[fallthrough]];
        case toInt(VehicleProperty::ADAPTIVE_CRUISE_CONTROL_TARGET_TIME_GAP): {
            auto isAdasPropertyAvailableResult =
                    isAdasPropertyAvailable(toInt(VehicleProperty::CRUISE_CONTROL_STATE));
            if (!isAdasPropertyAvailableResult.ok()) {
                *isSpecialValue = true;
            }
            return isAdasPropertyAvailableResult;
        }

#ifdef ENABLE_VEHICLE_HAL_TEST_PROPERTIES
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
                return StatusError(getErrorCode(writeResult))
                       << "failed to write value into property store, error: "
                       << getErrorMsg(writeResult);
            }
            return {};
#endif  // ENABLE_VEHICLE_HAL_TEST_PROPERTIES

        default:
            break;
    }
    return {};
}

StatusCode FakeVehicleHardware::setValues(std::shared_ptr<const SetValuesCallback> callback,
                                          const std::vector<SetValueRequest>& requests) {
    for (auto& request : requests) {
        if (FAKE_VEHICLEHARDWARE_DEBUG) {
            ALOGD("Set value for property ID: %d", request.value.prop);
        }

        // In a real VHAL implementation, you could either send the setValue request to vehicle bus
        // here in the binder thread, or you could send the request in setValue which runs in
        // the handler thread. If you decide to send the setValue request here, you should not
        // wait for the response here and the handler thread should handle the setValue response.
        mPendingSetValueRequests.addRequest(request, callback);
    }

    return StatusCode::OK;
}

VhalResult<void> FakeVehicleHardware::setValue(const VehiclePropValue& value) {
    // In a real VHAL implementation, this will send the request to vehicle bus if not already
    // sent in setValues, and wait for the response from vehicle bus.
    // Here we are just updating mValuePool.
    bool isSpecialValue = false;
    auto setSpecialValueResult = maybeSetSpecialValue(value, &isSpecialValue);
    if (isSpecialValue) {
        if (!setSpecialValueResult.ok()) {
            return StatusError(getErrorCode(setSpecialValueResult))
                   << StringPrintf("failed to set special value for property ID: %d, error: %s",
                                   value.prop, getErrorMsg(setSpecialValueResult).c_str());
        }
        return {};
    }

    auto updatedValue = mValuePool->obtain(value);
    int64_t timestamp = elapsedRealtimeNano();
    updatedValue->timestamp = timestamp;

    auto writeResult = mServerSidePropStore->writeValue(std::move(updatedValue));
    if (!writeResult.ok()) {
        return StatusError(getErrorCode(writeResult))
               << StringPrintf("failed to write value into property store, error: %s",
                               getErrorMsg(writeResult).c_str());
    }

    return {};
}

SetValueResult FakeVehicleHardware::handleSetValueRequest(const SetValueRequest& request) {
    SetValueResult setValueResult;
    setValueResult.requestId = request.requestId;

    if (auto result = setValue(request.value); !result.ok()) {
        ALOGE("failed to set value, error: %s, code: %d", getErrorMsg(result).c_str(),
              getIntErrorCode(result));
        setValueResult.status = getErrorCode(result);
    } else {
        setValueResult.status = StatusCode::OK;
    }

    return setValueResult;
}

StatusCode FakeVehicleHardware::getValues(std::shared_ptr<const GetValuesCallback> callback,
                                          const std::vector<GetValueRequest>& requests) const {
    for (auto& request : requests) {
        if (FAKE_VEHICLEHARDWARE_DEBUG) {
            ALOGD("getValues(%d)", request.prop.prop);
        }

        // In a real VHAL implementation, you could either send the getValue request to vehicle bus
        // here in the binder thread, or you could send the request in getValue which runs in
        // the handler thread. If you decide to send the getValue request here, you should not
        // wait for the response here and the handler thread should handle the getValue response.
        mPendingGetValueRequests.addRequest(request, callback);
    }

    return StatusCode::OK;
}

GetValueResult FakeVehicleHardware::handleGetValueRequest(const GetValueRequest& request) {
    GetValueResult getValueResult;
    getValueResult.requestId = request.requestId;

    auto result = getValue(request.prop);
    if (!result.ok()) {
        ALOGE("failed to get value, error: %s, code: %d", getErrorMsg(result).c_str(),
              getIntErrorCode(result));
        getValueResult.status = getErrorCode(result);
    } else {
        getValueResult.status = StatusCode::OK;
        getValueResult.prop = *result.value();
    }
    return getValueResult;
}

FakeVehicleHardware::ValueResultType FakeVehicleHardware::getValue(
        const VehiclePropValue& value) const {
    // In a real VHAL implementation, this will send the request to vehicle bus if not already
    // sent in getValues, and wait for the response from vehicle bus.
    // Here we are just reading value from mValuePool.
    bool isSpecialValue = false;
    auto result = maybeGetSpecialValue(value, &isSpecialValue);
    if (isSpecialValue) {
        if (!result.ok()) {
            return StatusError(getErrorCode(result))
                   << StringPrintf("failed to get special value: %d, error: %s", value.prop,
                                   getErrorMsg(result).c_str());
        } else {
            return std::move(result);
        }
    }

    auto readResult = mServerSidePropStore->readValue(value);
    if (!readResult.ok()) {
        StatusCode errorCode = getErrorCode(readResult);
        if (errorCode == StatusCode::NOT_AVAILABLE) {
            return StatusError(errorCode) << "value has not been set yet";
        } else {
            return StatusError(errorCode)
                   << "failed to get value, error: " << getErrorMsg(readResult);
        }
    }

    return std::move(readResult);
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
    } else if (EqualsIgnoreCase(option, "--getWithArg")) {
        result.buffer = dumpGetPropertyWithArg(options);
    } else if (EqualsIgnoreCase(option, "--set")) {
        result.buffer = dumpSetProperties(options);
    } else if (EqualsIgnoreCase(option, "--save-prop")) {
        result.buffer = dumpSaveProperty(options);
    } else if (EqualsIgnoreCase(option, "--restore-prop")) {
        result.buffer = dumpRestoreProperty(options);
    } else if (EqualsIgnoreCase(option, "--inject-event")) {
        result.buffer = dumpInjectEvent(options);
    } else if (EqualsIgnoreCase(option, kUserHalDumpOption)) {
        result.buffer = mFakeUserHal->dump();
    } else if (EqualsIgnoreCase(option, "--genfakedata")) {
        result.buffer = genFakeDataCommand(options);
    } else if (EqualsIgnoreCase(option, "--genTestVendorConfigs")) {
        mAddExtraTestVendorConfigs = true;
        result.refreshPropertyConfigs = true;
        result.buffer = "successfully generated vendor configs";
    } else if (EqualsIgnoreCase(option, "--restoreVendorConfigs")) {
        mAddExtraTestVendorConfigs = false;
        result.refreshPropertyConfigs = true;
        result.buffer = "successfully restored vendor configs";
    } else {
        result.buffer = StringPrintf("Invalid option: %s\n", option.c_str());
    }
    return result;
}

std::string FakeVehicleHardware::genFakeDataHelp() {
    return R"(
Generate Fake Data Usage:
--genfakedata --startlinear [propID] [mValue] [cValue] [dispersion] [increment] [interval]: "
Start a linear generator that generates event with floatValue within range:
[mValue - disperson, mValue + dispersion].
propID(int32): ID for the property to generate event for.
mValue(float): The middle of the possible values for the property.
cValue(float): The start value for the property, must be within the range.
dispersion(float): The range the value can change.
increment(float): The step the value would increase by for each generated event,
if exceed the range, the value would loop back.
interval(int64): The interval in nanoseconds the event would generate by.

--genfakedata --stoplinear [propID(int32)]: Stop a linear generator

--genfakedata --startjson --path [jsonFilePath] [repetition]:
Start a JSON generator that would generate events according to a JSON file.
jsonFilePath(string): The path to a JSON file. The JSON content must be in the format of
[{
    "timestamp": 1000000,
    "areaId": 0,
    "value": 8,
    "prop": 289408000
}, {...}]
Each event in the JSON file would be generated by the same interval their timestamp is relative to
the first event's timestamp.
repetition(int32, optional): how many iterations the events would be generated. If it is not
provided, it would iterate indefinitely.

--genfakedata --startjson --content [jsonContent]: Start a JSON generator using the content.

--genfakedata --stopjson [generatorID(string)]: Stop a JSON generator.

--genfakedata --keypress [keyCode(int32)] [display[int32]]: Generate key press.

--genfakedata --keyinputv2 [area(int32)] [display(int32)] [keyCode[int32]] [action[int32]]
  [repeatCount(int32)]

--genfakedata --motioninput [area(int32)] [display(int32)] [inputType[int32]] [action[int32]]
  [buttonState(int32)] --pointer [pointerId(int32)] [toolType(int32)] [xData(float)] [yData(float)]
  [pressure(float)] [size(float)]
  Generate a motion input event. --pointer option can be specified multiple times.

--genTestVendorConfigs: Generates fake VehiclePropConfig ranging from 0x5000 to 0x8000 all with
  vendor property group, global vehicle area, and int32 vehicle property type. This is mainly used
  for testing

--restoreVendorConfigs: Restores to to the default state if genTestVendorConfigs was used.
  Otherwise this will do nothing.

)";
}

std::string FakeVehicleHardware::parseErrMsg(std::string fieldName, std::string value,
                                             std::string type) {
    return StringPrintf("failed to parse %s as %s: \"%s\"\n%s", fieldName.c_str(), type.c_str(),
                        value.c_str(), genFakeDataHelp().c_str());
}

void FakeVehicleHardware::generateVendorConfigs(
        std::vector<VehiclePropConfig>& outAllConfigs) const {
    for (int i = STARTING_VENDOR_CODE_PROPERTIES_FOR_TEST;
         i < ENDING_VENDOR_CODE_PROPERTIES_FOR_TEST; i++) {
        VehiclePropConfig config;
        config.prop = i;
        config.access = VehiclePropertyAccess::READ_WRITE;
        outAllConfigs.push_back(config);
    }
}

std::string FakeVehicleHardware::genFakeDataCommand(const std::vector<std::string>& options) {
    if (options.size() < 2) {
        return "No subcommand specified for genfakedata\n" + genFakeDataHelp();
    }

    std::string command = options[1];
    if (command == "--startlinear") {
        // --genfakedata --startlinear [propID(int32)] [middleValue(float)]
        // [currentValue(float)] [dispersion(float)] [increment(float)] [interval(int64)]
        if (options.size() != 8) {
            return "incorrect argument count, need 8 arguments for --genfakedata --startlinear\n" +
                   genFakeDataHelp();
        }
        int32_t propId;
        float middleValue;
        float currentValue;
        float dispersion;
        float increment;
        int64_t interval;
        if (!android::base::ParseInt(options[2], &propId)) {
            return parseErrMsg("propId", options[2], "int");
        }
        if (!android::base::ParseFloat(options[3], &middleValue)) {
            return parseErrMsg("middleValue", options[3], "float");
        }
        if (!android::base::ParseFloat(options[4], &currentValue)) {
            return parseErrMsg("currentValue", options[4], "float");
        }
        if (!android::base::ParseFloat(options[5], &dispersion)) {
            return parseErrMsg("dispersion", options[5], "float");
        }
        if (!android::base::ParseFloat(options[6], &increment)) {
            return parseErrMsg("increment", options[6], "float");
        }
        if (!android::base::ParseInt(options[7], &interval)) {
            return parseErrMsg("interval", options[7], "int");
        }
        auto generator = std::make_unique<LinearFakeValueGenerator>(
                propId, middleValue, currentValue, dispersion, increment, interval);
        mGeneratorHub->registerGenerator(propId, std::move(generator));
        return "Linear event generator started successfully";
    } else if (command == "--stoplinear") {
        // --genfakedata --stoplinear [propID(int32)]
        if (options.size() != 3) {
            return "incorrect argument count, need 3 arguments for --genfakedata --stoplinear\n" +
                   genFakeDataHelp();
        }
        int32_t propId;
        if (!android::base::ParseInt(options[2], &propId)) {
            return parseErrMsg("propId", options[2], "int");
        }
        if (mGeneratorHub->unregisterGenerator(propId)) {
            return "Linear event generator stopped successfully";
        }
        return StringPrintf("No linear event generator found for property: %d", propId);
    } else if (command == "--startjson") {
        // --genfakedata --startjson --path path repetition
        // or
        // --genfakedata --startjson --content content repetition.
        if (options.size() != 4 && options.size() != 5) {
            return "incorrect argument count, need 4 or 5 arguments for --genfakedata "
                   "--startjson\n";
        }
        // Iterate infinitely if repetition number is not provided
        int32_t repetition = -1;
        if (options.size() == 5) {
            if (!android::base::ParseInt(options[4], &repetition)) {
                return parseErrMsg("repetition", options[4], "int");
            }
        }
        std::unique_ptr<JsonFakeValueGenerator> generator;
        if (options[2] == "--path") {
            const std::string& fileName = options[3];
            generator = std::make_unique<JsonFakeValueGenerator>(fileName, repetition);
            if (!generator->hasNext()) {
                return "invalid JSON file, no events";
            }
        } else if (options[2] == "--content") {
            const std::string& content = options[3];
            generator =
                    std::make_unique<JsonFakeValueGenerator>(/*unused=*/true, content, repetition);
            if (!generator->hasNext()) {
                return "invalid JSON content, no events";
            }
        }
        int32_t cookie = std::hash<std::string>()(options[3]);
        mGeneratorHub->registerGenerator(cookie, std::move(generator));
        return StringPrintf("JSON event generator started successfully, ID: %" PRId32, cookie);
    } else if (command == "--stopjson") {
        // --genfakedata --stopjson [generatorID(string)]
        if (options.size() != 3) {
            return "incorrect argument count, need 3 arguments for --genfakedata --stopjson\n";
        }
        int32_t cookie;
        if (!android::base::ParseInt(options[2], &cookie)) {
            return parseErrMsg("cookie", options[2], "int");
        }
        if (mGeneratorHub->unregisterGenerator(cookie)) {
            return "JSON event generator stopped successfully";
        } else {
            return StringPrintf("No JSON event generator found for ID: %s", options[2].c_str());
        }
    } else if (command == "--keypress") {
        int32_t keyCode;
        int32_t display;
        // --genfakedata --keypress [keyCode(int32)] [display[int32]]
        if (options.size() != 4) {
            return "incorrect argument count, need 4 arguments for --genfakedata --keypress\n";
        }
        if (!android::base::ParseInt(options[2], &keyCode)) {
            return parseErrMsg("keyCode", options[2], "int");
        }
        if (!android::base::ParseInt(options[3], &display)) {
            return parseErrMsg("display", options[3], "int");
        }
        // Send back to HAL
        onValueChangeCallback(
                createHwInputKeyProp(VehicleHwKeyInputAction::ACTION_DOWN, keyCode, display));
        onValueChangeCallback(
                createHwInputKeyProp(VehicleHwKeyInputAction::ACTION_UP, keyCode, display));
        return "keypress event generated successfully";
    } else if (command == "--keyinputv2") {
        int32_t area;
        int32_t display;
        int32_t keyCode;
        int32_t action;
        int32_t repeatCount;
        // --genfakedata --keyinputv2 [area(int32)] [display(int32)] [keyCode[int32]]
        // [action[int32]] [repeatCount(int32)]
        if (options.size() != 7) {
            return "incorrect argument count, need 7 arguments for --genfakedata --keyinputv2\n";
        }
        if (!android::base::ParseInt(options[2], &area)) {
            return parseErrMsg("area", options[2], "int");
        }
        if (!android::base::ParseInt(options[3], &display)) {
            return parseErrMsg("display", options[3], "int");
        }
        if (!android::base::ParseInt(options[4], &keyCode)) {
            return parseErrMsg("keyCode", options[4], "int");
        }
        if (!android::base::ParseInt(options[5], &action)) {
            return parseErrMsg("action", options[5], "int");
        }
        if (!android::base::ParseInt(options[6], &repeatCount)) {
            return parseErrMsg("repeatCount", options[6], "int");
        }
        // Send back to HAL
        onValueChangeCallback(createHwKeyInputV2Prop(area, display, keyCode, action, repeatCount));
        return StringPrintf(
                "keyinputv2 event generated successfully with area:%d, display:%d,"
                " keyCode:%d, action:%d, repeatCount:%d",
                area, display, keyCode, action, repeatCount);

    } else if (command == "--motioninput") {
        int32_t area;
        int32_t display;
        int32_t inputType;
        int32_t action;
        int32_t buttonState;
        int32_t pointerCount;

        // --genfakedata --motioninput [area(int32)] [display(int32)] [inputType[int32]]
        // [action[int32]] [buttonState(int32)] [pointerCount(int32)]
        // --pointer [pointerId(int32)] [toolType(int32)] [xData(float)] [yData(float)]
        // [pressure(float)] [size(float)]
        int optionsSize = (int)options.size();
        if (optionsSize / 7 < 2) {
            return "incorrect argument count, need at least 14 arguments for --genfakedata "
                   "--motioninput including at least 1 --pointer\n";
        }

        if (optionsSize % 7 != 0) {
            return "incorrect argument count, need 6 arguments for every --pointer\n";
        }
        pointerCount = (int)optionsSize / 7 - 1;

        if (!android::base::ParseInt(options[2], &area)) {
            return parseErrMsg("area", options[2], "int");
        }
        if (!android::base::ParseInt(options[3], &display)) {
            return parseErrMsg("display", options[3], "int");
        }
        if (!android::base::ParseInt(options[4], &inputType)) {
            return parseErrMsg("inputType", options[4], "int");
        }
        if (!android::base::ParseInt(options[5], &action)) {
            return parseErrMsg("action", options[5], "int");
        }
        if (!android::base::ParseInt(options[6], &buttonState)) {
            return parseErrMsg("buttonState", options[6], "int");
        }

        int32_t pointerId[pointerCount];
        int32_t toolType[pointerCount];
        float xData[pointerCount];
        float yData[pointerCount];
        float pressure[pointerCount];
        float size[pointerCount];

        for (int i = 7, pc = 0; i < optionsSize; i += 7, pc += 1) {
            int offset = i;
            if (options[offset] != "--pointer") {
                return "--pointer is needed for the motion input\n";
            }
            offset += 1;
            if (!android::base::ParseInt(options[offset], &pointerId[pc])) {
                return parseErrMsg("pointerId", options[offset], "int");
            }
            offset += 1;
            if (!android::base::ParseInt(options[offset], &toolType[pc])) {
                return parseErrMsg("toolType", options[offset], "int");
            }
            offset += 1;
            if (!android::base::ParseFloat(options[offset], &xData[pc])) {
                return parseErrMsg("xData", options[offset], "float");
            }
            offset += 1;
            if (!android::base::ParseFloat(options[offset], &yData[pc])) {
                return parseErrMsg("yData", options[offset], "float");
            }
            offset += 1;
            if (!android::base::ParseFloat(options[offset], &pressure[pc])) {
                return parseErrMsg("pressure", options[offset], "float");
            }
            offset += 1;
            if (!android::base::ParseFloat(options[offset], &size[pc])) {
                return parseErrMsg("size", options[offset], "float");
            }
        }

        // Send back to HAL
        onValueChangeCallback(createHwMotionInputProp(area, display, inputType, action, buttonState,
                                                      pointerCount, pointerId, toolType, xData,
                                                      yData, pressure, size));

        std::string successMessage = StringPrintf(
                "motion event generated successfully with area:%d, display:%d,"
                " inputType:%d, action:%d, buttonState:%d, pointerCount:%d\n",
                area, display, inputType, action, buttonState, pointerCount);
        for (int i = 0; i < pointerCount; i++) {
            successMessage += StringPrintf(
                    "Pointer #%d {\n"
                    " id:%d , tooltype:%d \n"
                    " x:%f , y:%f\n"
                    " pressure: %f, data: %f\n"
                    "}\n",
                    i, pointerId[i], toolType[i], xData[i], yData[i], pressure[i], size[i]);
        }
        return successMessage;
    }

    return StringPrintf("Unknown command: \"%s\"\n%s", command.c_str(), genFakeDataHelp().c_str());
}

VehiclePropValue FakeVehicleHardware::createHwInputKeyProp(VehicleHwKeyInputAction action,
                                                           int32_t keyCode, int32_t targetDisplay) {
    VehiclePropValue value = {
            .prop = toInt(VehicleProperty::HW_KEY_INPUT),
            .areaId = 0,
            .timestamp = elapsedRealtimeNano(),
            .status = VehiclePropertyStatus::AVAILABLE,
            .value.int32Values = {toInt(action), keyCode, targetDisplay},
    };
    return value;
}

VehiclePropValue FakeVehicleHardware::createHwKeyInputV2Prop(int32_t area, int32_t targetDisplay,
                                                             int32_t keyCode, int32_t action,
                                                             int32_t repeatCount) {
    VehiclePropValue value = {.prop = toInt(VehicleProperty::HW_KEY_INPUT_V2),
                              .areaId = area,
                              .timestamp = elapsedRealtimeNano(),
                              .status = VehiclePropertyStatus::AVAILABLE,
                              .value.int32Values = {targetDisplay, keyCode, action, repeatCount},
                              .value.int64Values = {elapsedRealtimeNano()}};
    return value;
}

VehiclePropValue FakeVehicleHardware::createHwMotionInputProp(
        int32_t area, int32_t display, int32_t inputType, int32_t action, int32_t buttonState,
        int32_t pointerCount, int32_t pointerId[], int32_t toolType[], float xData[], float yData[],
        float pressure[], float size[]) {
    std::vector<int> intValues;
    intValues.push_back(display);
    intValues.push_back(inputType);
    intValues.push_back(action);
    intValues.push_back(buttonState);
    intValues.push_back(pointerCount);
    for (int i = 0; i < pointerCount; i++) {
        intValues.push_back(pointerId[i]);
    }
    for (int i = 0; i < pointerCount; i++) {
        intValues.push_back(toolType[i]);
    }

    std::vector<float> floatValues;
    for (int i = 0; i < pointerCount; i++) {
        floatValues.push_back(xData[i]);
    }
    for (int i = 0; i < pointerCount; i++) {
        floatValues.push_back(yData[i]);
    }
    for (int i = 0; i < pointerCount; i++) {
        floatValues.push_back(pressure[i]);
    }
    for (int i = 0; i < pointerCount; i++) {
        floatValues.push_back(size[i]);
    }

    VehiclePropValue value = {.prop = toInt(VehicleProperty::HW_MOTION_INPUT),
                              .areaId = area,
                              .timestamp = elapsedRealtimeNano(),
                              .status = VehiclePropertyStatus::AVAILABLE,
                              .value.int32Values = intValues,
                              .value.floatValues = floatValues,
                              .value.int64Values = {elapsedRealtimeNano()}};
    return value;
}

void FakeVehicleHardware::eventFromVehicleBus(const VehiclePropValue& value) {
    mServerSidePropStore->writeValue(mValuePool->obtain(value));
}

std::string FakeVehicleHardware::dumpHelp() {
    return "Usage: \n\n"
           "[no args]: dumps (id and value) all supported properties \n"
           "--help: shows this help\n"
           "--list: lists the ids of all supported properties\n"
           "--get <PROP1> [PROP2] [PROPN]: dumps the value of specific properties. \n"
           "--getWithArg <PROP> [ValueArguments]: gets the value for a specific property with "
           "arguments. \n"
           "--set <PROP> [ValueArguments]: sets the value of property PROP. \n"
           "--save-prop <prop> [-a AREA_ID]: saves the current value for PROP, integration test"
           " that modifies prop value must call this before test and restore-prop after test. \n"
           "--restore-prop <prop> [-a AREA_ID]: restores a previously saved property value. \n"
           "--inject-event <PROP> [ValueArguments]: inject a property update event from car\n\n"
           "ValueArguments are in the format of [-i INT_VALUE [INT_VALUE ...]] "
           "[-i64 INT64_VALUE [INT64_VALUE ...]] [-f FLOAT_VALUE [FLOAT_VALUE ...]] [-s STR_VALUE] "
           "[-b BYTES_VALUE] [-a AREA_ID].\n"
           "Notice that the string, bytes and area value can be set just once, while the other can"
           " have multiple values (so they're used in the respective array), "
           "BYTES_VALUE is in the form of 0xXXXX, e.g. 0xdeadbeef.\n" +
           genFakeDataHelp() + "Fake user HAL usage: \n" + mFakeUserHal->showDumpHelp();
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

std::vector<std::string> FakeVehicleHardware::getOptionValues(
        const std::vector<std::string>& options, size_t* index) {
    std::vector<std::string> values;
    while (*index < options.size()) {
        std::string option = options[*index];
        if (SET_PROP_OPTIONS.find(option) != SET_PROP_OPTIONS.end()) {
            return std::move(values);
        }
        values.push_back(option);
        (*index)++;
    }
    return std::move(values);
}

Result<VehiclePropValue> FakeVehicleHardware::parsePropOptions(
        const std::vector<std::string>& options) {
    // Options format:
    // --set/get/inject-event PROP [-f f1 f2...] [-i i1 i2...] [-i64 i1 i2...] [-s s1 s2...]
    // [-b b1 b2...] [-a a] [-t timestamp]
    size_t optionIndex = 1;
    auto result = safelyParseInt<int32_t>(optionIndex, options[optionIndex]);
    if (!result.ok()) {
        return Error() << StringPrintf("Property value: \"%s\" is not a valid int: %s\n",
                                       options[optionIndex].c_str(), getErrorMsg(result).c_str());
    }
    VehiclePropValue prop = {};
    prop.prop = result.value();
    prop.status = VehiclePropertyStatus::AVAILABLE;
    optionIndex++;
    std::unordered_set<std::string> parsedOptions;

    while (optionIndex < options.size()) {
        std::string argType = options[optionIndex];
        optionIndex++;

        size_t currentIndex = optionIndex;
        std::vector<std::string> argValues = getOptionValues(options, &optionIndex);
        if (parsedOptions.find(argType) != parsedOptions.end()) {
            return Error() << StringPrintf("Duplicate \"%s\" options\n", argType.c_str());
        }
        parsedOptions.insert(argType);
        size_t argValuesSize = argValues.size();
        if (EqualsIgnoreCase(argType, "-i")) {
            if (argValuesSize == 0) {
                return Error() << "No values specified when using \"-i\"\n";
            }
            prop.value.int32Values.resize(argValuesSize);
            for (size_t i = 0; i < argValuesSize; i++) {
                auto int32Result = safelyParseInt<int32_t>(currentIndex + i, argValues[i]);
                if (!int32Result.ok()) {
                    return Error()
                           << StringPrintf("Value: \"%s\" is not a valid int: %s\n",
                                           argValues[i].c_str(), getErrorMsg(int32Result).c_str());
                }
                prop.value.int32Values[i] = int32Result.value();
            }
        } else if (EqualsIgnoreCase(argType, "-i64")) {
            if (argValuesSize == 0) {
                return Error() << "No values specified when using \"-i64\"\n";
            }
            prop.value.int64Values.resize(argValuesSize);
            for (size_t i = 0; i < argValuesSize; i++) {
                auto int64Result = safelyParseInt<int64_t>(currentIndex + i, argValues[i]);
                if (!int64Result.ok()) {
                    return Error()
                           << StringPrintf("Value: \"%s\" is not a valid int64: %s\n",
                                           argValues[i].c_str(), getErrorMsg(int64Result).c_str());
                }
                prop.value.int64Values[i] = int64Result.value();
            }
        } else if (EqualsIgnoreCase(argType, "-f")) {
            if (argValuesSize == 0) {
                return Error() << "No values specified when using \"-f\"\n";
            }
            prop.value.floatValues.resize(argValuesSize);
            for (size_t i = 0; i < argValuesSize; i++) {
                auto floatResult = safelyParseFloat(currentIndex + i, argValues[i]);
                if (!floatResult.ok()) {
                    return Error()
                           << StringPrintf("Value: \"%s\" is not a valid float: %s\n",
                                           argValues[i].c_str(), getErrorMsg(floatResult).c_str());
                }
                prop.value.floatValues[i] = floatResult.value();
            }
        } else if (EqualsIgnoreCase(argType, "-s")) {
            if (argValuesSize != 1) {
                return Error() << "Expect exact one value when using \"-s\"\n";
            }
            prop.value.stringValue = argValues[0];
        } else if (EqualsIgnoreCase(argType, "-b")) {
            if (argValuesSize != 1) {
                return Error() << "Expect exact one value when using \"-b\"\n";
            }
            auto bytesResult = parseHexString(argValues[0]);
            if (!bytesResult.ok()) {
                return Error() << StringPrintf("value: \"%s\" is not a valid hex string: %s\n",
                                               argValues[0].c_str(),
                                               getErrorMsg(bytesResult).c_str());
            }
            prop.value.byteValues = std::move(bytesResult.value());
        } else if (EqualsIgnoreCase(argType, "-a")) {
            if (argValuesSize != 1) {
                return Error() << "Expect exact one value when using \"-a\"\n";
            }
            auto int32Result = safelyParseInt<int32_t>(currentIndex, argValues[0]);
            if (!int32Result.ok()) {
                return Error() << StringPrintf("Area ID: \"%s\" is not a valid int: %s\n",
                                               argValues[0].c_str(),
                                               getErrorMsg(int32Result).c_str());
            }
            prop.areaId = int32Result.value();
        } else if (EqualsIgnoreCase(argType, "-t")) {
            if (argValuesSize != 1) {
                return Error() << "Expect exact one value when using \"-t\"\n";
            }
            auto int64Result = safelyParseInt<int64_t>(currentIndex, argValues[0]);
            if (!int64Result.ok()) {
                return Error() << StringPrintf("Timestamp: \"%s\" is not a valid int64: %s\n",
                                               argValues[0].c_str(),
                                               getErrorMsg(int64Result).c_str());
            }
            prop.timestamp = int64Result.value();
        } else {
            return Error() << StringPrintf("Unknown option: %s\n", argType.c_str());
        }
    }

    return prop;
}

std::string FakeVehicleHardware::dumpSetProperties(const std::vector<std::string>& options) {
    if (auto result = checkArgumentsSize(options, 3); !result.ok()) {
        return getErrorMsg(result);
    }

    auto parseResult = parsePropOptions(options);
    if (!parseResult.ok()) {
        return getErrorMsg(parseResult);
    }
    VehiclePropValue prop = std::move(parseResult.value());
    ALOGD("Dump: Setting property: %s", prop.toString().c_str());

    bool isSpecialValue = false;
    auto setResult = maybeSetSpecialValue(prop, &isSpecialValue);

    if (!isSpecialValue) {
        auto updatedValue = mValuePool->obtain(prop);
        updatedValue->timestamp = elapsedRealtimeNano();
        setResult = mServerSidePropStore->writeValue(std::move(updatedValue));
    }

    if (setResult.ok()) {
        return StringPrintf("Set property: %s\n", prop.toString().c_str());
    }
    return StringPrintf("failed to set property: %s, error: %s\n", prop.toString().c_str(),
                        getErrorMsg(setResult).c_str());
}

std::string FakeVehicleHardware::dumpGetPropertyWithArg(const std::vector<std::string>& options) {
    if (auto result = checkArgumentsSize(options, 3); !result.ok()) {
        return getErrorMsg(result);
    }

    auto parseResult = parsePropOptions(options);
    if (!parseResult.ok()) {
        return getErrorMsg(parseResult);
    }
    VehiclePropValue prop = std::move(parseResult.value());
    ALOGD("Dump: Getting property: %s", prop.toString().c_str());

    bool isSpecialValue = false;
    auto result = maybeGetSpecialValue(prop, &isSpecialValue);

    if (!isSpecialValue) {
        result = mServerSidePropStore->readValue(prop);
    }

    if (!result.ok()) {
        return StringPrintf("failed to read property value: %d, error: %s, code: %d\n", prop.prop,
                            getErrorMsg(result).c_str(), getIntErrorCode(result));
    }
    return StringPrintf("Get property result: %s\n", result.value()->toString().c_str());
}

std::string FakeVehicleHardware::dumpSaveProperty(const std::vector<std::string>& options) {
    // Format: --save-prop PROP [-a areaID]
    if (auto result = checkArgumentsSize(options, 2); !result.ok()) {
        return getErrorMsg(result);
    }

    auto parseResult = parsePropOptions(options);
    if (!parseResult.ok()) {
        return getErrorMsg(parseResult);
    }
    // We are only using the prop and areaId option.
    VehiclePropValue value = std::move(parseResult.value());
    int32_t propId = value.prop;
    int32_t areaId = value.areaId;

    auto readResult = mServerSidePropStore->readValue(value);
    if (!readResult.ok()) {
        return StringPrintf("Failed to save current property value, error: %s",
                            getErrorMsg(readResult).c_str());
    }

    std::scoped_lock<std::mutex> lockGuard(mLock);
    mSavedProps[PropIdAreaId{
            .propId = propId,
            .areaId = areaId,
    }] = std::move(readResult.value());

    return StringPrintf("Property: %" PRId32 ", areaID: %" PRId32 " saved", propId, areaId);
}

std::string FakeVehicleHardware::dumpRestoreProperty(const std::vector<std::string>& options) {
    // Format: --restore-prop PROP [-a areaID]
    if (auto result = checkArgumentsSize(options, 2); !result.ok()) {
        return getErrorMsg(result);
    }

    auto parseResult = parsePropOptions(options);
    if (!parseResult.ok()) {
        return getErrorMsg(parseResult);
    }
    // We are only using the prop and areaId option.
    VehiclePropValue value = std::move(parseResult.value());
    int32_t propId = value.prop;
    int32_t areaId = value.areaId;
    VehiclePropValuePool::RecyclableType savedValue;

    {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        auto it = mSavedProps.find(PropIdAreaId{
                .propId = propId,
                .areaId = areaId,
        });
        if (it == mSavedProps.end()) {
            return StringPrintf("No saved property for property: %" PRId32 ", areaID: %" PRId32,
                                propId, areaId);
        }

        savedValue = std::move(it->second);
        // Remove the saved property after restoring it.
        mSavedProps.erase(it);
    }

    // Update timestamp.
    savedValue->timestamp = elapsedRealtimeNano();

    auto writeResult = mServerSidePropStore->writeValue(std::move(savedValue));
    if (!writeResult.ok()) {
        return StringPrintf("Failed to restore property value, error: %s",
                            getErrorMsg(writeResult).c_str());
    }

    return StringPrintf("Property: %" PRId32 ", areaID: %" PRId32 " restored", propId, areaId);
}

std::string FakeVehicleHardware::dumpInjectEvent(const std::vector<std::string>& options) {
    if (auto result = checkArgumentsSize(options, 3); !result.ok()) {
        return getErrorMsg(result);
    }

    auto parseResult = parsePropOptions(options);
    if (!parseResult.ok()) {
        return getErrorMsg(parseResult);
    }
    VehiclePropValue prop = std::move(parseResult.value());
    ALOGD("Dump: Injecting event from vehicle bus: %s", prop.toString().c_str());

    eventFromVehicleBus(prop);

    return StringPrintf("Event for property: %d injected", prop.prop);
}

StatusCode FakeVehicleHardware::checkHealth() {
    // Always return OK for checkHealth.
    return StatusCode::OK;
}

void FakeVehicleHardware::registerOnPropertyChangeEvent(
        std::unique_ptr<const PropertyChangeCallback> callback) {
    if (mOnPropertyChangeCallback != nullptr) {
        ALOGE("registerOnPropertyChangeEvent must only be called once");
        return;
    }
    mOnPropertyChangeCallback = std::move(callback);
}

void FakeVehicleHardware::registerOnPropertySetErrorEvent(
        std::unique_ptr<const PropertySetErrorCallback> callback) {
    // In FakeVehicleHardware, we will never use mOnPropertySetErrorCallback.
    if (mOnPropertySetErrorCallback != nullptr) {
        ALOGE("registerOnPropertySetErrorEvent must only be called once");
        return;
    }
    mOnPropertySetErrorCallback = std::move(callback);
}

StatusCode FakeVehicleHardware::updateSampleRate(int32_t propId, int32_t areaId, float sampleRate) {
    // DefaultVehicleHal makes sure that sampleRate must be within minSampleRate and maxSampleRate.
    // For fake implementation, we would write the same value with a new timestamp into propStore
    // at sample rate.
    std::scoped_lock<std::mutex> lockGuard(mLock);

    PropIdAreaId propIdAreaId{
            .propId = propId,
            .areaId = areaId,
    };
    if (mRecurrentActions.find(propIdAreaId) != mRecurrentActions.end()) {
        mRecurrentTimer->unregisterTimerCallback(mRecurrentActions[propIdAreaId]);
    }
    if (sampleRate == 0) {
        return StatusCode::OK;
    }
    int64_t interval = static_cast<int64_t>(1'000'000'000. / sampleRate);
    auto action = std::make_shared<RecurrentTimer::Callback>([this, propId, areaId] {
        // Refresh the property value. In real implementation, this should poll the latest value
        // from vehicle bus. Here, we are just refreshing the existing value with a new timestamp.
        auto result = getValue(VehiclePropValue{
                .prop = propId,
                .areaId = areaId,
        });
        if (!result.ok()) {
            // Failed to read current value, skip refreshing.
            return;
        }
        result.value()->timestamp = elapsedRealtimeNano();
        // For continuous properties, we must generate a new onPropertyChange event periodically
        // according to the sample rate.
        mServerSidePropStore->writeValue(std::move(result.value()), /*updateStatus=*/true,
                                         VehiclePropertyStore::EventMode::ALWAYS);
    });
    mRecurrentTimer->registerTimerCallback(interval, action);
    mRecurrentActions[propIdAreaId] = action;
    return StatusCode::OK;
}

void FakeVehicleHardware::onValueChangeCallback(const VehiclePropValue& value) {
    if (mOnPropertyChangeCallback == nullptr) {
        return;
    }

    std::vector<VehiclePropValue> updatedValues;
    updatedValues.push_back(value);
    (*mOnPropertyChangeCallback)(std::move(updatedValues));
}

void FakeVehicleHardware::loadPropConfigsFromDir(
        const std::string& dirPath,
        std::unordered_map<int32_t, ConfigDeclaration>* configsByPropId) {
    ALOGI("loading properties from %s", dirPath.c_str());
    if (auto dir = opendir(dirPath.c_str()); dir != NULL) {
        std::regex regJson(".*[.]json", std::regex::icase);
        while (auto f = readdir(dir)) {
            if (!std::regex_match(f->d_name, regJson)) {
                continue;
            }
            std::string filePath = dirPath + "/" + std::string(f->d_name);
            ALOGI("loading properties from %s", filePath.c_str());
            auto result = mLoader.loadPropConfig(filePath);
            if (!result.ok()) {
                ALOGE("failed to load config file: %s, error: %s", filePath.c_str(),
                      result.error().message().c_str());
                continue;
            }
            for (auto& [propId, configDeclaration] : result.value()) {
                (*configsByPropId)[propId] = std::move(configDeclaration);
            }
        }
        closedir(dir);
    }
}

Result<float> FakeVehicleHardware::safelyParseFloat(int index, const std::string& s) {
    float out;
    if (!ParseFloat(s, &out)) {
        return Error() << StringPrintf("non-float argument at index %d: %s\n", index, s.c_str());
    }
    return out;
}

Result<std::vector<uint8_t>> FakeVehicleHardware::parseHexString(const std::string& s) {
    std::vector<uint8_t> bytes;
    if (s.size() % 2 != 0) {
        return Error() << StringPrintf("invalid hex string: %s, should have even size\n",
                                       s.c_str());
    }
    if (!StartsWith(s, "0x")) {
        return Error() << StringPrintf("hex string should start with \"0x\", got %s\n", s.c_str());
    }
    std::string subs = s.substr(2);
    std::transform(subs.begin(), subs.end(), subs.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    bool highDigit = true;
    for (size_t i = 0; i < subs.size(); i++) {
        char c = subs[i];
        uint8_t v;
        if (c >= '0' && c <= '9') {
            v = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            v = c - 'a' + 10;
        } else {
            return Error() << StringPrintf("invalid character %c in hex string %s\n", c,
                                           subs.c_str());
        }
        if (highDigit) {
            bytes.push_back(v * 16);
        } else {
            bytes[bytes.size() - 1] += v;
        }
        highDigit = !highDigit;
    }
    return bytes;
}

template <class CallbackType, class RequestType>
FakeVehicleHardware::PendingRequestHandler<CallbackType, RequestType>::PendingRequestHandler(
        FakeVehicleHardware* hardware)
    : mHardware(hardware) {
    // Don't initialize mThread in initialization list because mThread depends on mRequests and we
    // want mRequests to be initialized first.
    mThread = std::thread([this] {
        while (mRequests.waitForItems()) {
            handleRequestsOnce();
        }
    });
}

template <class CallbackType, class RequestType>
void FakeVehicleHardware::PendingRequestHandler<CallbackType, RequestType>::addRequest(
        RequestType request, std::shared_ptr<const CallbackType> callback) {
    mRequests.push({
            request,
            callback,
    });
}

template <class CallbackType, class RequestType>
void FakeVehicleHardware::PendingRequestHandler<CallbackType, RequestType>::stop() {
    mRequests.deactivate();
    if (mThread.joinable()) {
        mThread.join();
    }
}

template <>
void FakeVehicleHardware::PendingRequestHandler<FakeVehicleHardware::GetValuesCallback,
                                                GetValueRequest>::handleRequestsOnce() {
    std::unordered_map<std::shared_ptr<const GetValuesCallback>, std::vector<GetValueResult>>
            callbackToResults;
    for (const auto& rwc : mRequests.flush()) {
        ATRACE_BEGIN("FakeVehicleHardware:handleGetValueRequest");
        auto result = mHardware->handleGetValueRequest(rwc.request);
        ATRACE_END();
        callbackToResults[rwc.callback].push_back(std::move(result));
    }
    for (const auto& [callback, results] : callbackToResults) {
        ATRACE_BEGIN("FakeVehicleHardware:call get value result callback");
        (*callback)(std::move(results));
        ATRACE_END();
    }
}

template <>
void FakeVehicleHardware::PendingRequestHandler<FakeVehicleHardware::SetValuesCallback,
                                                SetValueRequest>::handleRequestsOnce() {
    std::unordered_map<std::shared_ptr<const SetValuesCallback>, std::vector<SetValueResult>>
            callbackToResults;
    for (const auto& rwc : mRequests.flush()) {
        ATRACE_BEGIN("FakeVehicleHardware:handleSetValueRequest");
        auto result = mHardware->handleSetValueRequest(rwc.request);
        ATRACE_END();
        callbackToResults[rwc.callback].push_back(std::move(result));
    }
    for (const auto& [callback, results] : callbackToResults) {
        ATRACE_BEGIN("FakeVehicleHardware:call set value result callback");
        (*callback)(std::move(results));
        ATRACE_END();
    }
}

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
