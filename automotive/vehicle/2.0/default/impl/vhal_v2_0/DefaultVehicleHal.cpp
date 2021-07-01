/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <utils/Log.h>
#include <utils/SystemClock.h>
#include <vhal_v2_0/RecurrentTimer.h>

#include "VehicleUtils.h"

#include "DefaultVehicleHal.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

DefaultVehicleHal::DefaultVehicleHal(VehiclePropertyStore* propStore, VehicleHalClient* client)
    : mPropStore(propStore), mRecurrentTimer(getTimerAction()), mVehicleClient(client) {
    initStaticConfig();
    mVehicleClient->registerPropertyValueCallback(
            [this](const VehiclePropValue& value, bool updateStatus) {
                onPropertyValue(value, updateStatus);
            });
}

VehicleHal::VehiclePropValuePtr DefaultVehicleHal::get(const VehiclePropValue& requestedPropValue,
                                                       StatusCode* outStatus) {
    auto propId = requestedPropValue.prop;
    ALOGV("get(%d)", propId);

    VehiclePropValuePtr v = nullptr;
    auto internalPropValue = mPropStore->readValueOrNull(requestedPropValue);
    if (internalPropValue != nullptr) {
        v = getValuePool()->obtain(*internalPropValue);
    }

    if (!v) {
        *outStatus = StatusCode::INVALID_ARG;
    } else if (v->status == VehiclePropertyStatus::AVAILABLE) {
        *outStatus = StatusCode::OK;
    } else {
        *outStatus = StatusCode::TRY_AGAIN;
    }
    if (v.get()) {
        v->timestamp = elapsedRealtimeNano();
    }
    return v;
}

std::vector<VehiclePropConfig> DefaultVehicleHal::listProperties() {
    return mPropStore->getAllConfigs();
}

bool DefaultVehicleHal::dump(const hidl_handle& fd, const hidl_vec<hidl_string>& options) {
    return mVehicleClient->dump(fd, options);
}

StatusCode DefaultVehicleHal::set(const VehiclePropValue& propValue) {
    if (propValue.status != VehiclePropertyStatus::AVAILABLE) {
        // Android side cannot set property status - this value is the
        // purview of the HAL implementation to reflect the state of
        // its underlying hardware
        return StatusCode::INVALID_ARG;
    }
    auto currentPropValue = mPropStore->readValueOrNull(propValue);

    if (currentPropValue == nullptr) {
        return StatusCode::INVALID_ARG;
    }
    if (currentPropValue->status != VehiclePropertyStatus::AVAILABLE) {
        // do not allow Android side to set() a disabled/error property
        return StatusCode::NOT_AVAILABLE;
    }

    // Send the value to the vehicle server, the server will talk to the (real or emulated) car
    return mVehicleClient->setProperty(propValue, /*updateStatus=*/false);
}

// Parse supported properties list and generate vector of property values to hold current values.
void DefaultVehicleHal::onCreate() {
    auto configs = mVehicleClient->getAllPropertyConfig();

    for (const auto& cfg : configs) {
        int32_t numAreas = isGlobalProp(cfg.prop) ? 0 : cfg.areaConfigs.size();

        for (int i = 0; i < numAreas; i++) {
            int32_t curArea = isGlobalProp(cfg.prop) ? 0 : cfg.areaConfigs[i].areaId;

            // Create a separate instance for each individual zone
            VehiclePropValue prop = {
                    .areaId = curArea,
                    .prop = cfg.prop,
                    .status = VehiclePropertyStatus::UNAVAILABLE,
            };
            // Allow the initial values to set status.
            mPropStore->writeValue(prop, /*updateStatus=*/true);
        }
    }

    mVehicleClient->triggerSendAllValues();
}

void DefaultVehicleHal::onContinuousPropertyTimer(const std::vector<int32_t>& properties) {
    auto& pool = *getValuePool();

    for (int32_t property : properties) {
        VehiclePropValuePtr v;
        if (isContinuousProperty(property)) {
            auto internalPropValue = mPropStore->readValueOrNull(property);
            if (internalPropValue != nullptr) {
                v = pool.obtain(*internalPropValue);
            }
        } else {
            ALOGE("Unexpected onContinuousPropertyTimer for property: 0x%x", property);
            continue;
        }

        if (v.get()) {
            v->timestamp = elapsedRealtimeNano();
            doHalEvent(std::move(v));
        }
    }
}

RecurrentTimer::Action DefaultVehicleHal::getTimerAction() {
    return [this](const std::vector<int32_t>& properties) {
        onContinuousPropertyTimer(properties);
    };
}

StatusCode DefaultVehicleHal::subscribe(int32_t property, float sampleRate) {
    ALOGI("%s propId: 0x%x, sampleRate: %f", __func__, property, sampleRate);

    if (!isContinuousProperty(property)) {
        return StatusCode::INVALID_ARG;
    }
    mRecurrentTimer.registerRecurrentEvent(hertzToNanoseconds(sampleRate), property);
    return StatusCode::OK;
}

StatusCode DefaultVehicleHal::unsubscribe(int32_t property) {
    ALOGI("%s propId: 0x%x", __func__, property);
    if (!isContinuousProperty(property)) {
        return StatusCode::INVALID_ARG;
    }
    // If the event was not registered before, this would do nothing.
    mRecurrentTimer.unregisterRecurrentEvent(property);
    return StatusCode::OK;
}

bool DefaultVehicleHal::isContinuousProperty(int32_t propId) const {
    const VehiclePropConfig* config = mPropStore->getConfigOrNull(propId);
    if (config == nullptr) {
        ALOGW("Config not found for property: 0x%x", propId);
        return false;
    }
    return config->changeMode == VehiclePropertyChangeMode::CONTINUOUS;
}

void DefaultVehicleHal::onPropertyValue(const VehiclePropValue& value, bool updateStatus) {
    VehiclePropValuePtr updatedPropValue = getValuePool()->obtain(value);

    if (mPropStore->writeValue(*updatedPropValue, updateStatus)) {
        doHalEvent(std::move(updatedPropValue));
    }
}

void DefaultVehicleHal::initStaticConfig() {
    auto configs = mVehicleClient->getAllPropertyConfig();
    for (auto&& cfg : configs) {
        mPropStore->registerProperty(cfg, nullptr);
    }
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
