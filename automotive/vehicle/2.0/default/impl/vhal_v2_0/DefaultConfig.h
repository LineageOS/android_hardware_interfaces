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
//
// Some handy constants to avoid conversions from enum to int.
constexpr int ABS_ACTIVE = (int)VehicleProperty::ABS_ACTIVE;
constexpr int AP_POWER_STATE_REQ = (int)VehicleProperty::AP_POWER_STATE_REQ;
constexpr int AP_POWER_STATE_REPORT = (int)VehicleProperty::AP_POWER_STATE_REPORT;
constexpr int DOOR_1_LEFT = (int)VehicleAreaDoor::ROW_1_LEFT;
constexpr int DOOR_1_RIGHT = (int)VehicleAreaDoor::ROW_1_RIGHT;
constexpr int DOOR_2_LEFT = (int)VehicleAreaDoor::ROW_2_LEFT;
constexpr int DOOR_2_RIGHT = (int)VehicleAreaDoor::ROW_2_RIGHT;
constexpr int WINDOW_1_LEFT = (int)VehicleAreaWindow::ROW_1_LEFT;
constexpr int WINDOW_1_RIGHT = (int)VehicleAreaWindow::ROW_1_RIGHT;
constexpr int WINDOW_2_LEFT = (int)VehicleAreaWindow::ROW_2_LEFT;
constexpr int WINDOW_2_RIGHT = (int)VehicleAreaWindow::ROW_2_RIGHT;
constexpr int FAN_DIRECTION_FACE = (int)VehicleHvacFanDirection::FACE;
constexpr int FAN_DIRECTION_FLOOR = (int)VehicleHvacFanDirection::FLOOR;
constexpr int OBD2_LIVE_FRAME = (int)VehicleProperty::OBD2_LIVE_FRAME;
constexpr int OBD2_FREEZE_FRAME = (int)VehicleProperty::OBD2_FREEZE_FRAME;
constexpr int OBD2_FREEZE_FRAME_INFO = (int)VehicleProperty::OBD2_FREEZE_FRAME_INFO;
constexpr int OBD2_FREEZE_FRAME_CLEAR = (int)VehicleProperty::OBD2_FREEZE_FRAME_CLEAR;
constexpr int TRACTION_CONTROL_ACTIVE = (int)VehicleProperty::TRACTION_CONTROL_ACTIVE;
constexpr int VEHICLE_MAP_SERVICE = (int)VehicleProperty::VEHICLE_MAP_SERVICE;
constexpr int WHEEL_TICK = (int)VehicleProperty::WHEEL_TICK;
constexpr int ALL_WHEELS =
    (int)(VehicleAreaWheel::LEFT_FRONT | VehicleAreaWheel::RIGHT_FRONT |
          VehicleAreaWheel::LEFT_REAR | VehicleAreaWheel::RIGHT_REAR);
constexpr int SEAT_1_LEFT = (int)(VehicleAreaSeat::ROW_1_LEFT);
constexpr int SEAT_1_RIGHT = (int)(VehicleAreaSeat::ROW_1_RIGHT);
constexpr int HVAC_LEFT = (int)(VehicleAreaSeat::ROW_1_LEFT | VehicleAreaSeat::ROW_2_LEFT |
                                VehicleAreaSeat::ROW_2_CENTER);
constexpr int HVAC_RIGHT = (int)(VehicleAreaSeat::ROW_1_RIGHT | VehicleAreaSeat::ROW_2_RIGHT);
constexpr int HVAC_ALL = HVAC_LEFT | HVAC_RIGHT;
constexpr int VENDOR_EXTENSION_BOOLEAN_PROPERTY =
    (int)(0x101 | VehiclePropertyGroup::VENDOR | VehiclePropertyType::BOOLEAN | VehicleArea::DOOR);
constexpr int VENDOR_EXTENSION_FLOAT_PROPERTY =
    (int)(0x102 | VehiclePropertyGroup::VENDOR | VehiclePropertyType::FLOAT | VehicleArea::SEAT);
constexpr int VENDOR_EXTENSION_INT_PROPERTY =
    (int)(0x103 | VehiclePropertyGroup::VENDOR | VehiclePropertyType::INT32 | VehicleArea::WINDOW);
constexpr int VENDOR_EXTENSION_STRING_PROPERTY =
    (int)(0x104 | VehiclePropertyGroup::VENDOR | VehiclePropertyType::STRING | VehicleArea::GLOBAL);
constexpr int FUEL_DOOR_REAR_LEFT = (int)PortLocationType::REAR_LEFT;
constexpr int CHARGE_PORT_FRONT_LEFT = (int)PortLocationType::FRONT_LEFT;
constexpr int LIGHT_STATE_ON = (int)VehicleLightState::ON;
constexpr int LIGHT_SWITCH_AUTO = (int)VehicleLightSwitch::AUTOMATIC;
constexpr int WHEEL_FRONT_LEFT = (int)VehicleAreaWheel::LEFT_FRONT;
constexpr int WHEEL_FRONT_RIGHT = (int)VehicleAreaWheel::RIGHT_FRONT;
constexpr int WHEEL_REAR_LEFT = (int)VehicleAreaWheel::LEFT_REAR;
constexpr int WHEEL_REAR_RIGHT = (int)VehicleAreaWheel::RIGHT_REAR;

/**
 * This property is used for test purpose to generate fake events. Here is the test package that
 * is referencing this property definition: packages/services/Car/tests/vehiclehal_test
 */
const int32_t kGenerateFakeDataControllingProperty =
    0x0666 | VehiclePropertyGroup::VENDOR | VehicleArea::GLOBAL | VehiclePropertyType::MIXED;

/**
 * FakeDataCommand enum defines the supported command type for kGenerateFakeDataControllingProperty.
 * All those commands can be send independently with each other. And each will override the one sent
 * previously.
 *
 * The controlling property has the following format:
 *
 *     int32Values[0] - command enum defined in FakeDataCommand
 *
 * The format of the arguments is defined for each command type as below:
 */
enum class FakeDataCommand : int32_t {
    /**
     * Starts linear fake data generation. Caller must provide additional data:
     *     int32Values[1] - vehicle property to which command applies
     *     int64Values[0] - periodic interval in nanoseconds
     *     floatValues[0] - initial value
     *     floatValues[1] - dispersion defines the min/max value relative to initial value, where
     *                      max = initial_value + dispersion, min = initial_value - dispersion.
     *                      Dispersion should be non-negative, otherwise the behavior is undefined.
     *     floatValues[2] - increment, with every timer tick the value will be incremented by this
     *                      amount. When reaching to max value, the current value will be set to
     *                      min. It should be non-negative, otherwise the behavior is undefined.
     */
    StartLinear = 0,

    /** Stops linear fake data generation that was triggered by StartLinear commands.
     *     int32Values[1] - vehicle property to which command applies. VHAL will stop the
     *                      corresponding linear generation for that property.
     */
    StopLinear = 1,

    /**
     * Starts JSON-based fake data generation. It iterates through JSON-encoded VHAL events from a
     * file and inject them to VHAL. The iteration can be repeated multiple times or infinitely.
     * Caller must provide additional data:
     *     int32Values[1] - number of iterations. If it is not provided or -1. The iteration will be
     *                      repeated infinite times.
     *     stringValue    - path to the fake values JSON file
     */
    StartJson = 2,

    /**
     * Stops JSON-based fake data generation. As multiple JSON-based generation can happen at the
     * same time. Caller must provide the path of fake value JSON file to stop the corresponding
     * generation:
     *     stringValue    - path to the fake values JSON file
     */
    StopJson = 3,

    /**
     * Injects key press event (HAL incorporates UP/DOWN acction and triggers 2 HAL events for every
     * key-press). We set the enum with high number to leave space for future start/stop commands.
     * Caller must provide the following data:
     *     int32Values[2] - Android key code
     *     int32Values[3] - target display (0 - for main display, 1 - for instrument cluster, see
     *                      VehicleDisplay)
     */
    KeyPress = 100,
};

const int32_t kHvacPowerProperties[] = {
    toInt(VehicleProperty::HVAC_FAN_SPEED),
    toInt(VehicleProperty::HVAC_FAN_DIRECTION),
};

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
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.floatValues = {15000.0f}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::INFO_FUEL_TYPE),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::STATIC,
         },
     .initialValue = {.int32Values = {1}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::INFO_EV_BATTERY_CAPACITY),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::STATIC,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.floatValues = {150000.0f}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::INFO_EV_CONNECTOR_TYPE),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::STATIC,
         },
     .initialValue = {.int32Values = {1}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::INFO_FUEL_DOOR_LOCATION),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::STATIC,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {FUEL_DOOR_REAR_LEFT}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::INFO_EV_PORT_LOCATION),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::STATIC,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {CHARGE_PORT_FRONT_LEFT}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::INFO_MAKE),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::STATIC,
         },
     .initialValue = {.stringValue = "Toy Vehicle"}},
    {.config =
         {
             .prop = toInt(VehicleProperty::PERF_VEHICLE_SPEED),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .minSampleRate = 1.0f,
             .maxSampleRate = 10.0f,
         },
     .initialValue = {.floatValues = {0.0f}}},

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
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
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
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.floatValues = {15000.0f}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::FUEL_DOOR_OPEN),
             .access = VehiclePropertyAccess::READ_WRITE,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {0}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::EV_BATTERY_LEVEL),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.floatValues = {150000.0f}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::EV_CHARGE_PORT_OPEN),
             .access = VehiclePropertyAccess::READ_WRITE,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {0}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::EV_CHARGE_PORT_CONNECTED),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {0}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::EV_BATTERY_INSTANTANEOUS_CHARGE_RATE),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.floatValues = {0.0f}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::RANGE_REMAINING),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.floatValues = {100.0f}}},  // units in meters

    {.config =
         {.prop = toInt(VehicleProperty::TIRE_PRESSURE),
          .access = VehiclePropertyAccess::READ,
          .changeMode = VehiclePropertyChangeMode::CONTINUOUS,
          .areaConfigs =
              {VehicleAreaConfig{
                   .areaId = WHEEL_FRONT_LEFT, .minFloatValue = 100.0f, .maxFloatValue = 300.0f,
               },
               VehicleAreaConfig{
                   .areaId = WHEEL_FRONT_RIGHT, .minFloatValue = 100.0f, .maxFloatValue = 300.0f,
               },
               VehicleAreaConfig{
                   .areaId = WHEEL_REAR_LEFT, .minFloatValue = 100.0f, .maxFloatValue = 300.0f,
               },
               VehicleAreaConfig{
                   .areaId = WHEEL_REAR_RIGHT, .minFloatValue = 100.0f, .maxFloatValue = 300.0f,
               }}},
     .initialValue = {.floatValues = {200}}},  // units in kPa

    {.config =
         {
             .prop = toInt(VehicleProperty::CURRENT_GEAR),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
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
             .prop = toInt(VehicleProperty::FUEL_LEVEL_LOW),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {0}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::HW_KEY_INPUT),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
         },
     .initialValue = {.int32Values = {0, 0, 0}}},

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
                       {VehicleAreaConfig{.areaId = toInt(VehicleAreaWindow::FRONT_WINDSHIELD)},
                        VehicleAreaConfig{.areaId = toInt(VehicleAreaWindow::REAR_WINDSHIELD)}}},
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
                                      FAN_DIRECTION_FACE | FAN_DIRECTION_FLOOR}}},

    {.config = {.prop = toInt(VehicleProperty::HVAC_SEAT_VENTILATION),
                .access = VehiclePropertyAccess::READ_WRITE,
                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                .areaConfigs = {VehicleAreaConfig{
                                    .areaId = SEAT_1_LEFT, .minInt32Value = 0, .maxInt32Value = 3,
                                },
                                VehicleAreaConfig{
                                    .areaId = SEAT_1_RIGHT, .minInt32Value = 0, .maxInt32Value = 3,
                                }}},
     .initialValue = {.int32Values = {0}}},  // 0 is off and +ve values indicate ventilation level.

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
                                    .areaId = SEAT_1_LEFT, .minInt32Value = -2, .maxInt32Value = 2,
                                },
                                VehicleAreaConfig{
                                    .areaId = SEAT_1_RIGHT, .minInt32Value = -2, .maxInt32Value = 2,
                                }}},
     .initialValue = {.int32Values = {0}}},  // +ve values for heating and -ve for cooling

    {.config = {.prop = toInt(VehicleProperty::HVAC_TEMPERATURE_SET),
                .access = VehiclePropertyAccess::READ_WRITE,
                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
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
                .areaConfigs = {VehicleAreaConfig{.areaId = (0)}}},
     .initialValue = {.int32Values = {(int)VehicleUnit::FAHRENHEIT}}},

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
         },
     .initialValue = {.int32Values = {toInt(VehicleGear::GEAR_PARK)}}},

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
        .config =
            {
                .prop = kGenerateFakeDataControllingProperty,
                .access = VehiclePropertyAccess::WRITE,
                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
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
                .areaConfigs = {VehicleAreaConfig{.areaId = DOOR_1_LEFT},
                                VehicleAreaConfig{.areaId = DOOR_1_RIGHT},
                                VehicleAreaConfig{.areaId = DOOR_2_LEFT},
                                VehicleAreaConfig{.areaId = DOOR_2_RIGHT}}},
     .initialAreaValues = {{DOOR_1_LEFT, {.int32Values = {0}}},
                           {DOOR_1_RIGHT, {.int32Values = {0}}},
                           {DOOR_2_LEFT, {.int32Values = {0}}},
                           {DOOR_2_RIGHT, {.int32Values = {0}}}}},

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
                .areaConfigs = {VehicleAreaConfig{.areaId = WINDOW_1_LEFT},
                                VehicleAreaConfig{.areaId = WINDOW_1_RIGHT},
                                VehicleAreaConfig{.areaId = WINDOW_2_LEFT},
                                VehicleAreaConfig{.areaId = WINDOW_2_RIGHT}}},
     .initialAreaValues = {{WINDOW_1_LEFT, {.int32Values = {0}}},
                           {WINDOW_1_RIGHT, {.int32Values = {0}}},
                           {WINDOW_2_LEFT, {.int32Values = {0}}},
                           {WINDOW_2_RIGHT, {.int32Values = {0}}}}},

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
                .access = VehiclePropertyAccess::WRITE,
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
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::HIGH_BEAM_LIGHTS_STATE),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::FOG_LIGHTS_STATE),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::HAZARD_LIGHTS_STATE),
             .access = VehiclePropertyAccess::READ,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {LIGHT_STATE_ON}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::HEADLIGHTS_SWITCH),
             .access = VehiclePropertyAccess::READ_WRITE,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::HIGH_BEAM_LIGHTS_SWITCH),
             .access = VehiclePropertyAccess::READ_WRITE,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::FOG_LIGHTS_SWITCH),
             .access = VehiclePropertyAccess::READ_WRITE,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

    {.config =
         {
             .prop = toInt(VehicleProperty::HAZARD_LIGHTS_SWITCH),
             .access = VehiclePropertyAccess::READ_WRITE,
             .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
             .areaConfigs = {VehicleAreaConfig{.areaId = (0)}},
         },
     .initialValue = {.int32Values = {LIGHT_SWITCH_AUTO}}},

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
                .areaConfigs = {VehicleAreaConfig{
                                    .areaId = HVAC_LEFT, .minFloatValue = -10, .maxFloatValue = 10},
                                VehicleAreaConfig{.areaId = HVAC_RIGHT,
                                                  .minFloatValue = -10,
                                                  .maxFloatValue = 10}}},
     .initialAreaValues = {{HVAC_LEFT, {.floatValues = {1}}}, {HVAC_RIGHT, {.floatValues = {2}}}}},

    {.config = {.prop = VENDOR_EXTENSION_INT_PROPERTY,
                .access = VehiclePropertyAccess::READ_WRITE,
                .changeMode = VehiclePropertyChangeMode::ON_CHANGE,
                .areaConfigs = {VehicleAreaConfig{
                                    .areaId = (int)VehicleAreaWindow::FRONT_WINDSHIELD,
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
};

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif // android_hardware_automotive_vehicle_V2_0_impl_DefaultConfig_H_
