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

#include "PropertyUtils.h"

#include <map>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

struct ConfigDeclaration {
    VehiclePropConfig config;

    /* This value will be used as an initial value for the property. If this field is specified for
     * property that supports multiple areas then it will be used for all areas unless particular
     * area is overridden in initialAreaValue field. */
    VehiclePropValue::RawValue initialValue;
    /* Use initialAreaValues if it is necessary to specify different values per each area. */
    std::map<int32_t, VehiclePropValue::RawValue> initialAreaValues;
};

const ConfigDeclaration kVehicleProperties[]{
        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_FUEL_CAPACITY),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.floatValues = {15000.0f}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_FUEL_TYPE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {(int)FuelType::FUEL_TYPE_UNLEADED}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_EV_BATTERY_CAPACITY),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.floatValues = {150000.0f}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_EV_CONNECTOR_TYPE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {(int)EvConnectorType::IEC_TYPE_1_AC}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_FUEL_DOOR_LOCATION),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {FUEL_DOOR_REAR_LEFT}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_EV_PORT_LOCATION),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {CHARGE_PORT_FRONT_LEFT}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_MULTI_EV_PORT_LOCATIONS),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {CHARGE_PORT_FRONT_LEFT, CHARGE_PORT_REAR_LEFT}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_MAKE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.stringValue = "Toy Vehicle"}},
        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_MODEL),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.stringValue = "Speedy Model"}},
        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_MODEL_YEAR),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {2020}}},
        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_EXTERIOR_DIMENSIONS),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {1776, 4950, 2008, 2140, 2984, 1665, 1667, 11800}}},
        {.config =
                 {
                         .prop = toInt(VehicleProperty::PERF_VEHICLE_SPEED),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.floatValues = {0.0f}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::VEHICLE_SPEED_DISPLAY_UNITS),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray = {(int)VehicleUnit::METER_PER_SEC,
                                         (int)VehicleUnit::MILES_PER_HOUR,
                                         (int)VehicleUnit::KILOMETERS_PER_HOUR},
                 },
         .initialValue = {.int32Values = {(int)VehicleUnit::KILOMETERS_PER_HOUR}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::SEAT_OCCUPANCY),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                         .areaConfigs = {VehicleAreaConfig{.areaId = (SEAT_1_LEFT)},
                                         VehicleAreaConfig{.areaId = (SEAT_1_RIGHT)}},
                 },
         .initialAreaValues = {{SEAT_1_LEFT,
                                {.int32Values = {(int)VehicleSeatOccupancyState::VACANT}}},
                               {SEAT_1_RIGHT,
                                {.int32Values = {(int)VehicleSeatOccupancyState::VACANT}}}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::INFO_DRIVER_SEAT),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                         // this was a zoned property on an old vhal, but it is meant to be global
                         .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
                 },
         .initialValue = {.int32Values = {SEAT_1_LEFT}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::PERF_ODOMETER),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.floatValues = {0.0f}}},
        {.config =
                 {
                         .prop = toInt(VehicleProperty::PERF_STEERING_ANGLE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.floatValues = {0.0f}}},
        {.config =
                 {
                         .prop = toInt(VehicleProperty::PERF_REAR_STEERING_ANGLE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.floatValues = {0.0f}}},
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::ENGINE_RPM),
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                                .minSampleRate = 1.0f,
                                .maxSampleRate = 10.0f,
                        },
                .initialValue = {.floatValues = {0.0f}},
        },

        {.config =
                 {
                         .prop = toInt(VehicleProperty::FUEL_LEVEL),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 100.0f,
                 },
         .initialValue = {.floatValues = {15000.0f}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::FUEL_DOOR_OPEN),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::EV_BATTERY_LEVEL),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 100.0f,
                 },
         .initialValue = {.floatValues = {150000.0f}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::EV_CHARGE_PORT_OPEN),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::EV_CHARGE_PORT_CONNECTED),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::EV_BATTERY_INSTANTANEOUS_CHARGE_RATE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.floatValues = {0.0f}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::RANGE_REMAINING),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 2.0f,
                 },
         .initialValue = {.floatValues = {50000.0f}}},  // units in meters

        {.config =
                 {
                         .prop = toInt(VehicleProperty::TIRE_PRESSURE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .areaConfigs = {VehicleAreaConfig{
                                                 .areaId = WHEEL_FRONT_LEFT,
                                                 .minFloatValue = 193.0f,
                                                 .maxFloatValue = 300.0f,
                                         },
                                         VehicleAreaConfig{
                                                 .areaId = WHEEL_FRONT_RIGHT,
                                                 .minFloatValue = 193.0f,
                                                 .maxFloatValue = 300.0f,
                                         },
                                         VehicleAreaConfig{
                                                 .areaId = WHEEL_REAR_LEFT,
                                                 .minFloatValue = 193.0f,
                                                 .maxFloatValue = 300.0f,
                                         },
                                         VehicleAreaConfig{
                                                 .areaId = WHEEL_REAR_RIGHT,
                                                 .minFloatValue = 193.0f,
                                                 .maxFloatValue = 300.0f,
                                         }},
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 2.0f,
                 },
         .initialValue = {.floatValues = {200.0f}}},  // units in kPa

        {.config =
                 {
                         .prop = toInt(VehicleProperty::CRITICALLY_LOW_TIRE_PRESSURE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::STATIC,
                         .areaConfigs = {VehicleAreaConfig{
                                                 .areaId = WHEEL_FRONT_LEFT,
                                         },
                                         VehicleAreaConfig{
                                                 .areaId = WHEEL_FRONT_RIGHT,
                                         },
                                         VehicleAreaConfig{
                                                 .areaId = WHEEL_REAR_LEFT,
                                         },
                                         VehicleAreaConfig{
                                                 .areaId = WHEEL_REAR_RIGHT,
                                         }},
                 },
         .initialAreaValues = {{WHEEL_FRONT_LEFT, {.floatValues = {137.0f}}},
                               {WHEEL_FRONT_RIGHT, {.floatValues = {137.0f}}},
                               {WHEEL_REAR_RIGHT, {.floatValues = {137.0f}}},
                               {WHEEL_REAR_LEFT, {.floatValues = {137.0f}}}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::TIRE_PRESSURE_DISPLAY_UNITS),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray = {(int)VehicleUnit::KILOPASCAL, (int)VehicleUnit::PSI,
                                         (int)VehicleUnit::BAR},
                 },
         .initialValue = {.int32Values = {toInt(VehicleUnit::PSI)}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::CURRENT_GEAR),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray = {(int)VehicleGear::GEAR_PARK,
                                         (int)VehicleGear::GEAR_NEUTRAL,
                                         (int)VehicleGear::GEAR_REVERSE, (int)VehicleGear::GEAR_1,
                                         (int)VehicleGear::GEAR_2, (int)VehicleGear::GEAR_3,
                                         (int)VehicleGear::GEAR_4, (int)VehicleGear::GEAR_5},
                 },
         .initialValue = {.int32Values = {toInt(VehicleGear::GEAR_PARK)}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::PARKING_BRAKE_ON),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {1}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::PARKING_BRAKE_AUTO_APPLY),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {1}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::FUEL_LEVEL_LOW),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::HW_KEY_INPUT),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0, 0, 0}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::HW_ROTARY_INPUT),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0, 0, 0}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::HW_CUSTOM_INPUT),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray = {0, 0, 0, 3, 0, 0, 0, 0, 0},
                 },
         .initialValue =
                 {
                         .int32Values = {0, 0, 0},
                 }},

        {.config = {.prop = toInt(VehicleProperty::HVAC_POWER_ON),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_ALL}},
                    // TODO(bryaneyler): Ideally, this is generated dynamically from
                    // kHvacPowerProperties.
                    .configArray = {toInt(VehicleProperty::HVAC_FAN_SPEED),
                                    toInt(VehicleProperty::HVAC_FAN_DIRECTION)}},
         .initialValue = {.int32Values = {1}}},

        {
                .config = {.prop = toInt(VehicleProperty::HVAC_DEFROSTER),
                           .access = VehiclePropertyAccess::READ_WRITE,
                           .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                           .areaConfigs =
                                   {VehicleAreaConfig{
                                            .areaId = toInt(VehicleAreaWindow::FRONT_WINDSHIELD)},
                                    VehicleAreaConfig{
                                            .areaId = toInt(VehicleAreaWindow::REAR_WINDSHIELD)}}},
                .initialValue = {.int32Values = {0}}  // Will be used for all areas.
        },
        {
                .config = {.prop = toInt(VehicleProperty::HVAC_ELECTRIC_DEFROSTER_ON),
                           .access = VehiclePropertyAccess::READ_WRITE,
                           .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                           .areaConfigs =
                                   {VehicleAreaConfig{
                                            .areaId = toInt(VehicleAreaWindow::FRONT_WINDSHIELD)},
                                    VehicleAreaConfig{
                                            .areaId = toInt(VehicleAreaWindow::REAR_WINDSHIELD)}}},
                .initialValue = {.int32Values = {0}}  // Will be used for all areas.
        },

        {.config = {.prop = toInt(VehicleProperty::HVAC_MAX_DEFROST_ON),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(VehicleProperty::HVAC_RECIRC_ON),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {1}}},

        {.config = {.prop = toInt(VehicleProperty::HVAC_AUTO_RECIRC_ON),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(VehicleProperty::HVAC_AC_ON),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {1}}},

        {.config = {.prop = toInt(VehicleProperty::HVAC_MAX_AC_ON),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(VehicleProperty::HVAC_AUTO_ON),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {1}}},

        {.config = {.prop = toInt(VehicleProperty::HVAC_DUAL_ON),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(VehicleProperty::HVAC_FAN_SPEED),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{
                            .areaId = HVAC_ALL, .minInt32Value = 1, .maxInt32Value = 7}}},
         .initialValue = {.int32Values = {3}}},

        {.config = {.prop = toInt(VehicleProperty::HVAC_FAN_DIRECTION),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {toInt(VehicleHvacFanDirection::FACE)}}},

        {.config = {.prop = toInt(VehicleProperty::HVAC_FAN_DIRECTION_AVAILABLE),
                    .access = VehiclePropertyAccess::READ,
                    .changeMode = VehiclePropertyChangeMode::STATIC,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {FAN_DIRECTION_FACE, FAN_DIRECTION_FLOOR,
                                          FAN_DIRECTION_FACE | FAN_DIRECTION_FLOOR,
                                          FAN_DIRECTION_DEFROST,
                                          FAN_DIRECTION_FACE | FAN_DIRECTION_DEFROST,
                                          FAN_DIRECTION_FLOOR | FAN_DIRECTION_DEFROST,
                                          FAN_DIRECTION_FLOOR | FAN_DIRECTION_DEFROST |
                                                  FAN_DIRECTION_FACE}}},
        {.config = {.prop = toInt(VehicleProperty::HVAC_SEAT_VENTILATION),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{
                                            .areaId = SEAT_1_LEFT,
                                            .minInt32Value = 0,
                                            .maxInt32Value = 3,
                                    },
                                    VehicleAreaConfig{
                                            .areaId = SEAT_1_RIGHT,
                                            .minInt32Value = 0,
                                            .maxInt32Value = 3,
                                    }}},
         .initialValue =
                 {.int32Values = {0}}},  // 0 is off and +ve values indicate ventilation level.

        {.config = {.prop = toInt(VehicleProperty::HVAC_STEERING_WHEEL_HEAT),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{
                            .areaId = (0), .minInt32Value = -2, .maxInt32Value = 2}}},
         .initialValue = {.int32Values = {0}}},  // +ve values for heating and -ve for cooling

        {.config = {.prop = toInt(VehicleProperty::HVAC_SEAT_TEMPERATURE),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{
                                            .areaId = SEAT_1_LEFT,
                                            .minInt32Value = -2,
                                            .maxInt32Value = 2,
                                    },
                                    VehicleAreaConfig{
                                            .areaId = SEAT_1_RIGHT,
                                            .minInt32Value = -2,
                                            .maxInt32Value = 2,
                                    }}},
         .initialValue = {.int32Values = {0}}},  // +ve values for heating and -ve for cooling

        {.config = {.prop = toInt(VehicleProperty::HVAC_TEMPERATURE_SET),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .configArray = {160, 280, 5, 605, 825, 10},
                    .areaConfigs = {VehicleAreaConfig{
                                            .areaId = HVAC_LEFT,
                                            .minFloatValue = 16,
                                            .maxFloatValue = 32,
                                    },
                                    VehicleAreaConfig{
                                            .areaId = HVAC_RIGHT,
                                            .minFloatValue = 16,
                                            .maxFloatValue = 32,
                                    }}},
         .initialAreaValues = {{HVAC_LEFT, {.floatValues = {16}}},
                               {HVAC_RIGHT, {.floatValues = {20}}}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::HVAC_TEMPERATURE_VALUE_SUGGESTION),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.floatValues = {66.2f, (float)VehicleUnit::FAHRENHEIT, 19.0f, 66.5f}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::ENV_OUTSIDE_TEMPERATURE),
                         .access = VehiclePropertyAccess::READ,
                         // TODO(bryaneyler): Support ON_CHANGE as well.
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 2.0f,
                 },
         .initialValue = {.floatValues = {25.0f}}},

        {.config = {.prop = toInt(VehicleProperty::HVAC_TEMPERATURE_DISPLAY_UNITS),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .configArray = {(int)VehicleUnit::FAHRENHEIT, (int)VehicleUnit::CELSIUS}},
         .initialValue = {.int32Values = {(int)VehicleUnit::FAHRENHEIT}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::DISTANCE_DISPLAY_UNITS),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                         .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
                         .configArray = {(int)VehicleUnit::KILOMETER, (int)VehicleUnit::MILE},
                 },
         .initialValue = {.int32Values = {(int)VehicleUnit::MILE}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::NIGHT_MODE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::GEAR_SELECTION),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray = {(int)VehicleGear::GEAR_PARK,
                                         (int)VehicleGear::GEAR_NEUTRAL,
                                         (int)VehicleGear::GEAR_REVERSE,
                                         (int)VehicleGear::GEAR_DRIVE, (int)VehicleGear::GEAR_1,
                                         (int)VehicleGear::GEAR_2, (int)VehicleGear::GEAR_3,
                                         (int)VehicleGear::GEAR_4, (int)VehicleGear::GEAR_5},
                 },
         .initialValue = {.int32Values = {toInt(VehicleGear::GEAR_PARK)}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::TURN_SIGNAL_STATE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {toInt(VehicleTurnSignal::NONE)}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::IGNITION_STATE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {toInt(VehicleIgnitionState::ON)}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::ENGINE_OIL_LEVEL),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {toInt(VehicleOilLevel::NORMAL)}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::ENGINE_OIL_TEMP),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 0.1,  // 0.1 Hz, every 10 seconds
                         .maxSampleRate = 10,   // 10 Hz, every 100 ms
                 },
         .initialValue = {.floatValues = {101.0f}}},

        {
                .config = {.prop = kMixedTypePropertyForTest,
                           .access = VehiclePropertyAccess::READ_WRITE,
                           .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                           .configArray = {1, 1, 0, 2, 0, 0, 1, 0, 0}},
                .initialValue =
                        {
                                .int32Values = {1 /* indicate TRUE boolean value */, 2, 3},
                                .floatValues = {4.5f},
                                .stringValue = "MIXED property",
                        },
        },

        {.config = {.prop = toInt(VehicleProperty::DOOR_LOCK),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = DOOR_1_LEFT},
                                    VehicleAreaConfig{.areaId = DOOR_1_RIGHT},
                                    VehicleAreaConfig{.areaId = DOOR_2_LEFT},
                                    VehicleAreaConfig{.areaId = DOOR_2_RIGHT}}},
         .initialAreaValues = {{DOOR_1_LEFT, {.int32Values = {1}}},
                               {DOOR_1_RIGHT, {.int32Values = {1}}},
                               {DOOR_2_LEFT, {.int32Values = {1}}},
                               {DOOR_2_RIGHT, {.int32Values = {1}}}}},

        {.config = {.prop = toInt(VehicleProperty::DOOR_POS),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs =
                            {VehicleAreaConfig{
                                     .areaId = DOOR_1_LEFT, .minInt32Value = 0, .maxInt32Value = 1},
                             VehicleAreaConfig{.areaId = DOOR_1_RIGHT,
                                               .minInt32Value = 0,
                                               .maxInt32Value = 1},
                             VehicleAreaConfig{
                                     .areaId = DOOR_2_LEFT, .minInt32Value = 0, .maxInt32Value = 1},
                             VehicleAreaConfig{.areaId = DOOR_2_RIGHT,
                                               .minInt32Value = 0,
                                               .maxInt32Value = 1},
                             VehicleAreaConfig{
                                     .areaId = DOOR_REAR, .minInt32Value = 0, .maxInt32Value = 1}}},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(VehicleProperty::WINDOW_LOCK),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = WINDOW_1_RIGHT | WINDOW_2_LEFT |
                                                                WINDOW_2_RIGHT}}},
         .initialAreaValues = {{WINDOW_1_RIGHT | WINDOW_2_LEFT | WINDOW_2_RIGHT,
                                {.int32Values = {0}}}}},

        {.config = {.prop = toInt(VehicleProperty::WINDOW_POS),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = WINDOW_1_LEFT,
                                                      .minInt32Value = 0,
                                                      .maxInt32Value = 10},
                                    VehicleAreaConfig{.areaId = WINDOW_1_RIGHT,
                                                      .minInt32Value = 0,
                                                      .maxInt32Value = 10},
                                    VehicleAreaConfig{.areaId = WINDOW_2_LEFT,
                                                      .minInt32Value = 0,
                                                      .maxInt32Value = 10},
                                    VehicleAreaConfig{.areaId = WINDOW_2_RIGHT,
                                                      .minInt32Value = 0,
                                                      .maxInt32Value = 10},
                                    VehicleAreaConfig{.areaId = WINDOW_ROOF_TOP_1,
                                                      .minInt32Value = -10,
                                                      .maxInt32Value = 10}}},
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = WHEEL_TICK,
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
                         .configArray = {ALL_WHEELS, 50000, 50000, 50000, 50000},
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.int64Values = {0, 100000, 200000, 300000, 400000}}},

        {.config = {.prop = ABS_ACTIVE,
                    .access = VehiclePropertyAccess::READ,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = TRACTION_CONTROL_ACTIVE,
                    .access = VehiclePropertyAccess::READ,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(VehicleProperty::AP_POWER_STATE_REQ),
                    .access = VehiclePropertyAccess::READ,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .configArray = {3}},
         .initialValue = {.int32Values = {toInt(VehicleApPowerStateReq::ON), 0}}},

        {.config = {.prop = toInt(VehicleProperty::AP_POWER_STATE_REPORT),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.int32Values = {toInt(VehicleApPowerStateReport::WAIT_FOR_VHAL), 0}}},

        {.config = {.prop = toInt(VehicleProperty::DISPLAY_BRIGHTNESS),
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.minInt32Value = 0, .maxInt32Value = 100}}},
         .initialValue = {.int32Values = {100}}},

        {
                .config = {.prop = OBD2_LIVE_FRAME,
                           .access = VehiclePropertyAccess::READ,
                           .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                           .configArray = {0, 0}},
        },

        {
                .config = {.prop = OBD2_FREEZE_FRAME,
                           .access = VehiclePropertyAccess::READ,
                           .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                           .configArray = {0, 0}},
        },

        {
                .config = {.prop = OBD2_FREEZE_FRAME_INFO,
                           .access = VehiclePropertyAccess::READ,
                           .changeMode = VehiclePropertyChangeMode::ON_CHANGE},
        },

        {
                .config = {.prop = OBD2_FREEZE_FRAME_CLEAR,
                           .access = VehiclePropertyAccess::WRITE,
                           .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                           .configArray = {1}},
        },

        {.config =
                 {
                         .prop = toInt(VehicleProperty::HEADLIGHTS_STATE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::HIGH_BEAM_LIGHTS_STATE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::FOG_LIGHTS_STATE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::HAZARD_LIGHTS_STATE),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::HEADLIGHTS_SWITCH),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::HIGH_BEAM_LIGHTS_SWITCH),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::FOG_LIGHTS_SWITCH),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::HAZARD_LIGHTS_SWITCH),
                         .access = VehiclePropertyAccess::READ_WRITE,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::EVS_SERVICE_REQUEST),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {toInt(EvsServiceType::REARVIEW),
                                          toInt(EvsServiceState::OFF)}}},

        {.config = {.prop = VEHICLE_MAP_SERVICE,
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE}},

        // Example Vendor Extension properties for testing
        {.config = {.prop = VENDOR_EXTENSION_BOOLEAN_PROPERTY,
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = DOOR_1_LEFT},
                                    VehicleAreaConfig{.areaId = DOOR_1_RIGHT},
                                    VehicleAreaConfig{.areaId = DOOR_2_LEFT},
                                    VehicleAreaConfig{.areaId = DOOR_2_RIGHT}}},
         .initialAreaValues = {{DOOR_1_LEFT, {.int32Values = {1}}},
                               {DOOR_1_RIGHT, {.int32Values = {1}}},
                               {DOOR_2_LEFT, {.int32Values = {0}}},
                               {DOOR_2_RIGHT, {.int32Values = {0}}}}},

        {.config = {.prop = VENDOR_EXTENSION_FLOAT_PROPERTY,
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {VehicleAreaConfig{.areaId = HVAC_LEFT,
                                                      .minFloatValue = -10,
                                                      .maxFloatValue = 10},
                                    VehicleAreaConfig{.areaId = HVAC_RIGHT,
                                                      .minFloatValue = -10,
                                                      .maxFloatValue = 10}}},
         .initialAreaValues = {{HVAC_LEFT, {.floatValues = {1}}},
                               {HVAC_RIGHT, {.floatValues = {2}}}}},

        {.config = {.prop = VENDOR_EXTENSION_INT_PROPERTY,
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs =
                            {VehicleAreaConfig{.areaId = (int)VehicleAreaWindow::FRONT_WINDSHIELD,
                                               .minInt32Value = -100,
                                               .maxInt32Value = 100},
                             VehicleAreaConfig{.areaId = (int)VehicleAreaWindow::REAR_WINDSHIELD,
                                               .minInt32Value = -100,
                                               .maxInt32Value = 100},
                             VehicleAreaConfig{.areaId = (int)VehicleAreaWindow::ROOF_TOP_1,
                                               .minInt32Value = -100,
                                               .maxInt32Value = 100}}},
         .initialAreaValues = {{(int)VehicleAreaWindow::FRONT_WINDSHIELD, {.int32Values = {1}}},
                               {(int)VehicleAreaWindow::REAR_WINDSHIELD, {.int32Values = {0}}},
                               {(int)VehicleAreaWindow::ROOF_TOP_1, {.int32Values = {-1}}}}},

        {.config = {.prop = VENDOR_EXTENSION_STRING_PROPERTY,
                    .access = VehiclePropertyAccess::READ_WRITE,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.stringValue = "Vendor String Property"}},

        {.config = {.prop = toInt(VehicleProperty::ELECTRONIC_TOLL_COLLECTION_CARD_TYPE),
                    .access = VehiclePropertyAccess::READ,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(VehicleProperty::ELECTRONIC_TOLL_COLLECTION_CARD_STATUS),
                    .access = VehiclePropertyAccess::READ,
                    .changeMode = VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(VehicleProperty::SUPPORT_CUSTOMIZE_VENDOR_PERMISSION),
                         .access = VehiclePropertyAccess::READ,
                         .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray =
                                 {kMixedTypePropertyForTest,
                                  (int)VehicleVendorPermission::PERMISSION_GET_VENDOR_CATEGORY_INFO,
                                  (int)VehicleVendorPermission::PERMISSION_SET_VENDOR_CATEGORY_INFO,
                                  VENDOR_EXTENSION_INT_PROPERTY,
                                  (int)VehicleVendorPermission::PERMISSION_GET_VENDOR_CATEGORY_SEAT,
                                  (int)VehicleVendorPermission::PERMISSION_NOT_ACCESSIBLE,
                                  VENDOR_EXTENSION_FLOAT_PROPERTY,
                                  (int)VehicleVendorPermission::PERMISSION_DEFAULT,
                                  (int)VehicleVendorPermission::PERMISSION_DEFAULT},
                 },
         .initialValue = {.int32Values = {1}}},

        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::INITIAL_USER_INFO),
                                .access = VehiclePropertyAccess::READ_WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::SWITCH_USER),
                                .access = VehiclePropertyAccess::READ_WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::CREATE_USER),
                                .access = VehiclePropertyAccess::READ_WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::REMOVE_USER),
                                .access = VehiclePropertyAccess::READ_WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::USER_IDENTIFICATION_ASSOCIATION),
                                .access = VehiclePropertyAccess::READ_WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::POWER_POLICY_REQ),
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::POWER_POLICY_GROUP_REQ),
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::CURRENT_POWER_POLICY),
                                .access = VehiclePropertyAccess::READ_WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::EPOCH_TIME),
                                .access = VehiclePropertyAccess::WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::WATCHDOG_ALIVE),
                                .access = VehiclePropertyAccess::WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::WATCHDOG_TERMINATED_PROCESS),
                                .access = VehiclePropertyAccess::WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::VHAL_HEARTBEAT),
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::CLUSTER_SWITCH_UI),
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.int32Values = {0 /* ClusterHome */}},
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::CLUSTER_DISPLAY_STATE),
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.int32Values = {0 /* Off */, -1, -1, -1, -1 /* Bounds */, -1, -1,
                                                 -1, -1 /* Insets */}},
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::CLUSTER_REPORT_STATE),
                                .access = VehiclePropertyAccess::WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                                .configArray = {0, 0, 0, 11, 0, 0, 0, 0, 16},
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::CLUSTER_REQUEST_DISPLAY),
                                .access = VehiclePropertyAccess::WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(VehicleProperty::CLUSTER_NAVIGATION_STATE),
                                .access = VehiclePropertyAccess::WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = PLACEHOLDER_PROPERTY_INT,
                                .access = VehiclePropertyAccess::READ_WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.int32Values = {0}},
        },
        {
                .config =
                        {
                                .prop = PLACEHOLDER_PROPERTY_FLOAT,
                                .access = VehiclePropertyAccess::READ_WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.floatValues = {0.0f}},
        },
        {
                .config =
                        {
                                .prop = PLACEHOLDER_PROPERTY_BOOLEAN,
                                .access = VehiclePropertyAccess::READ_WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.int32Values = {0 /* false */}},
        },
        {
                .config =
                        {
                                .prop = PLACEHOLDER_PROPERTY_STRING,
                                .access = VehiclePropertyAccess::READ_WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.stringValue = {"Test"}},
        },
        // This property is later defined in the AIDL VHAL interface. However, HIDL VHAL might
        // require support for this property to meet EU regulation.
        {
                .config =
                        {
                                // GENERAL_SAFETY_REGULATION_COMPLIANCE_REQUIREMENT
                                .prop = 0x11400F47,
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::STATIC,
                        },
                // GsrComplianceRequirementType::GSR_COMPLIANCE_REQUIRED_V1
                .initialValue = {.int32Values = {1}},
        },
#ifdef ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING
        // Vendor propetry for E2E ClusterHomeService testing.
        {
                .config =
                        {
                                .prop = VENDOR_CLUSTER_SWITCH_UI,
                                .access = VehiclePropertyAccess::WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = VENDOR_CLUSTER_DISPLAY_STATE,
                                .access = VehiclePropertyAccess::WRITE,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = VENDOR_CLUSTER_REPORT_STATE,
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                                .configArray = {0, 0, 0, 11, 0, 0, 0, 0, 16},
                        },
                .initialValue = {.int32Values = {0 /* Off */, -1, -1, -1, -1 /* Bounds */, -1, -1,
                                                 -1, -1 /* Insets */, 0 /* ClusterHome */,
                                                 -1 /* ClusterNone */}},
        },
        {
                .config =
                        {
                                .prop = VENDOR_CLUSTER_REQUEST_DISPLAY,
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.int32Values = {0 /* ClusterHome */}},
        },
        {
                .config =
                        {
                                .prop = VENDOR_CLUSTER_NAVIGATION_STATE,
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
#endif  // ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING
#ifdef ENABLE_GET_PROP_CONFIGS_BY_MULTIPLE_REQUESTS
        {
                .config =
                        {
                                // VHAL_SUPPORTED_PROPERTY_IDS
                                .prop = 289476424,
                                .access = VehiclePropertyAccess::READ,
                                .changeMode = VehiclePropertyChangeMode::STATIC,
                                // Fetch 100 configs in one request. This number is just arbitrarily
                                // chosen here. But some HAL impl with bigger config data may need a
                                // smaller number.
                                .configArray = {100},
                        },
                // All supported property IDs. This list is checked by
                // DefaultConfigSupportedPropertyIds_test.
                .initialValue =
                        {.int32Values =
                                 {291504388, 289472773, 291504390, 289472775, 289407240, 289407241,
                                  289472780, 286261505, 286261506, 289407235, 289472779, 291504647,
                                  289408517, 356518832, 356516106, 291504644, 291504649, 291504656,
                                  291504901, 291504903, 287310600, 291504905, 287310602, 287310603,
                                  291504908, 291504904, 392168201, 392168202, 289408514, 289408001,
                                  287310850, 287310851, 287310853, 289475088, 289475104, 289475120,
                                  354419984, 320865540, 320865556, 354419975, 354419976, 354419986,
                                  354419973, 354419974, 354419978, 354419977, 356517120, 356517121,
                                  356582673, 356517139, 289408269, 356517131, 358614275, 291570965,
                                  291505923, 289408270, 289408512, 287310855, 289408000, 289408008,
                                  289408009, 289407747, 291504900, 568332561, 371198722, 373295872,
                                  320867268, 322964416, 290521862, 287310858, 287310859, 289475072,
                                  289475073, 289409539, 299896064, 299896065, 299896066, 299896067,
                                  289410560, 289410561, 289410562, 289410563, 289410576, 289410577,
                                  289410578, 289410579, 289476368, 299895808, 639631617, 627048706,
                                  591397123, 554696964, 289410873, 289410874, 287313669, 299896583,
                                  299896584, 299896585, 299896586, 299896587, 286265121, 286265122,
                                  286265123, 290457094, 290459441, 299896626, 290459443, 289410868,
                                  289476405, 299896630, 289410871, 292556600, 557853201, 559950353,
                                  555756049, 554707473, 289410887, 557846324, 557911861, 568332086,
                                  557846327, 560992056, 289476424}},
        },
#endif  // ENABLE_GET_PROP_CONFIGS_BY_MULTIPLE_REQUESTS
};

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif // android_hardware_automotive_vehicle_V2_0_impl_DefaultConfig_H_
