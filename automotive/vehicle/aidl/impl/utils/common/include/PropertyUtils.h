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

#ifndef android_hardware_automotive_vehicle_aidl_impl_utils_common_include_PropertyUtils_H_
#define android_hardware_automotive_vehicle_aidl_impl_utils_common_include_PropertyUtils_H_

#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace propertyutils_impl {

// These names are not part of the API since we only expose ints.
using ::aidl::android::hardware::automotive::vehicle::EvStoppingMode;
using ::aidl::android::hardware::automotive::vehicle::PortLocationType;
using ::aidl::android::hardware::automotive::vehicle::VehicleArea;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaDoor;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaSeat;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaWheel;
using ::aidl::android::hardware::automotive::vehicle::VehicleAreaWindow;
using ::aidl::android::hardware::automotive::vehicle::VehicleHvacFanDirection;
using ::aidl::android::hardware::automotive::vehicle::VehicleLightState;
using ::aidl::android::hardware::automotive::vehicle::VehicleLightSwitch;
using ::aidl::android::hardware::automotive::vehicle::VehicleProperty;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyGroup;
using ::aidl::android::hardware::automotive::vehicle::VehiclePropertyType;

}  // namespace propertyutils_impl

// Some handy constants to avoid conversions from enum to int.
constexpr int ABS_ACTIVE = toInt(propertyutils_impl::VehicleProperty::ABS_ACTIVE);
constexpr int AP_POWER_STATE_REQ = toInt(propertyutils_impl::VehicleProperty::AP_POWER_STATE_REQ);
constexpr int AP_POWER_STATE_REPORT =
        toInt(propertyutils_impl::VehicleProperty::AP_POWER_STATE_REPORT);
constexpr int DOOR_1_LEFT = toInt(propertyutils_impl::VehicleAreaDoor::ROW_1_LEFT);
constexpr int DOOR_1_RIGHT = toInt(propertyutils_impl::VehicleAreaDoor::ROW_1_RIGHT);
constexpr int DOOR_2_LEFT = toInt(propertyutils_impl::VehicleAreaDoor::ROW_2_LEFT);
constexpr int DOOR_2_RIGHT = toInt(propertyutils_impl::VehicleAreaDoor::ROW_2_RIGHT);
constexpr int DOOR_REAR = toInt(propertyutils_impl::VehicleAreaDoor::REAR);
constexpr int WINDOW_1_LEFT = toInt(propertyutils_impl::VehicleAreaWindow::ROW_1_LEFT);
constexpr int WINDOW_1_RIGHT = toInt(propertyutils_impl::VehicleAreaWindow::ROW_1_RIGHT);
constexpr int WINDOW_2_LEFT = toInt(propertyutils_impl::VehicleAreaWindow::ROW_2_LEFT);
constexpr int WINDOW_2_RIGHT = toInt(propertyutils_impl::VehicleAreaWindow::ROW_2_RIGHT);
constexpr int WINDOW_ROOF_TOP_1 = toInt(propertyutils_impl::VehicleAreaWindow::ROOF_TOP_1);
constexpr int FAN_DIRECTION_FACE = toInt(propertyutils_impl::VehicleHvacFanDirection::FACE);
constexpr int FAN_DIRECTION_FLOOR = toInt(propertyutils_impl::VehicleHvacFanDirection::FLOOR);
constexpr int FAN_DIRECTION_DEFROST = toInt(propertyutils_impl::VehicleHvacFanDirection::DEFROST);
constexpr int OBD2_LIVE_FRAME = toInt(propertyutils_impl::VehicleProperty::OBD2_LIVE_FRAME);
constexpr int OBD2_FREEZE_FRAME = toInt(propertyutils_impl::VehicleProperty::OBD2_FREEZE_FRAME);
constexpr int OBD2_FREEZE_FRAME_INFO =
        toInt(propertyutils_impl::VehicleProperty::OBD2_FREEZE_FRAME_INFO);
constexpr int OBD2_FREEZE_FRAME_CLEAR =
        toInt(propertyutils_impl::VehicleProperty::OBD2_FREEZE_FRAME_CLEAR);
constexpr int TRACTION_CONTROL_ACTIVE =
        toInt(propertyutils_impl::VehicleProperty::TRACTION_CONTROL_ACTIVE);
constexpr int VEHICLE_MAP_SERVICE = toInt(propertyutils_impl::VehicleProperty::VEHICLE_MAP_SERVICE);
constexpr int WHEEL_TICK = toInt(propertyutils_impl::VehicleProperty::WHEEL_TICK);
constexpr int SEAT_1_LEFT = toInt(propertyutils_impl::VehicleAreaSeat::ROW_1_LEFT);
constexpr int SEAT_1_RIGHT = toInt(propertyutils_impl::VehicleAreaSeat::ROW_1_RIGHT);
constexpr int SEAT_2_LEFT = toInt(propertyutils_impl::VehicleAreaSeat::ROW_2_LEFT);
constexpr int SEAT_2_RIGHT = toInt(propertyutils_impl::VehicleAreaSeat::ROW_2_RIGHT);
constexpr int SEAT_2_CENTER = toInt(propertyutils_impl::VehicleAreaSeat::ROW_2_CENTER);
constexpr int FUEL_DOOR_REAR_LEFT = toInt(propertyutils_impl::PortLocationType::REAR_LEFT);
constexpr int CHARGE_PORT_FRONT_LEFT = toInt(propertyutils_impl::PortLocationType::FRONT_LEFT);
constexpr int CHARGE_PORT_REAR_LEFT = toInt(propertyutils_impl::PortLocationType::REAR_LEFT);
constexpr int LIGHT_STATE_ON = toInt(propertyutils_impl::VehicleLightState::ON);
constexpr int LIGHT_STATE_OFF = toInt(propertyutils_impl::VehicleLightState::OFF);
constexpr int LIGHT_SWITCH_OFF = toInt(propertyutils_impl::VehicleLightSwitch::OFF);
constexpr int LIGHT_SWITCH_ON = toInt(propertyutils_impl::VehicleLightSwitch::ON);
constexpr int LIGHT_SWITCH_AUTO = toInt(propertyutils_impl::VehicleLightSwitch::AUTOMATIC);
constexpr int EV_STOPPING_MODE_CREEP = toInt(propertyutils_impl::EvStoppingMode::CREEP);
constexpr int EV_STOPPING_MODE_ROLL = toInt(propertyutils_impl::EvStoppingMode::ROLL);
constexpr int EV_STOPPING_MODE_HOLD = toInt(propertyutils_impl::EvStoppingMode::HOLD);
constexpr int WHEEL_FRONT_LEFT = toInt(propertyutils_impl::VehicleAreaWheel::LEFT_FRONT);
constexpr int WHEEL_FRONT_RIGHT = toInt(propertyutils_impl::VehicleAreaWheel::RIGHT_FRONT);
constexpr int WHEEL_REAR_LEFT = toInt(propertyutils_impl::VehicleAreaWheel::LEFT_REAR);
constexpr int WHEEL_REAR_RIGHT = toInt(propertyutils_impl::VehicleAreaWheel::RIGHT_REAR);
constexpr int ALL_WHEELS =
        WHEEL_FRONT_LEFT | WHEEL_FRONT_RIGHT | WHEEL_REAR_LEFT | WHEEL_REAR_RIGHT;
constexpr int HVAC_LEFT = SEAT_1_LEFT | SEAT_2_LEFT | SEAT_2_CENTER;
constexpr int HVAC_RIGHT = SEAT_1_RIGHT | SEAT_2_RIGHT;
constexpr int HVAC_ALL = HVAC_LEFT | HVAC_RIGHT;

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_utils_common_include_PropertyUtils_H_
