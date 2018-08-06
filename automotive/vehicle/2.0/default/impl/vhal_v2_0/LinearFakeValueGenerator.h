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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_LinearFakeValueGenerator_H_
#define android_hardware_automotive_vehicle_V2_0_impl_LinearFakeValueGenerator_H_

#include <vhal_v2_0/RecurrentTimer.h>

#include "FakeValueGenerator.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

class LinearFakeValueGenerator : public FakeValueGenerator {
private:
    // In every timer tick we may want to generate new value based on initial value for debug
    // purpose. It's better to have sequential values to see if events gets delivered in order
    // to the client.

    struct GeneratorCfg {
        float initialValue;  //
        float currentValue;  //  Should be in range (initialValue +/- dispersion).
        float dispersion;    //  Defines minimum and maximum value based on initial value.
        float increment;     //  Value that we will be added to currentValue with each timer tick.
    };

public:
    LinearFakeValueGenerator(const OnHalEvent& onHalEvent);
    ~LinearFakeValueGenerator() = default;
    StatusCode start(const VehiclePropValue& request) override;
    StatusCode stop(const VehiclePropValue& request) override;

private:
    void removeLocked(int propId);
    void onTimer(const std::vector<int32_t>& properties);

private:
    mutable std::mutex mLock;
    OnHalEvent mOnHalEvent;
    RecurrentTimer mRecurrentTimer;
    std::unordered_map<int32_t, GeneratorCfg> mGenCfg;
};

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_impl_LinearFakeValueGenerator_H_
