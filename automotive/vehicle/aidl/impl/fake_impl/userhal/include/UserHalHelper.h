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

#ifndef android_hardware_automotive_vehicle_aidl_impl_fake_impl_userhal_include_UserHalHelper_H_
#define android_hardware_automotive_vehicle_aidl_impl_fake_impl_userhal_include_UserHalHelper_H_

#include <UserHalTypes.h>
#include <VehicleHalTypes.h>
#include <VehicleObjectPool.h>
#include <android-base/result.h>

#include <functional>
#include <memory>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {
namespace user_hal_helper {

// Verify whether the |value| can be casted to the type |T| and return the casted value on success.
// Otherwise, return the error.
template <typename T>
android::base::Result<T> verifyAndCast(int32_t value);

// Below functions parse VehiclePropValues to the respective User HAL request structs. On success,
// these functions return the User HAL struct. Otherwise, they return the error.
android::base::Result<aidl::android::hardware::automotive::vehicle::InitialUserInfoRequest>
toInitialUserInfoRequest(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);
android::base::Result<aidl::android::hardware::automotive::vehicle::SwitchUserRequest>
toSwitchUserRequest(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);
android::base::Result<aidl::android::hardware::automotive::vehicle::CreateUserRequest>
toCreateUserRequest(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);
android::base::Result<aidl::android::hardware::automotive::vehicle::RemoveUserRequest>
toRemoveUserRequest(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);
android::base::Result<aidl::android::hardware::automotive::vehicle::UserIdentificationGetRequest>
toUserIdentificationGetRequest(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);
android::base::Result<aidl::android::hardware::automotive::vehicle::UserIdentificationSetRequest>
toUserIdentificationSetRequest(
        const aidl::android::hardware::automotive::vehicle::VehiclePropValue& propValue);

// Below functions convert the User HAL structs to VehiclePropValues. On success, these functions
// return the pointer to VehiclePropValue. Otherwise, they return the error.
android::base::Result<VehiclePropValuePool::RecyclableType> toVehiclePropValue(
        VehiclePropValuePool& pool,
        const aidl::android::hardware::automotive::vehicle::SwitchUserRequest& request);
VehiclePropValuePool::RecyclableType toVehiclePropValue(
        VehiclePropValuePool& pool,
        const aidl::android::hardware::automotive::vehicle::InitialUserInfoResponse& response);
VehiclePropValuePool::RecyclableType toVehiclePropValue(
        VehiclePropValuePool& pool,
        const aidl::android::hardware::automotive::vehicle::SwitchUserResponse& response);
VehiclePropValuePool::RecyclableType toVehiclePropValue(
        VehiclePropValuePool& pool,
        const aidl::android::hardware::automotive::vehicle::CreateUserResponse& response);
VehiclePropValuePool::RecyclableType toVehiclePropValue(
        VehiclePropValuePool& pool,
        const aidl::android::hardware::automotive::vehicle::UserIdentificationResponse& response);

}  // namespace user_hal_helper
}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_fake_impl_userhal_include_UserHalHelper_H_
