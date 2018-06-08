/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "LinearFakeValueGenerator"

#include <log/log.h>
#include <vhal_v2_0/VehicleUtils.h>

#include "LinearFakeValueGenerator.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

LinearFakeValueGenerator::LinearFakeValueGenerator(const OnHalEvent& onHalEvent)
    : mOnHalEvent(onHalEvent),
      mRecurrentTimer(std::bind(&LinearFakeValueGenerator::onTimer, this, std::placeholders::_1)) {}

StatusCode LinearFakeValueGenerator::start(const VehiclePropValue& request) {
    const auto& v = request.value;
    if (v.int32Values.size() < 2) {
        ALOGE("%s: expected property ID in int32Values", __func__);
        return StatusCode::INVALID_ARG;
    }
    int32_t propId = v.int32Values[1];

    if (!v.int64Values.size()) {
        ALOGE("%s: interval is not provided in int64Values", __func__);
        return StatusCode::INVALID_ARG;
    }
    auto interval = std::chrono::nanoseconds(v.int64Values[0]);

    if (v.floatValues.size() < 3) {
        ALOGE("%s: expected at least 3 elements in floatValues, got: %zu", __func__,
              v.floatValues.size());
        return StatusCode::INVALID_ARG;
    }
    float initialValue = v.floatValues[0];
    float dispersion = v.floatValues[1];
    float increment = v.floatValues[2];

    MuxGuard g(mLock);
    removeLocked(propId);
    mGenCfg.insert({propId, GeneratorCfg{
                                .initialValue = initialValue,
                                .currentValue = initialValue,
                                .dispersion = dispersion,
                                .increment = increment,}});

    mRecurrentTimer.registerRecurrentEvent(interval, propId);
    return StatusCode::OK;
}

StatusCode LinearFakeValueGenerator::stop(const VehiclePropValue& request) {
    const auto& v = request.value;
    if (v.int32Values.size() < 2) {
        ALOGE("%s: expected property ID in int32Values", __func__);
        return StatusCode::INVALID_ARG;
    }
    int32_t propId = v.int32Values[1];

    MuxGuard g(mLock);
    if (propId == 0) {
        // Remove all.
        for (auto&& it : mGenCfg) {
            removeLocked(it.first);
        }
    } else {
        removeLocked(propId);
    }
    return StatusCode::OK;
}

void LinearFakeValueGenerator::removeLocked(int propId) {
    if (mGenCfg.erase(propId)) {
        mRecurrentTimer.unregisterRecurrentEvent(propId);
    }
}

void LinearFakeValueGenerator::onTimer(const std::vector<int32_t>& properties) {
    MuxGuard g(mLock);

    for (int32_t propId : properties) {
        auto& cfg = mGenCfg[propId];
        cfg.currentValue += cfg.increment;
        if (cfg.currentValue > cfg.initialValue + cfg.dispersion) {
            cfg.currentValue = cfg.initialValue - cfg.dispersion;
        }
        VehiclePropValue event = {.prop = propId};
        auto& value = event.value;
        switch (getPropType(event.prop)) {
            case VehiclePropertyType::INT32:
                value.int32Values.resize(1);
                value.int32Values[0] = static_cast<int32_t>(cfg.currentValue);
                break;
            case VehiclePropertyType::INT64:
                value.int64Values.resize(1);
                value.int64Values[0] = static_cast<int64_t>(cfg.currentValue);
                break;
            case VehiclePropertyType::FLOAT:
                value.floatValues.resize(1);
                value.floatValues[0] = cfg.currentValue;
                break;
            default:
                ALOGE("%s: unsupported property type for 0x%x", __func__, event.prop);
                continue;
        }
        mOnHalEvent(event);
    }
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
