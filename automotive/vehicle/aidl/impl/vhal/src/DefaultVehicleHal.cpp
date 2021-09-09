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

#define LOG_TAG "DefaultVehicleHal"

#include <DefaultVehicleHal.h>

#include <VehicleHalTypes.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::GetValueRequests;
using ::aidl::android::hardware::automotive::vehicle::IVehicleCallback;
using ::aidl::android::hardware::automotive::vehicle::SetValueRequests;
using ::aidl::android::hardware::automotive::vehicle::SubscribeOptions;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs;
using ::ndk::ScopedAStatus;

ScopedAStatus DefaultVehicleHal::getAllPropConfigs(VehiclePropConfigs*) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::getValues(const std::shared_ptr<IVehicleCallback>&,
                                           const GetValueRequests&) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::setValues(const std::shared_ptr<IVehicleCallback>&,
                                           const SetValueRequests&) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::getPropConfigs(const std::vector<int32_t>&, VehiclePropConfigs*) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::subscribe(const std::shared_ptr<IVehicleCallback>&,
                                           const std::vector<SubscribeOptions>&, int32_t) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::unsubscribe(const std::shared_ptr<IVehicleCallback>&,
                                             const std::vector<int32_t>&) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

ScopedAStatus DefaultVehicleHal::returnSharedMemory(const std::shared_ptr<IVehicleCallback>&,
                                                    int64_t) {
    // TODO(b/200737967): implement this.
    return ScopedAStatus::ok();
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
