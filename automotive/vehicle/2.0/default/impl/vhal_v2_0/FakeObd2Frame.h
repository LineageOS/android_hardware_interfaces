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
#ifndef android_hardware_automotive_vehicle_V2_0_impl_FakeObd2Frame_H_
#define android_hardware_automotive_vehicle_V2_0_impl_FakeObd2Frame_H_

#include <android/hardware/automotive/vehicle/2.0/types.h>
#include <vhal_v2_0/VehiclePropertyStore.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

void initObd2LiveFrame(VehiclePropertyStore* propStore, const VehiclePropConfig& propConfig);
void initObd2FreezeFrame(VehiclePropertyStore* propStore, const VehiclePropConfig& propConfig);
StatusCode fillObd2FreezeFrame(VehiclePropertyStore* propStore,
                               const VehiclePropValue& requestedPropValue,
                               VehiclePropValue* outValue);
StatusCode fillObd2DtcInfo(VehiclePropertyStore* propStore, VehiclePropValue* outValue);
StatusCode clearObd2FreezeFrames(VehiclePropertyStore* propStore,
                                 const VehiclePropValue& propValue);
bool isDiagnosticProperty(const VehiclePropConfig& propConfig);

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_impl_FakeObd2Frame_H_
