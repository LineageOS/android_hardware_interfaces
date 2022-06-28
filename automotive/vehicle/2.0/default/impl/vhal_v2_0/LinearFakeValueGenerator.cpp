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

LinearFakeValueGenerator::LinearFakeValueGenerator(int32_t propId, float middleValue,
                                                   float currentValue, float dispersion,
                                                   float increment, int64_t interval) {
    initGenCfg(propId, middleValue, currentValue, dispersion, increment, interval);
}

LinearFakeValueGenerator::LinearFakeValueGenerator(const VehiclePropValue& request) {
    const auto& v = request.value;
    initGenCfg(v.int32Values[1], v.floatValues[0], v.floatValues[0], v.floatValues[1],
               v.floatValues[2], v.int64Values[0]);
}

void LinearFakeValueGenerator::initGenCfg(int32_t propId, float middleValue, float currentValue,
                                          float dispersion, float increment, int64_t interval) {
    if (currentValue < middleValue - dispersion || currentValue >= middleValue + dispersion) {
        ALOGW("%s: invalid initValue: %f, out of range, default to %f", __func__, currentValue,
              middleValue);
        currentValue = middleValue;
    }
    mGenCfg = GeneratorCfg{
            .propId = propId,
            .middleValue = middleValue,
            .currentValue = currentValue,
            .dispersion = dispersion,
            .increment = increment,
            .interval = Nanos(interval),
    };
}

VehiclePropValue LinearFakeValueGenerator::nextEvent() {
    // TODO: (chenhaosjtuacm) remove "{}" if AGL compiler updated
    VehiclePropValue event = {.timestamp = {}, .areaId = {}, .prop = mGenCfg.propId};
    auto& value = event.value;
    switch (getPropType(event.prop)) {
        case VehiclePropertyType::INT32:
            value.int32Values.resize(1);
            value.int32Values[0] = static_cast<int32_t>(mGenCfg.currentValue);
            break;
        case VehiclePropertyType::INT64:
            value.int64Values.resize(1);
            value.int64Values[0] = static_cast<int64_t>(mGenCfg.currentValue);
            break;
        case VehiclePropertyType::FLOAT:
            value.floatValues.resize(1);
            value.floatValues[0] = mGenCfg.currentValue;
            break;
        default:
            ALOGE("%s: unsupported property type for 0x%x", __func__, event.prop);
    }
    TimePoint eventTime = Clock::now() + mGenCfg.interval;
    event.timestamp = eventTime.time_since_epoch().count();

    mGenCfg.currentValue += mGenCfg.increment;
    if (mGenCfg.currentValue >= mGenCfg.middleValue + mGenCfg.dispersion) {
        // Wrap around, (i - d) + c - (i + d) = c - 2 * d
        mGenCfg.currentValue = mGenCfg.currentValue - 2 * mGenCfg.dispersion;
    }
    return event;
}

bool LinearFakeValueGenerator::hasNext() {
    return true;
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
