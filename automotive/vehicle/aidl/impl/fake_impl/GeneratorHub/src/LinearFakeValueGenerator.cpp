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

#define LOG_TAG "LinearFakeValueGenerator"

#include "LinearFakeValueGenerator.h"

#include <VehicleUtils.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropValue;

LinearFakeValueGenerator::LinearFakeValueGenerator(int32_t propId, float middleValue,
                                                   float initValue, float dispersion,
                                                   float increment, int64_t interval) {
    initGenCfg(propId, middleValue, initValue, dispersion, increment, interval);
}

LinearFakeValueGenerator::LinearFakeValueGenerator(const VehiclePropValue& request) {
    const auto& v = request.value;
    initGenCfg(v.int32Values[1], v.floatValues[0], v.floatValues[0], v.floatValues[1],
               v.floatValues[2], v.int64Values[0]);
}

void LinearFakeValueGenerator::initGenCfg(int32_t propId, float middleValue, float initValue,
                                          float dispersion, float increment, int64_t interval) {
    // Other types are not supported.
    assert(getPropType(propId) == VehicleProperty::INT32 ||
           getPropType(propId) == VehicleProperty::INT64 ||
           getPropType(propId) == VehicleProperty::FLOAT);

    if (initValue < middleValue - dispersion || initValue >= middleValue + dispersion) {
        ALOGW("%s: invalid initValue: %f, out of range, default to %f", __func__, initValue,
              middleValue);
        initValue = middleValue;
    }
    mGenCfg = GeneratorCfg{
            .propId = propId,
            .middleValue = middleValue,
            .currentValue = initValue,
            .dispersion = dispersion,
            .increment = increment,
            .interval = interval,
    };
}

std::optional<VehiclePropValue> LinearFakeValueGenerator::nextEvent() {
    VehiclePropValue event = {
            .prop = mGenCfg.propId,
    };
    auto& value = event.value;
    switch (getPropType(event.prop)) {
        case VehiclePropertyType::INT32:
            value.int32Values = {static_cast<int32_t>(mGenCfg.currentValue)};
            break;
        case VehiclePropertyType::INT64:
            value.int64Values = {static_cast<int64_t>(mGenCfg.currentValue)};
            break;
        case VehiclePropertyType::FLOAT:
            value.floatValues = {mGenCfg.currentValue};
            break;
        default:
            ALOGE("%s: unsupported property type for 0x%x", __func__, event.prop);
    }
    if (mGenCfg.lastEventTimestamp == 0) {
        mGenCfg.lastEventTimestamp = elapsedRealtimeNano();
    } else {
        int64_t nextEventTime = mGenCfg.lastEventTimestamp + mGenCfg.interval;
        // Prevent overflow.
        assert(nextEventTime > mGenCfg.lastEventTimestamp);
        mGenCfg.lastEventTimestamp = nextEventTime;
    }
    event.timestamp = mGenCfg.lastEventTimestamp;

    mGenCfg.currentValue += mGenCfg.increment;
    if (mGenCfg.currentValue >= mGenCfg.middleValue + mGenCfg.dispersion) {
        // Wrap around, (i - d) + c - (i + d) = c - 2 * d
        mGenCfg.currentValue = mGenCfg.currentValue - 2 * mGenCfg.dispersion;
    }
    return event;
}

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
