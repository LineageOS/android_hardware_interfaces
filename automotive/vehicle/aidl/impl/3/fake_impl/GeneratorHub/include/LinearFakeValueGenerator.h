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

#ifndef android_hardware_automotive_vehicle_aidl_impl_fake_impl_GeneratorHub_include_LinearFakeValueGenerator_H_
#define android_hardware_automotive_vehicle_aidl_impl_fake_impl_GeneratorHub_include_LinearFakeValueGenerator_H_

#include "FakeValueGenerator.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

class LinearFakeValueGenerator : public FakeValueGenerator {
  public:
    // A linear value generator initialized using values in request.
    // int32Values[1]: propId
    // floatValues[0]: middleValue and currentValue
    // floatValues[1]: dispersion
    // floatValues[2]: increment
    // int64Values[0]: interval
    // {@code propId} must be INT32 or INT64 or FLOAT type.
    explicit LinearFakeValueGenerator(
            const aidl::android::hardware::automotive::vehicle::VehiclePropValue& request);
    // A linear value generator in range [middleValue - dispersion, middleValue + dispersion),
    // starts at 'currentValue' and at each 'interval', increase by 'increment' and loop back if
    // exceeds middleValue + dispersion. {@code propId} must be INT32 or INT64 or FLOAT type.
    explicit LinearFakeValueGenerator(int32_t propId, float middleValue, float initValue,
                                      float dispersion, float increment, int64_t interval);
    ~LinearFakeValueGenerator() = default;

    std::optional<aidl::android::hardware::automotive::vehicle::VehiclePropValue> nextEvent()
            override;

  private:
    // In every timer tick we may want to generate new value based on initial value for debug
    // purpose. It's better to have sequential values to see if events gets delivered in order
    // to the client.
    struct GeneratorCfg {
        int32_t propId;
        float middleValue;
        float currentValue;  //  Should be in range (middleValue +/- dispersion).
        float dispersion;    //  Defines minimum and maximum value based on initial value.
        float increment;     //  Value that we will be added to currentValue with each timer tick.
        int64_t interval;
        int64_t lastEventTimestamp;
    };

    GeneratorCfg mGenCfg;

    void initGenCfg(int32_t propId, float middleValue, float initValue, float dispersion,
                    float increment, int64_t interval);
};

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_fake_impl_GeneratorHub_include_LinearFakeValueGenerator_H_
