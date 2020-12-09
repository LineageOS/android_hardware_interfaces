/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_UserHalHelper_H_
#define android_hardware_automotive_vehicle_V2_0_impl_UserHalHelper_H_

#include <android-base/result.h>
#include <android/hardware/automotive/vehicle/2.0/types.h>

#include <functional>
#include <memory>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace user_hal_helper {

// Verify whether the |value| can be casted to the type |T| and return the casted value on success.
// Otherwise, return the error.
template <typename T>
android::base::Result<T> verifyAndCast(int32_t value);

// Below functions parse VehiclePropValues to the respective User HAL request structs. On success,
// these functions return the User HAL struct. Otherwise, they return the error.
android::base::Result<InitialUserInfoRequest> toInitialUserInfoRequest(
        const VehiclePropValue& propValue);
android::base::Result<SwitchUserRequest> toSwitchUserRequest(const VehiclePropValue& propValue);
android::base::Result<CreateUserRequest> toCreateUserRequest(const VehiclePropValue& propValue);
android::base::Result<RemoveUserRequest> toRemoveUserRequest(const VehiclePropValue& propValue);
android::base::Result<UserIdentificationGetRequest> toUserIdentificationGetRequest(
        const VehiclePropValue& propValue);
android::base::Result<UserIdentificationSetRequest> toUserIdentificationSetRequest(
        const VehiclePropValue& propValue);

// Below functions convert the User HAL structs to VehiclePropValues. On success, these functions
// return the pointer to VehiclePropValue. Otherwise, they return nullptr.
std::unique_ptr<VehiclePropValue> toVehiclePropValue(const SwitchUserRequest& request);
std::unique_ptr<VehiclePropValue> toVehiclePropValue(const InitialUserInfoResponse& response);
std::unique_ptr<VehiclePropValue> toVehiclePropValue(const SwitchUserResponse& response);
std::unique_ptr<VehiclePropValue> toVehiclePropValue(const CreateUserResponse& response);
std::unique_ptr<VehiclePropValue> toVehiclePropValue(const UserIdentificationResponse& response);

}  // namespace user_hal_helper

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_impl_UserHalHelper_H_
