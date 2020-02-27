/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_EmulatedVehicleConnector_H_
#define android_hardware_automotive_vehicle_V2_0_impl_EmulatedVehicleConnector_H_

#include <vhal_v2_0/VehicleConnector.h>

#include "VehicleHalClient.h"
#include "VehicleHalServer.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

using PassthroughConnector = IPassThroughConnector<VehicleHalClient, VehicleHalServer>;
using PassthroughConnectorPtr = std::unique_ptr<PassthroughConnector>;

PassthroughConnectorPtr makeEmulatedPassthroughConnector();

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_impl_EmulatedVehicleConnector_H_
