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

#ifndef android_hardware_automotive_vehicle_V2_1_impl_DefaultConfig_H_
#define android_hardware_automotive_vehicle_V2_1_impl_DefaultConfig_H_

#include <android/hardware/automotive/vehicle/2.1/types.h>
#include <vhal_v2_0/VehicleUtils.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_1 {

namespace impl {

// Some handy constants to avoid conversions from enum to int.
constexpr int OBD2_LIVE_FRAME = (int) V2_1::VehicleProperty::OBD2_LIVE_FRAME;
constexpr int OBD2_FREEZE_FRAME = (int) V2_1::VehicleProperty::OBD2_FREEZE_FRAME;
constexpr int OBD2_FREEZE_FRAME_INFO = (int) V2_1::VehicleProperty::OBD2_FREEZE_FRAME_INFO;
constexpr int OBD2_FREEZE_FRAME_CLEAR = (int) V2_1::VehicleProperty::OBD2_FREEZE_FRAME_CLEAR;
constexpr int VEHICLE_MAP_SERVICE = (int) V2_1::VehicleProperty::VEHICLE_MAP_SERVICE;
constexpr int WHEEL_TICK = (int) V2_1::VehicleProperty::WHEEL_TICK;


const V2_0::VehiclePropConfig kVehicleProperties[] = {
    {
        .prop = WHEEL_TICK,
        .access = V2_0::VehiclePropertyAccess::READ,
        .changeMode = V2_0::VehiclePropertyChangeMode::CONTINUOUS,
        .minSampleRate = 1.0f,
        .maxSampleRate = 100.0f,
    },

    {
        .prop = OBD2_LIVE_FRAME,
        .access = V2_0::VehiclePropertyAccess::READ,
        .changeMode = V2_0::VehiclePropertyChangeMode::ON_CHANGE,
        .configArray = {0,0}
    },

    {
        .prop = OBD2_FREEZE_FRAME,
        .access = V2_0::VehiclePropertyAccess::READ,
        .changeMode = V2_0::VehiclePropertyChangeMode::ON_CHANGE,
        .configArray = {0,0}
    },

    {
        .prop = OBD2_FREEZE_FRAME_INFO,
        .access = V2_0::VehiclePropertyAccess::READ,
        .changeMode = V2_0::VehiclePropertyChangeMode::ON_CHANGE
    },

    {
        .prop = OBD2_FREEZE_FRAME_CLEAR,
        .access = V2_0::VehiclePropertyAccess::WRITE,
        .changeMode = V2_0::VehiclePropertyChangeMode::ON_CHANGE
    },

    {
        .prop = VEHICLE_MAP_SERVICE,
        .access = V2_0::VehiclePropertyAccess::READ_WRITE,
        .changeMode = V2_0::VehiclePropertyChangeMode::ON_CHANGE
    }
};

}  // impl

}  // namespace V2_1
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif // android_hardware_automotive_vehicle_V2_1_impl_DefaultConfig_H_
