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

#ifndef android_hardware_automotive_vehicle_aidl_impl_fake_impl_GeneratorHub_include_FakeValueGenerator_H_
#define android_hardware_automotive_vehicle_aidl_impl_fake_impl_GeneratorHub_include_FakeValueGenerator_H_

#include <VehicleHalTypes.h>

#include <optional>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

// A abstract class for all fake value generators.
class FakeValueGenerator {
  public:
    virtual ~FakeValueGenerator() = default;

    // Returns the next event if there is one or {@code std::nullopt} if there is none.
    virtual std::optional<aidl::android::hardware::automotive::vehicle::VehiclePropValue>
    nextEvent() = 0;
};

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_fake_impl_GeneratorHub_include_FakeValueGenerator_H_
