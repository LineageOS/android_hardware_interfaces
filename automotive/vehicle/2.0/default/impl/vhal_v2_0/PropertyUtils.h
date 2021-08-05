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

#pragma once

#include <android/hardware/automotive/vehicle/2.0/types.h>
#include <vhal_v2_0/VehicleUtils.h>

namespace android::hardware::automotive::vehicle::V2_0::impl {

// Some handy constants to avoid conversions from enum to int.
constexpr int ABS_ACTIVE = (int)VehicleProperty::ABS_ACTIVE;
constexpr int AP_POWER_STATE_REQ = (int)VehicleProperty::AP_POWER_STATE_REQ;
constexpr int AP_POWER_STATE_REPORT = (int)VehicleProperty::AP_POWER_STATE_REPORT;
constexpr int DOOR_1_LEFT = (int)VehicleAreaDoor::ROW_1_LEFT;
constexpr int DOOR_1_RIGHT = (int)VehicleAreaDoor::ROW_1_RIGHT;
constexpr int DOOR_2_LEFT = (int)VehicleAreaDoor::ROW_2_LEFT;
constexpr int DOOR_2_RIGHT = (int)VehicleAreaDoor::ROW_2_RIGHT;
constexpr int DOOR_REAR = (int)VehicleAreaDoor::REAR;
constexpr int WINDOW_1_LEFT = (int)VehicleAreaWindow::ROW_1_LEFT;
constexpr int WINDOW_1_RIGHT = (int)VehicleAreaWindow::ROW_1_RIGHT;
constexpr int WINDOW_2_LEFT = (int)VehicleAreaWindow::ROW_2_LEFT;
constexpr int WINDOW_2_RIGHT = (int)VehicleAreaWindow::ROW_2_RIGHT;
constexpr int WINDOW_ROOF_TOP_1 = (int)VehicleAreaWindow::ROOF_TOP_1;
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
constexpr int CHARGE_PORT_REAR_LEFT = (int)PortLocationType::REAR_LEFT;
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
 * This property is used for test purpose to set properties' value from vehicle.
 * For example: Mocking hard button press triggering a HVAC fan speed change.
 * Android set kSetPropertyFromVehicleForTest with an array of integer {HVAC_FAN_SPEED, value of
 * fan speed} and a long value indicates the timestamp of the events .
 * It only works with integer type properties.
 */
const int32_t kSetIntPropertyFromVehicleForTest =
        0x1112 | VehiclePropertyGroup::VENDOR | VehicleArea::GLOBAL | VehiclePropertyType::MIXED;
/**
 * This property is used for test purpose to set properties' value from vehicle.
 * It only works with float type properties.
 */
const int32_t kSetFloatPropertyFromVehicleForTest =
        0x1113 | VehiclePropertyGroup::VENDOR | VehicleArea::GLOBAL | VehiclePropertyType::MIXED;
/**
 * This property is used for test purpose to set properties' value from vehicle.
 * It only works with boolean type properties.
 */
const int32_t kSetBooleanPropertyFromVehicleForTest =
        0x1114 | VehiclePropertyGroup::VENDOR | VehicleArea::GLOBAL | VehiclePropertyType::MIXED;

/**
 * This property is used for test purpose. End to end tests use this property to test set and get
 * method for MIXED type properties.
 */
const int32_t kMixedTypePropertyForTest =
        0x1111 | VehiclePropertyGroup::VENDOR | VehicleArea::GLOBAL | VehiclePropertyType::MIXED;

#ifdef ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING
/**
 * Converts the system property to the vendor property.
 * WARNING: This is only for the end-to-end testing, Should NOT include in the
 * user build */
inline constexpr int32_t toVendor(VehicleProperty prop) {
    return (toInt(prop) & ~toInt(VehiclePropertyGroup::MASK)) | VehiclePropertyGroup::VENDOR;
}

/**
 * These properties are used for the end-to-end testing of ClusterHomeService.
 */
constexpr int32_t VENDOR_CLUSTER_SWITCH_UI = toVendor(VehicleProperty::CLUSTER_SWITCH_UI);
constexpr int32_t VENDOR_CLUSTER_DISPLAY_STATE = toVendor(VehicleProperty::CLUSTER_DISPLAY_STATE);
constexpr int32_t VENDOR_CLUSTER_REPORT_STATE = toVendor(VehicleProperty::CLUSTER_REPORT_STATE);
constexpr int32_t VENDOR_CLUSTER_REQUEST_DISPLAY =
        toVendor(VehicleProperty::CLUSTER_REQUEST_DISPLAY);
constexpr int32_t VENDOR_CLUSTER_NAVIGATION_STATE =
        toVendor(VehicleProperty::CLUSTER_NAVIGATION_STATE);
#endif  // ENABLE_VENDOR_CLUSTER_PROPERTY_FOR_TESTING

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

/**
 * These properties are placeholder properties for developers to test new features without
 * implementing a real property.
 */
constexpr int32_t PLACEHOLDER_PROPERTY_INT =
        0x2a11 | VehiclePropertyGroup::VENDOR | VehicleArea::GLOBAL | VehiclePropertyType::INT32;
constexpr int32_t PLACEHOLDER_PROPERTY_FLOAT =
        0x2a11 | VehiclePropertyGroup::VENDOR | VehicleArea::GLOBAL | VehiclePropertyType::FLOAT;
constexpr int32_t PLACEHOLDER_PROPERTY_BOOLEAN =
        0x2a11 | VehiclePropertyGroup::VENDOR | VehicleArea::GLOBAL | VehiclePropertyType::BOOLEAN;
constexpr int32_t PLACEHOLDER_PROPERTY_STRING =
        0x2a11 | VehiclePropertyGroup::VENDOR | VehicleArea::GLOBAL | VehiclePropertyType::STRING;

const int32_t kHvacPowerProperties[] = {
    toInt(VehicleProperty::HVAC_FAN_SPEED),
    toInt(VehicleProperty::HVAC_FAN_DIRECTION),
};

}  // namespace android::hardware::automotive::vehicle::V2_0::impl
