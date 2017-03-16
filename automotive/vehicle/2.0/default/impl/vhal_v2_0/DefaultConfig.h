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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_DefaultConfig_H_
#define android_hardware_automotive_vehicle_V2_0_impl_DefaultConfig_H_

#include <android/hardware/automotive/vehicle/2.0/IVehicle.h>
#include <vhal_v2_0/VehicleUtils.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

const int32_t kHvacPowerProperties[] = {
    toInt(VehicleProperty::HVAC_FAN_SPEED),
    toInt(VehicleProperty::HVAC_FAN_DIRECTION),
};

const VehiclePropConfig kVehicleProperties[] = {
    {
        .prop = toInt(VehicleProperty::INFO_MAKE),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::STATIC,
    },

    {
        .prop = toInt(VehicleProperty::PERF_VEHICLE_SPEED),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
    },

    {
        .prop = toInt(VehicleProperty::CURRENT_GEAR),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
    },

    {
        .prop = toInt(VehicleProperty::PARKING_BRAKE_ON),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
    },

    {
        .prop = toInt(VehicleProperty::FUEL_LEVEL_LOW),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
    },

    {
        .prop = toInt(VehicleProperty::HVAC_POWER_ON),
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .supportedAreas = toInt(VehicleAreaZone::ROW_1),
        // TODO(bryaneyler): Ideally, this is generated dynamically from
        // kHvacPowerProperties.
        .configString = "0x12400500,0x12400501"  // HVAC_FAN_SPEED,HVAC_FAN_DIRECTION
    },

    {
        .prop = toInt(VehicleProperty::HVAC_DEFROSTER),
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .supportedAreas =
                VehicleAreaWindow::FRONT_WINDSHIELD
                | VehicleAreaWindow::REAR_WINDSHIELD
    },

    {
        .prop = toInt(VehicleProperty::HVAC_RECIRC_ON),
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .supportedAreas = toInt(VehicleAreaZone::ROW_1)
    },

    {
        .prop = toInt(VehicleProperty::HVAC_AC_ON),
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .supportedAreas = toInt(VehicleAreaZone::ROW_1)
    },

    {
        .prop = toInt(VehicleProperty::HVAC_AUTO_ON),
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .supportedAreas = toInt(VehicleAreaZone::ROW_1)
    },

    {
        .prop = toInt(VehicleProperty::HVAC_FAN_SPEED),
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .supportedAreas = toInt(VehicleAreaZone::ROW_1),
        .areaConfigs = {
            VehicleAreaConfig {
                .areaId = toInt(VehicleAreaZone::ROW_1),
                .minInt32Value = 1,
                .maxInt32Value = 7
            }
        }
    },

    {
        .prop = toInt(VehicleProperty::HVAC_FAN_DIRECTION),
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .supportedAreas = toInt(VehicleAreaZone::ROW_1),
    },

    {
        .prop = toInt(VehicleProperty::HVAC_TEMPERATURE_SET),
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .supportedAreas =
                VehicleAreaZone::ROW_1_LEFT
                | VehicleAreaZone::ROW_1_RIGHT,
        .areaConfigs = {
            VehicleAreaConfig {
                .areaId = toInt(VehicleAreaZone::ROW_1_LEFT),
                .minFloatValue = 16,
                .maxFloatValue = 32,
            },
            VehicleAreaConfig {
                .areaId = toInt(VehicleAreaZone::ROW_1_RIGHT),
                .minFloatValue = 16,
                .maxFloatValue = 32,
            }
        }
    },

    {
        .prop = toInt(VehicleProperty::ENV_OUTSIDE_TEMPERATURE),
        .access = VehiclePropertyAccess::READ,
        // TODO(bryaneyler): Support ON_CHANGE as well.
        .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
        .minSampleRate = 1.0f,
        .maxSampleRate = 2.0f,
    },

    {
        .prop = toInt(VehicleProperty::NIGHT_MODE),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
    },

    {
        .prop = toInt(VehicleProperty::DRIVING_STATUS),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
    },

    {
        .prop = toInt(VehicleProperty::GEAR_SELECTION),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
    },

    {
        .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::STATIC,
    },

    {
        .prop = toInt(VehicleProperty::DISPLAY_BRIGHTNESS),
        .access = VehiclePropertyAccess::READ_WRITE,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
        .areaConfigs = {
            VehicleAreaConfig {
                .minInt32Value = 0,
                .maxInt32Value = 10
            }
        }
    },

    {
        .prop = toInt(VehicleProperty::IGNITION_STATE),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
    },

    {
        .prop = toInt(VehicleProperty::ENGINE_OIL_TEMP),
        .access = VehiclePropertyAccess::READ,
        .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
        .minSampleRate = 0.1, // 0.1 Hz, every 10 seconds
        .maxSampleRate = 10,  // 10 Hz, every 100 ms
    }
};

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif // android_hardware_automotive_vehicle_V2_0_impl_DefaultConfig_H_
