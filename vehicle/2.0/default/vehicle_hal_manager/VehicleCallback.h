/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef HIDL_GENERATED_android_hardware_vehicle_V2_0_VehicleCallback_H_
#define HIDL_GENERATED_android_hardware_vehicle_V2_0_VehicleCallback_H_

#include <android/hardware/vehicle/2.0/IVehicleCallback.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace vehicle {
namespace V2_0 {
namespace implementation {

using ::android::hardware::vehicle::V2_0::IVehicleCallback;
using ::android::hardware::vehicle::V2_0::VehiclePropValue;
using ::android::hardware::vehicle::V2_0::VehicleProperty;
using ::android::hardware::vehicle::V2_0::VehiclePropertyOperation;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct VehicleCallback : public IVehicleCallback {
    // Methods from ::android::hardware::vehicle::V2_0::IVehicleCallback follow.
    Return<void> onPropertyEvent(const hidl_vec<VehiclePropValue>& values)  override;
    Return<void> onPropertySet(const VehiclePropValue& value)  override;
    Return<void> onError(StatusCode errorCode, VehicleProperty propId, VehiclePropertyOperation operation)  override;

};

extern "C" IVehicleCallback* HIDL_FETCH_IVehicleCallback(const char* name);

}  // namespace implementation
}  // namespace V2_0
}  // namespace vehicle
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_vehicle_V2_0_VehicleCallback_H_
