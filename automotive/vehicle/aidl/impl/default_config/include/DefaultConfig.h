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

#ifndef android_hardware_automotive_vehicle_aidl_impl_default_config_include_DefaultConfig_H_
#define android_hardware_automotive_vehicle_aidl_impl_default_config_include_DefaultConfig_H_

#include <PropertyUtils.h>
#include <TestPropertyUtils.h>
#include <VehicleHalTypes.h>

#include <map>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// Types used in configs, not to be exposed as public API.
namespace defaultconfig_impl {

using ::aidl::android::hardware::automotive::vehicle::EvConnectorType;
using ::aidl::android::hardware::automotive::vehicle::EvsServiceState;
using ::aidl::android::hardware::automotive::vehicle::EvsServiceType;
using ::aidl::android::hardware::automotive::vehicle::FuelType;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReport;
using ::aidl::android::hardware::automotive::vehicle::VehicleApPowerStateReq;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaWindow;
using ::aidl::android::hardware::automotive::vehicle::VehicleGear;
using ::aidl::android::hardware::automotive::vehicle::VehicleHvacFanDirection;
using ::aidl::android::hardware::automotive::vehicle::VehicleIgnitionState;
using ::aidl::android::hardware::automotive::vehicle::VehicleOilLevel;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropConfig;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyAccess;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyChangeMode;
using ::aidl::android::hardware::automotive::vehicle::VehicleSeatOccupancyState;
using ::aidl::android::hardware::automotive::vehicle::VehicleTurnSignal;
using ::aidl::android::hardware::automotive::vehicle::VehicleUnit;
using ::aidl::android::hardware::automotive::vehicle::VehicleVendorPermission;

}  // namespace defaultconfig_impl

struct ConfigDeclaration {
    defaultconfig_impl::VehiclePropConfig config;

    // This value will be used as an initial value for the property. If this field is specified for
    // property that supports multiple areas then it will be used for all areas unless particular
    // area is overridden in initialAreaValue field.
    ::aidl::android::hardware::automotive::vehicle::RawPropValues initialValue;
    // Use initialAreaValues if it is necessary to specify different values per each area.
    std::map<int32_t, ::aidl::android::hardware::automotive::vehicle::RawPropValues>
            initialAreaValues;
};

const ConfigDeclaration kVehicleProperties[]{
        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::INFO_FUEL_CAPACITY),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.floatValues = {15000.0f}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::INFO_FUEL_TYPE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {toInt(
                                  defaultconfig_impl::FuelType::FUEL_TYPE_UNLEADED)}}},

        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::INFO_EV_BATTERY_CAPACITY),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.floatValues = {150000.0f}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::INFO_EV_CONNECTOR_TYPE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {toInt(
                                  defaultconfig_impl::EvConnectorType::IEC_TYPE_1_AC)}}},

        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::INFO_FUEL_DOOR_LOCATION),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {FUEL_DOOR_REAR_LEFT}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::INFO_EV_PORT_LOCATION),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {CHARGE_PORT_FRONT_LEFT}}},

        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::INFO_MULTI_EV_PORT_LOCATIONS),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {CHARGE_PORT_FRONT_LEFT, CHARGE_PORT_REAR_LEFT}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::INFO_MAKE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.stringValue = "Toy Vehicle"}},
        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::INFO_MODEL),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.stringValue = "Speedy Model"}},
        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::INFO_MODEL_YEAR),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {2020}}},
        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::INFO_EXTERIOR_DIMENSIONS),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialValue = {.int32Values = {1776, 4950, 2008, 2140, 2984, 1665, 1667, 11800}}},
        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::PERF_VEHICLE_SPEED),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.floatValues = {0.0f}}},

        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::VEHICLE_SPEED_DISPLAY_UNITS),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray =
                                 {toInt(defaultconfig_impl::VehicleUnit::METER_PER_SEC),
                                  toInt(defaultconfig_impl::VehicleUnit::MILES_PER_HOUR),
                                  toInt(defaultconfig_impl::VehicleUnit::KILOMETERS_PER_HOUR)},
                 },
         .initialValue = {.int32Values = {toInt(
                                  defaultconfig_impl::VehicleUnit::KILOMETERS_PER_HOUR)}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::SEAT_OCCUPANCY),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                         .areaConfigs =
                                 {defaultconfig_impl::VehicleAreaConfig{.areaId = (SEAT_1_LEFT)},
                                  defaultconfig_impl::VehicleAreaConfig{.areaId = (SEAT_1_RIGHT)}},
                 },
         .initialAreaValues =
                 {{SEAT_1_LEFT,
                   {.int32Values = {toInt(defaultconfig_impl::VehicleSeatOccupancyState::VACANT)}}},
                  {SEAT_1_RIGHT,
                   {.int32Values = {toInt(
                            defaultconfig_impl::VehicleSeatOccupancyState::VACANT)}}}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::INFO_DRIVER_SEAT),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                         // this was a zoned property on an old vhal, but it is meant to be global
                         .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = (0)}},
                 },
         .initialValue = {.int32Values = {SEAT_1_LEFT}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::PERF_ODOMETER),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.floatValues = {0.0f}}},
        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::PERF_STEERING_ANGLE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.floatValues = {0.0f}}},
        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::PERF_REAR_STEERING_ANGLE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.floatValues = {0.0f}}},
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::ENGINE_RPM),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                                .minSampleRate = 1.0f,
                                .maxSampleRate = 10.0f,
                        },
                .initialValue = {.floatValues = {0.0f}},
        },

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::FUEL_LEVEL),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 100.0f,
                 },
         .initialValue = {.floatValues = {15000.0f}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::FUEL_DOOR_OPEN),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::EV_BATTERY_LEVEL),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 100.0f,
                 },
         .initialValue = {.floatValues = {150000.0f}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::EV_CHARGE_PORT_OPEN),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::EV_CHARGE_PORT_CONNECTED),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::
                                               EV_BATTERY_INSTANTANEOUS_CHARGE_RATE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.floatValues = {0.0f}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::RANGE_REMAINING),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 2.0f,
                 },
         .initialValue = {.floatValues = {50000.0f}}},  // units in meters

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::TIRE_PRESSURE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{
                                                 .areaId = WHEEL_FRONT_LEFT,
                                                 .minFloatValue = 193.0f,
                                                 .maxFloatValue = 300.0f,
                                         },
                                         defaultconfig_impl::VehicleAreaConfig{
                                                 .areaId = WHEEL_FRONT_RIGHT,
                                                 .minFloatValue = 193.0f,
                                                 .maxFloatValue = 300.0f,
                                         },
                                         defaultconfig_impl::VehicleAreaConfig{
                                                 .areaId = WHEEL_REAR_LEFT,
                                                 .minFloatValue = 193.0f,
                                                 .maxFloatValue = 300.0f,
                                         },
                                         defaultconfig_impl::VehicleAreaConfig{
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
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::CRITICALLY_LOW_TIRE_PRESSURE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                 },
         .initialAreaValues = {{WHEEL_FRONT_LEFT, {.floatValues = {137.0f}}},
                               {WHEEL_FRONT_RIGHT, {.floatValues = {137.0f}}},
                               {WHEEL_REAR_RIGHT, {.floatValues = {137.0f}}},
                               {WHEEL_REAR_LEFT, {.floatValues = {137.0f}}}}},

        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::TIRE_PRESSURE_DISPLAY_UNITS),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray = {toInt(defaultconfig_impl::VehicleUnit::KILOPASCAL),
                                         toInt(defaultconfig_impl::VehicleUnit::PSI),
                                         toInt(defaultconfig_impl::VehicleUnit::BAR)},
                 },
         .initialValue = {.int32Values = {toInt(defaultconfig_impl::VehicleUnit::PSI)}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::CURRENT_GEAR),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray = {toInt(defaultconfig_impl::VehicleGear::GEAR_PARK),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_NEUTRAL),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_REVERSE),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_1),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_2),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_3),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_4),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_5)},
                 },
         .initialValue = {.int32Values = {toInt(defaultconfig_impl::VehicleGear::GEAR_PARK)}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::PARKING_BRAKE_ON),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {1}}},

        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::PARKING_BRAKE_AUTO_APPLY),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {1}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::FUEL_LEVEL_LOW),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::HW_KEY_INPUT),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0, 0, 0}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::HW_ROTARY_INPUT),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0, 0, 0}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::HW_CUSTOM_INPUT),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray = {0, 0, 0, 3, 0, 0, 0, 0, 0},
                 },
         .initialValue =
                 {
                         .int32Values = {0, 0, 0},
                 }},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_POWER_ON),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_ALL}},
                    // TODO(bryaneyler): Ideally, this is generated dynamically from
                    // kHvacPowerProperties.
                    .configArray =
                            {toInt(defaultconfig_impl::VehicleProperty::HVAC_FAN_SPEED),
                             toInt(defaultconfig_impl::VehicleProperty::HVAC_FAN_DIRECTION)}},
         .initialValue = {.int32Values = {1}}},

        {
                .config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_DEFROSTER),
                           .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                           .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                           .areaConfigs =
                                   {defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = toInt(defaultconfig_impl::VehicleAreaWindow::
                                                                    FRONT_WINDSHIELD)},
                                    defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = toInt(defaultconfig_impl::VehicleAreaWindow::
                                                                    REAR_WINDSHIELD)}}},
                .initialValue = {.int32Values = {0}}  // Will be used for all areas.
        },
        {
                .config = {.prop = toInt(
                                   defaultconfig_impl::VehicleProperty::HVAC_ELECTRIC_DEFROSTER_ON),
                           .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                           .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                           .areaConfigs =
                                   {defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = toInt(defaultconfig_impl::VehicleAreaWindow::
                                                                    FRONT_WINDSHIELD)},
                                    defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = toInt(defaultconfig_impl::VehicleAreaWindow::
                                                                    REAR_WINDSHIELD)}}},
                .initialValue = {.int32Values = {0}}  // Will be used for all areas.
        },

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_MAX_DEFROST_ON),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_RECIRC_ON),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {1}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_AUTO_RECIRC_ON),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_AC_ON),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {1}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_MAX_AC_ON),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_AUTO_ON),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {1}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_DUAL_ON),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_FAN_SPEED),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{
                            .areaId = HVAC_ALL, .minInt32Value = 1, .maxInt32Value = 7}}},
         .initialValue = {.int32Values = {3}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_FAN_DIRECTION),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {toInt(
                                  defaultconfig_impl::VehicleHvacFanDirection::FACE)}}},

        {.config = {.prop = toInt(
                            defaultconfig_impl::VehicleProperty::HVAC_FAN_DIRECTION_AVAILABLE),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::STATIC,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_ALL}}},
         .initialValue = {.int32Values = {FAN_DIRECTION_FACE, FAN_DIRECTION_FLOOR,
                                          FAN_DIRECTION_FACE | FAN_DIRECTION_FLOOR,
                                          FAN_DIRECTION_DEFROST,
                                          FAN_DIRECTION_FACE | FAN_DIRECTION_DEFROST,
                                          FAN_DIRECTION_FLOOR | FAN_DIRECTION_DEFROST,
                                          FAN_DIRECTION_FLOOR | FAN_DIRECTION_DEFROST |
                                                  FAN_DIRECTION_FACE}}},
        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_SEAT_VENTILATION),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = SEAT_1_LEFT,
                                            .minInt32Value = 0,
                                            .maxInt32Value = 3,
                                    },
                                    defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = SEAT_1_RIGHT,
                                            .minInt32Value = 0,
                                            .maxInt32Value = 3,
                                    }}},
         .initialValue =
                 {.int32Values = {0}}},  // 0 is off and +ve values indicate ventilation level.

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_STEERING_WHEEL_HEAT),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{
                            .areaId = (0), .minInt32Value = -2, .maxInt32Value = 2}}},
         .initialValue = {.int32Values = {0}}},  // +ve values for heating and -ve for cooling

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_SEAT_TEMPERATURE),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = SEAT_1_LEFT,
                                            .minInt32Value = -2,
                                            .maxInt32Value = 2,
                                    },
                                    defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = SEAT_1_RIGHT,
                                            .minInt32Value = -2,
                                            .maxInt32Value = 2,
                                    }}},
         .initialValue = {.int32Values = {0}}},  // +ve values for heating and -ve for cooling

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::HVAC_TEMPERATURE_SET),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .configArray = {160, 280, 5, 605, 825, 10},
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = HVAC_LEFT,
                                            .minFloatValue = 16,
                                            .maxFloatValue = 32,
                                    },
                                    defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = HVAC_RIGHT,
                                            .minFloatValue = 16,
                                            .maxFloatValue = 32,
                                    }}},
         .initialAreaValues = {{HVAC_LEFT, {.floatValues = {16}}},
                               {HVAC_RIGHT, {.floatValues = {20}}}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::
                                               HVAC_TEMPERATURE_VALUE_SUGGESTION),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.floatValues = {66.2f, (float)defaultconfig_impl::VehicleUnit::FAHRENHEIT,
                                          19.0f, 66.5f}}},

        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::ENV_OUTSIDE_TEMPERATURE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         // TODO(bryaneyler): Support ON_CHANGE as well.
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 2.0f,
                 },
         .initialValue = {.floatValues = {25.0f}}},

        {.config = {.prop = toInt(
                            defaultconfig_impl::VehicleProperty::HVAC_TEMPERATURE_DISPLAY_UNITS),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .configArray = {toInt(defaultconfig_impl::VehicleUnit::FAHRENHEIT),
                                    toInt(defaultconfig_impl::VehicleUnit::CELSIUS)}},
         .initialValue = {.int32Values = {toInt(defaultconfig_impl::VehicleUnit::FAHRENHEIT)}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::DISTANCE_DISPLAY_UNITS),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                         .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = (0)}},
                         .configArray = {toInt(defaultconfig_impl::VehicleUnit::KILOMETER),
                                         toInt(defaultconfig_impl::VehicleUnit::MILE)},
                 },
         .initialValue = {.int32Values = {toInt(defaultconfig_impl::VehicleUnit::MILE)}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::NIGHT_MODE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::GEAR_SELECTION),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray = {toInt(defaultconfig_impl::VehicleGear::GEAR_PARK),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_NEUTRAL),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_REVERSE),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_DRIVE),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_1),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_2),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_3),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_4),
                                         toInt(defaultconfig_impl::VehicleGear::GEAR_5)},
                 },
         .initialValue = {.int32Values = {toInt(defaultconfig_impl::VehicleGear::GEAR_PARK)}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::TURN_SIGNAL_STATE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {toInt(defaultconfig_impl::VehicleTurnSignal::NONE)}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::IGNITION_STATE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {toInt(defaultconfig_impl::VehicleIgnitionState::ON)}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::ENGINE_OIL_LEVEL),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {toInt(defaultconfig_impl::VehicleOilLevel::NORMAL)}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::ENGINE_OIL_TEMP),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .minSampleRate = 0.1,  // 0.1 Hz, every 10 seconds
                         .maxSampleRate = 10,   // 10 Hz, every 100 ms
                 },
         .initialValue = {.floatValues = {101.0f}}},

        {
                .config = {.prop = kMixedTypePropertyForTest,
                           .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                           .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                           .configArray = {1, 1, 0, 2, 0, 0, 1, 0, 0}},
                .initialValue =
                        {
                                .int32Values = {1 /* indicate TRUE boolean value */, 2, 3},
                                .floatValues = {4.5f},
                                .stringValue = "MIXED property",
                        },
        },

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::DOOR_LOCK),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = DOOR_1_LEFT},
                                    defaultconfig_impl::VehicleAreaConfig{.areaId = DOOR_1_RIGHT},
                                    defaultconfig_impl::VehicleAreaConfig{.areaId = DOOR_2_LEFT},
                                    defaultconfig_impl::VehicleAreaConfig{.areaId = DOOR_2_RIGHT}}},
         .initialAreaValues = {{DOOR_1_LEFT, {.int32Values = {1}}},
                               {DOOR_1_RIGHT, {.int32Values = {1}}},
                               {DOOR_2_LEFT, {.int32Values = {1}}},
                               {DOOR_2_RIGHT, {.int32Values = {1}}}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::DOOR_POS),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs =
                            {defaultconfig_impl::VehicleAreaConfig{
                                     .areaId = DOOR_1_LEFT, .minInt32Value = 0, .maxInt32Value = 1},
                             defaultconfig_impl::VehicleAreaConfig{.areaId = DOOR_1_RIGHT,
                                                                   .minInt32Value = 0,
                                                                   .maxInt32Value = 1},
                             defaultconfig_impl::VehicleAreaConfig{
                                     .areaId = DOOR_2_LEFT, .minInt32Value = 0, .maxInt32Value = 1},
                             defaultconfig_impl::VehicleAreaConfig{.areaId = DOOR_2_RIGHT,
                                                                   .minInt32Value = 0,
                                                                   .maxInt32Value = 1},
                             defaultconfig_impl::VehicleAreaConfig{
                                     .areaId = DOOR_REAR, .minInt32Value = 0, .maxInt32Value = 1}}},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::WINDOW_LOCK),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{
                            .areaId = WINDOW_1_RIGHT | WINDOW_2_LEFT | WINDOW_2_RIGHT}}},
         .initialAreaValues = {{WINDOW_1_RIGHT | WINDOW_2_LEFT | WINDOW_2_RIGHT,
                                {.int32Values = {0}}}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::WINDOW_POS),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = WINDOW_1_LEFT,
                                                                          .minInt32Value = 0,
                                                                          .maxInt32Value = 10},
                                    defaultconfig_impl::VehicleAreaConfig{.areaId = WINDOW_1_RIGHT,
                                                                          .minInt32Value = 0,
                                                                          .maxInt32Value = 10},
                                    defaultconfig_impl::VehicleAreaConfig{.areaId = WINDOW_2_LEFT,
                                                                          .minInt32Value = 0,
                                                                          .maxInt32Value = 10},
                                    defaultconfig_impl::VehicleAreaConfig{.areaId = WINDOW_2_RIGHT,
                                                                          .minInt32Value = 0,
                                                                          .maxInt32Value = 10},
                                    defaultconfig_impl::VehicleAreaConfig{
                                            .areaId = WINDOW_ROOF_TOP_1,
                                            .minInt32Value = -10,
                                            .maxInt32Value = 10}}},
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = WHEEL_TICK,
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::CONTINUOUS,
                         .configArray = {ALL_WHEELS, 50000, 50000, 50000, 50000},
                         .minSampleRate = 1.0f,
                         .maxSampleRate = 10.0f,
                 },
         .initialValue = {.int64Values = {0, 100000, 200000, 300000, 400000}}},

        {.config = {.prop = ABS_ACTIVE,
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = TRACTION_CONTROL_ACTIVE,
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::AP_POWER_STATE_REQ),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .configArray = {3}},
         .initialValue = {.int32Values = {toInt(defaultconfig_impl::VehicleApPowerStateReq::ON),
                                          0}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::AP_POWER_STATE_REPORT),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue =
                 {.int32Values =
                          {toInt(defaultconfig_impl::VehicleApPowerStateReport::WAIT_FOR_VHAL),
                           0}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::DISPLAY_BRIGHTNESS),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.minInt32Value = 0,
                                                                          .maxInt32Value = 100}}},
         .initialValue = {.int32Values = {100}}},

        {
                .config = {.prop = OBD2_LIVE_FRAME,
                           .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                           .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                           .configArray = {0, 0}},
        },

        {
                .config = {.prop = OBD2_FREEZE_FRAME,
                           .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                           .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                           .configArray = {0, 0}},
        },

        {
                .config = {.prop = OBD2_FREEZE_FRAME_INFO,
                           .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                           .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE},
        },

        {
                .config = {.prop = OBD2_FREEZE_FRAME_CLEAR,
                           .access = defaultconfig_impl::VehiclePropertyAccess::WRITE,
                           .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                           .configArray = {1}},
        },

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::HEADLIGHTS_STATE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::HIGH_BEAM_LIGHTS_STATE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::FOG_LIGHTS_STATE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::HAZARD_LIGHTS_STATE),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::HEADLIGHTS_SWITCH),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

        {.config =
                 {
                         .prop = toInt(
                                 defaultconfig_impl::VehicleProperty::HIGH_BEAM_LIGHTS_SWITCH),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::FOG_LIGHTS_SWITCH),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::HAZARD_LIGHTS_SWITCH),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::EVS_SERVICE_REQUEST),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                 },
         .initialValue = {.int32Values = {toInt(defaultconfig_impl::EvsServiceType::REARVIEW),
                                          toInt(defaultconfig_impl::EvsServiceState::OFF)}}},

        {.config = {.prop = VEHICLE_MAP_SERVICE,
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE}},

        // Example Vendor Extension properties for testing
        {.config = {.prop = VENDOR_EXTENSION_BOOLEAN_PROPERTY,
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = DOOR_1_LEFT},
                                    defaultconfig_impl::VehicleAreaConfig{.areaId = DOOR_1_RIGHT},
                                    defaultconfig_impl::VehicleAreaConfig{.areaId = DOOR_2_LEFT},
                                    defaultconfig_impl::VehicleAreaConfig{.areaId = DOOR_2_RIGHT}}},
         .initialAreaValues = {{DOOR_1_LEFT, {.int32Values = {1}}},
                               {DOOR_1_RIGHT, {.int32Values = {1}}},
                               {DOOR_2_LEFT, {.int32Values = {0}}},
                               {DOOR_2_RIGHT, {.int32Values = {0}}}}},

        {.config = {.prop = VENDOR_EXTENSION_FLOAT_PROPERTY,
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs = {defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_LEFT,
                                                                          .minFloatValue = -10,
                                                                          .maxFloatValue = 10},
                                    defaultconfig_impl::VehicleAreaConfig{.areaId = HVAC_RIGHT,
                                                                          .minFloatValue = -10,
                                                                          .maxFloatValue = 10}}},
         .initialAreaValues = {{HVAC_LEFT, {.floatValues = {1}}},
                               {HVAC_RIGHT, {.floatValues = {2}}}}},

        {.config = {.prop = VENDOR_EXTENSION_INT_PROPERTY,
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                    .areaConfigs =
                            {defaultconfig_impl::VehicleAreaConfig{
                                     .areaId = toInt(defaultconfig_impl::VehicleAreaWindow::
                                                             FRONT_WINDSHIELD),
                                     .minInt32Value = -100,
                                     .maxInt32Value = 100},
                             defaultconfig_impl::VehicleAreaConfig{
                                     .areaId = toInt(defaultconfig_impl::VehicleAreaWindow::
                                                             REAR_WINDSHIELD),
                                     .minInt32Value = -100,
                                     .maxInt32Value = 100},
                             defaultconfig_impl::VehicleAreaConfig{
                                     .areaId = toInt(
                                             defaultconfig_impl::VehicleAreaWindow::ROOF_TOP_1),
                                     .minInt32Value = -100,
                                     .maxInt32Value = 100}}},
         .initialAreaValues = {{toInt(defaultconfig_impl::VehicleAreaWindow::FRONT_WINDSHIELD),
                                {.int32Values = {1}}},
                               {toInt(defaultconfig_impl::VehicleAreaWindow::REAR_WINDSHIELD),
                                {.int32Values = {0}}},
                               {toInt(defaultconfig_impl::VehicleAreaWindow::ROOF_TOP_1),
                                {.int32Values = {-1}}}}},

        {.config = {.prop = VENDOR_EXTENSION_STRING_PROPERTY,
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.stringValue = "Vendor String Property"}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::
                                          ELECTRONIC_TOLL_COLLECTION_CARD_TYPE),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.int32Values = {0}}},

        {.config = {.prop = toInt(defaultconfig_impl::VehicleProperty::
                                          ELECTRONIC_TOLL_COLLECTION_CARD_STATUS),
                    .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                    .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE},
         .initialValue = {.int32Values = {0}}},

        {.config =
                 {
                         .prop = toInt(defaultconfig_impl::VehicleProperty::
                                               SUPPORT_CUSTOMIZE_VENDOR_PERMISSION),
                         .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                         .changeMode = defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                         .configArray =
                                 {kMixedTypePropertyForTest,
                                  toInt(defaultconfig_impl::
                                                VehicleVendorPermission::
                                                        PERMISSION_GET_VENDOR_CATEGORY_INFO),
                                  toInt(defaultconfig_impl::
                                                VehicleVendorPermission::
                                                        PERMISSION_SET_VENDOR_CATEGORY_INFO),
                                  VENDOR_EXTENSION_INT_PROPERTY,
                                  toInt(defaultconfig_impl::
                                                VehicleVendorPermission::
                                                        PERMISSION_GET_VENDOR_CATEGORY_SEAT),
                                  toInt(defaultconfig_impl::
                                                VehicleVendorPermission::PERMISSION_NOT_ACCESSIBLE),
                                  VENDOR_EXTENSION_FLOAT_PROPERTY,
                                  toInt(defaultconfig_impl::
                                                VehicleVendorPermission::PERMISSION_DEFAULT),
                                  toInt(defaultconfig_impl::
                                                VehicleVendorPermission::PERMISSION_DEFAULT)},
                 },
         .initialValue = {.int32Values = {1}}},

        {
                .config =
                        {
                                .prop = toInt(
                                        defaultconfig_impl::VehicleProperty::INITIAL_USER_INFO),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::SWITCH_USER),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::CREATE_USER),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::REMOVE_USER),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::
                                                      USER_IDENTIFICATION_ASSOCIATION),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(
                                        defaultconfig_impl::VehicleProperty::POWER_POLICY_REQ),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::
                                                      POWER_POLICY_GROUP_REQ),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(
                                        defaultconfig_impl::VehicleProperty::CURRENT_POWER_POLICY),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::EPOCH_TIME),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::WATCHDOG_ALIVE),
                                .access = defaultconfig_impl::VehiclePropertyAccess::WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::
                                                      WATCHDOG_TERMINATED_PROCESS),
                                .access = defaultconfig_impl::VehiclePropertyAccess::WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::VHAL_HEARTBEAT),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(
                                        defaultconfig_impl::VehicleProperty::CLUSTER_SWITCH_UI),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.int32Values = {0 /* ClusterHome */, -1 /* ClusterNone */}},
        },
        {
                .config =
                        {
                                .prop = toInt(
                                        defaultconfig_impl::VehicleProperty::CLUSTER_DISPLAY_STATE),
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.int32Values = {0 /* Off */, -1, -1, -1, -1 /* Bounds */, -1, -1,
                                                 -1, -1 /* Insets */}},
        },
        {
                .config =
                        {
                                .prop = toInt(
                                        defaultconfig_impl::VehicleProperty::CLUSTER_REPORT_STATE),
                                .access = defaultconfig_impl::VehiclePropertyAccess::WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                                .configArray = {0, 0, 0, 11, 0, 0, 0, 0, 16},
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::
                                                      CLUSTER_REQUEST_DISPLAY),
                                .access = defaultconfig_impl::VehiclePropertyAccess::WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = toInt(defaultconfig_impl::VehicleProperty::
                                                      CLUSTER_NAVIGATION_STATE),
                                .access = defaultconfig_impl::VehiclePropertyAccess::WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = PLACEHOLDER_PROPERTY_INT,
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.int32Values = {0}},
        },
        {
                .config =
                        {
                                .prop = PLACEHOLDER_PROPERTY_FLOAT,
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.floatValues = {0.0f}},
        },
        {
                .config =
                        {
                                .prop = PLACEHOLDER_PROPERTY_BOOLEAN,
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.int32Values = {0 /* false */}},
        },
        {
                .config =
                        {
                                .prop = PLACEHOLDER_PROPERTY_STRING,
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ_WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.stringValue = {"Test"}},
        },
#ifdef ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING
        // Vendor propetry for E2E ClusterHomeService testing.
        {
                .config =
                        {
                                .prop = VENDOR_CLUSTER_SWITCH_UI,
                                .access = defaultconfig_impl::VehiclePropertyAccess::WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = VENDOR_CLUSTER_DISPLAY_STATE,
                                .access = defaultconfig_impl::VehiclePropertyAccess::WRITE,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
        {
                .config =
                        {
                                .prop = VENDOR_CLUSTER_REPORT_STATE,
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
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
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
                .initialValue = {.int32Values = {0 /* ClusterHome */}},
        },
        {
                .config =
                        {
                                .prop = VENDOR_CLUSTER_NAVIGATION_STATE,
                                .access = defaultconfig_impl::VehiclePropertyAccess::READ,
                                .changeMode =
                                        defaultconfig_impl::VehiclePropertyChangeMode::ON_CHANGE,
                        },
        },
#endif  // ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_default_config_include_DefaultConfig_H_
