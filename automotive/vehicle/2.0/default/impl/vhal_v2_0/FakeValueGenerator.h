/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_FakeValueGenerator_H_
#define android_hardware_automotive_vehicle_V2_0_impl_FakeValueGenerator_H_

#include <android/hardware/automotive/vehicle/2.0/types.h>

#include <chrono>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

class FakeValueGenerator {
public:
    virtual ~FakeValueGenerator() = default;

    virtual VehiclePropValue nextEvent() = 0;

    virtual bool hasNext() = 0;
};

using Clock = std::chrono::steady_clock;
using Nanos = std::chrono::nanoseconds;
using TimePoint = std::chrono::time_point<Clock, Nanos>;

using FakeValueGeneratorPtr = std::unique_ptr<FakeValueGenerator>;

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_impl_FakeValueGenerator_H_
