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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_include_DefaultVehicleHal_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_include_DefaultVehicleHal_H_

#include <aidl/android/hardware/automotive/vehicle/BnVehicle.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

class DefaultVehicleHal final : public ::aidl::android::hardware::automotive::vehicle::BnVehicle {
    ::ndk::ScopedAStatus getAllPropConfigs(
            ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs* returnConfigs)
            override;
    ::ndk::ScopedAStatus getValues(
            const std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>&
                    callback,
            const ::aidl::android::hardware::automotive::vehicle::GetValueRequests& requests)
            override;
    ::ndk::ScopedAStatus setValues(
            const std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>&
                    callback,
            const ::aidl::android::hardware::automotive::vehicle::SetValueRequests& requests)
            override;
    ::ndk::ScopedAStatus getPropConfigs(
            const std::vector<int32_t>& props,
            ::aidl::android::hardware::automotive::vehicle::VehiclePropConfigs* returnConfigs)
            override;
    ::ndk::ScopedAStatus subscribe(
            const std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>&
                    callback,
            const std::vector<::aidl::android::hardware::automotive::vehicle::SubscribeOptions>&
                    options,
            int32_t maxSharedMemoryFileCount) override;
    ::ndk::ScopedAStatus unsubscribe(
            const std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>&
                    callback,
            const std::vector<int32_t>& propIds) override;
    ::ndk::ScopedAStatus returnSharedMemory(
            const std::shared_ptr<::aidl::android::hardware::automotive::vehicle::IVehicleCallback>&
                    callback,
            int64_t sharedMemoryId) override;
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_DefaultVehicleHal_H_
