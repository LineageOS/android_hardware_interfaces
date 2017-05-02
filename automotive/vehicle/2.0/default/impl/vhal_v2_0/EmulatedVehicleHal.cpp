/*
 * Copyright (C) 2016 The Android Open Source Project
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
#define LOG_TAG "DefaultVehicleHal_v2_0"

#include <android/log.h>
#include <android-base/macros.h>

#include "EmulatedVehicleHal.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

enum class FakeDataCommand : int32_t {
    Stop = 0,
    Start = 1,
};

EmulatedVehicleHal::EmulatedVehicleHal(VehiclePropertyStore* propStore)
    : mPropStore(propStore),
      mHvacPowerProps(std::begin(kHvacPowerProperties), std::end(kHvacPowerProperties)),
      mRecurrentTimer(std::bind(&EmulatedVehicleHal::onContinuousPropertyTimer,
                                  this, std::placeholders::_1)),
      mFakeValueGenerator(std::bind(&EmulatedVehicleHal::onFakeValueGenerated,
                                    this, std::placeholders::_1, std::placeholders::_2)) {

    for (size_t i = 0; i < arraysize(kVehicleProperties); i++) {
        mPropStore->registerProperty(kVehicleProperties[i].config);
    }
}

VehicleHal::VehiclePropValuePtr EmulatedVehicleHal::get(
        const VehiclePropValue& requestedPropValue, StatusCode* outStatus) {
    VehiclePropValuePtr v = nullptr;

    auto internalPropValue = mPropStore->readValueOrNull(requestedPropValue);
    if (internalPropValue != nullptr) {
        v = getValuePool()->obtain(*internalPropValue);
    }

    *outStatus = v != nullptr ? StatusCode::OK : StatusCode::INVALID_ARG;
    return v;
}

StatusCode EmulatedVehicleHal::set(const VehiclePropValue& propValue) {
    if (propValue.prop == kGenerateFakeDataControllingProperty) {
        return handleGenerateFakeDataRequest(propValue);
    };

    if (mHvacPowerProps.count(propValue.prop)) {
        auto hvacPowerOn = mPropStore->readValueOrNull(toInt(VehicleProperty::HVAC_POWER_ON),
                                                      toInt(VehicleAreaZone::ROW_1));

        if (hvacPowerOn && hvacPowerOn->value.int32Values.size() == 1
                && hvacPowerOn->value.int32Values[0] == 0) {
            return StatusCode::NOT_AVAILABLE;
        }
    }

    if (!mPropStore->writeValue(propValue)) {
        return StatusCode::INVALID_ARG;
    }

    getEmulatorOrDie()->doSetValueFromClient(propValue);

    return StatusCode::OK;
}

// Parse supported properties list and generate vector of property values to hold current values.
void EmulatedVehicleHal::onCreate() {
    for (auto& it : kVehicleProperties) {
        VehiclePropConfig cfg = it.config;
        int32_t supportedAreas = cfg.supportedAreas;

        //  A global property will have supportedAreas = 0
        if (isGlobalProp(cfg.prop)) {
            supportedAreas = 0;
        }

        // This loop is a do-while so it executes at least once to handle global properties
        do {
            int32_t curArea = supportedAreas;
            supportedAreas &= supportedAreas - 1;  // Clear the right-most bit of supportedAreas.
            curArea ^= supportedAreas;  // Set curArea to the previously cleared bit.

            // Create a separate instance for each individual zone
            VehiclePropValue prop = {
                .prop = cfg.prop,
                .areaId = curArea,
            };
            if (it.initialAreaValues.size() > 0) {
                auto valueForAreaIt = it.initialAreaValues.find(curArea);
                if (valueForAreaIt != it.initialAreaValues.end()) {
                    prop.value = valueForAreaIt->second;
                } else {
                    ALOGW("%s failed to get default value for prop 0x%x area 0x%x",
                            __func__, cfg.prop, curArea);
                }
            } else {
                prop.value = it.initialValue;
            }
            mPropStore->writeValue(prop);

        } while (supportedAreas != 0);
    }
}

std::vector<VehiclePropConfig> EmulatedVehicleHal::listProperties()  {
    return mPropStore->getAllConfigs();
}

void EmulatedVehicleHal::onContinuousPropertyTimer(const std::vector<int32_t>& properties) {
    VehiclePropValuePtr v;

    auto& pool = *getValuePool();

    for (int32_t property : properties) {
        if (isContinuousProperty(property)) {
            auto internalPropValue = mPropStore->readValueOrNull(property);
            if (internalPropValue != nullptr) {
                v = pool.obtain(*internalPropValue);
            }
        } else {
            ALOGE("Unexpected onContinuousPropertyTimer for property: 0x%x", property);
        }

        if (v.get()) {
            v->timestamp = elapsedRealtimeNano();
            doHalEvent(std::move(v));
        }
    }
}

StatusCode EmulatedVehicleHal::subscribe(int32_t property, int32_t,
                                        float sampleRate) {
    ALOGI("%s propId: 0x%x, sampleRate: %f", __func__, property, sampleRate);

    if (isContinuousProperty(property)) {
        mRecurrentTimer.registerRecurrentEvent(hertzToNanoseconds(sampleRate), property);
    }
    return StatusCode::OK;
}

StatusCode EmulatedVehicleHal::unsubscribe(int32_t property) {
    ALOGI("%s propId: 0x%x", __func__, property);
    if (isContinuousProperty(property)) {
        mRecurrentTimer.unregisterRecurrentEvent(property);
    }
    return StatusCode::OK;
}

bool EmulatedVehicleHal::isContinuousProperty(int32_t propId) const {
    const VehiclePropConfig* config = mPropStore->getConfigOrNull(propId);
    if (config == nullptr) {
        ALOGW("Config not found for property: 0x%x", propId);
        return false;
    }
    return config->changeMode == VehiclePropertyChangeMode::CONTINUOUS;
}

bool EmulatedVehicleHal::setPropertyFromVehicle(const VehiclePropValue& propValue) {
    if (mPropStore->writeValue(propValue)) {
        doHalEvent(getValuePool()->obtain(propValue));
        return true;
    } else {
        return false;
    }
}

std::vector<VehiclePropValue> EmulatedVehicleHal::getAllProperties() const  {
    return mPropStore->readAllValues();
}

StatusCode EmulatedVehicleHal::handleGenerateFakeDataRequest(const VehiclePropValue& request) {
    ALOGI("%s", __func__);
    const auto& v = request.value;
    if (v.int32Values.size() < 2) {
        ALOGE("%s: expected at least 2 elements in int32Values, got: %zu", __func__,
                v.int32Values.size());
        return StatusCode::INVALID_ARG;
    }

    FakeDataCommand command = static_cast<FakeDataCommand>(v.int32Values[0]);
    int32_t propId = v.int32Values[1];

    switch (command) {
        case FakeDataCommand::Start: {
            if (!v.int64Values.size()) {
                ALOGE("%s: interval is not provided in int64Values", __func__);
                return StatusCode::INVALID_ARG;
            }
            auto interval = std::chrono::nanoseconds(v.int64Values[0]);

            if (v.floatValues.size() < 3) {
                ALOGE("%s: expected at least 3 element sin floatValues, got: %zu", __func__,
                        v.floatValues.size());
                return StatusCode::INVALID_ARG;
            }
            float initialValue = v.floatValues[0];
            float dispersion = v.floatValues[1];
            float increment = v.floatValues[2];

            ALOGI("%s, propId: %d, initalValue: %f", __func__, propId, initialValue);
            mFakeValueGenerator.startGeneratingHalEvents(
                interval, propId, initialValue, dispersion, increment);

            break;
        }
        case FakeDataCommand::Stop: {
            ALOGI("%s, FakeDataCommandStop", __func__);
            mFakeValueGenerator.stopGeneratingHalEvents(propId);
            break;
        }
        default: {
            ALOGE("%s: unexpected command: %d", __func__, command);
            return StatusCode::INVALID_ARG;
        }
    }
    return StatusCode::OK;
}

void EmulatedVehicleHal::onFakeValueGenerated(int32_t propId, float value) {
    VehiclePropValuePtr updatedPropValue {};
    switch (getPropType(propId)) {
        case VehiclePropertyType::FLOAT:
            updatedPropValue = getValuePool()->obtainFloat(value);
            break;
        case VehiclePropertyType::INT32:
            updatedPropValue = getValuePool()->obtainInt32(static_cast<int32_t>(value));
            break;
        default:
            ALOGE("%s: data type for property: 0x%x not supported", __func__, propId);
            return;

    }

    if (updatedPropValue) {
        updatedPropValue->prop = propId;
        updatedPropValue->areaId = 0;  // Add area support if necessary.
        updatedPropValue->timestamp = elapsedRealtimeNano();
        mPropStore->writeValue(*updatedPropValue);
        auto changeMode = mPropStore->getConfigOrDie(propId)->changeMode;
        if (VehiclePropertyChangeMode::ON_CHANGE == changeMode) {
            doHalEvent(move(updatedPropValue));
        }
    }
}

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
