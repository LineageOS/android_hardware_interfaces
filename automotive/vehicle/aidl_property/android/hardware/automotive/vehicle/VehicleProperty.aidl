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

package android.hardware.automotive.vehicle;

import android.hardware.automotive.vehicle.VehicleArea;
import android.hardware.automotive.vehicle.VehiclePropertyGroup;
import android.hardware.automotive.vehicle.VehiclePropertyType;
/**
 * Declares all vehicle properties. VehicleProperty has a bitwise structure.
 * Each property must have:
 *  - a unique id from range 0x0100 - 0xffff
 *  - associated data type using VehiclePropertyType
 *  - property group (VehiclePropertyGroup)
 *  - vehicle area (VehicleArea)
 *
 * Vendors are allowed to extend this enum with their own properties. In this
 * case they must use VehiclePropertyGroup:VENDOR flag when the property is
 * declared.
 *
 * When a property's status field is not set to AVAILABLE:
 *  - IVehicle#set may return StatusCode::NOT_AVAILABLE.
 *  - IVehicle#get is not guaranteed to work.
 *
 * Properties set to values out of range must be ignored and no action taken
 * in response to such ill formed requests.
 */
@VintfStability
@Backing(type="int")
enum VehicleProperty {
    /**
     * Undefined property.
     *
     * This property must never be used/supported.
     */
    INVALID = 0x00000000,
    /**
     * VIN of vehicle
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    INFO_VIN = 0x0100 + 0x10000000 + 0x01000000
            + 0x00100000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:STRING
    /**
     * Manufacturer of vehicle
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    INFO_MAKE = 0x0101 + 0x10000000 + 0x01000000
            + 0x00100000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:STRING
    /**
     * Model of vehicle
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    INFO_MODEL = 0x0102 + 0x10000000 + 0x01000000
            + 0x00100000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:STRING
    /**
     * Model year of vehicle.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:YEAR
     */
    INFO_MODEL_YEAR = 0x0103 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Fuel capacity of the vehicle in milliliters
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:MILLILITER
     */
    INFO_FUEL_CAPACITY = 0x0104 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * List of fuels the vehicle may use.
     *
     * FuelType::FUEL_TYPE_ELECTRIC must only be included if the vehicle is plug in rechargeable.
     * For example:
     *   An FHEV (Fully Hybrid Electric Vehicle) must not include FuelType::FUEL_TYPE_ELECTRIC in
     *   INFO_FUEL_TYPE's INT32_VEC value. So INFO_FUEL_TYPE can be populated as such:
     *     int32Values = { FuelType::FUEL_TYPE_UNLEADED }
     *   On the other hand, a PHEV (Partially Hybrid Electric Vehicle) is plug in rechargeable, and
     *   hence should include FuelType::FUEL_TYPE_ELECTRIC in INFO_FUEL_TYPE's INT32_VEC value. So
     *   INFO_FUEL_TYPE can be populated as such:
     *     int32Values = { FuelType::FUEL_TYPE_UNLEADED, FuelType::FUEL_TYPE_ELECTRIC }
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @data_enum FuelType
     */
    INFO_FUEL_TYPE = 0x0105 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Nominal battery capacity for EV or hybrid vehicle
     *
     * Returns the nominal battery capacity, if EV or hybrid. This is the battery capacity when the
     * vehicle is new. This value might be different from EV_CURRENT_BATTERY_CAPACITY because
     * EV_CURRENT_BATTERY_CAPACITY returns the real-time battery capacity taking into account
     * factors such as battery aging and temperature dependency.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:WH
     */
    INFO_EV_BATTERY_CAPACITY = 0x0106 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * List of connectors this EV may use
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @data_enum EvConnectorType
     * @access VehiclePropertyAccess.READ
     */
    INFO_EV_CONNECTOR_TYPE = 0x0107 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Fuel door location
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @data_enum PortLocationType
     * @access VehiclePropertyAccess.READ
     */
    INFO_FUEL_DOOR_LOCATION = 0x0108 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * EV port location
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @data_enum PortLocationType
     */
    INFO_EV_PORT_LOCATION = 0x0109 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Driver's seat location
     * VHAL implementations must ignore the areaId. Use VehicleArea:GLOBAL.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @data_enum VehicleAreaSeat
     * @access VehiclePropertyAccess.READ
     */
    INFO_DRIVER_SEAT = 0x010A + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Exterior dimensions of vehicle.
     *
     *  int32Values[0] = height
     *  int32Values[1] = length
     *  int32Values[2] = width
     *  int32Values[3] = width including mirrors
     *  int32Values[4] = wheel base
     *  int32Values[5] = track width front
     *  int32Values[6] = track width rear
     *  int32Values[7] = curb to curb turning diameter
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:MILLIMETER
     */
    INFO_EXTERIOR_DIMENSIONS = 0x010B + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Multiple EV port locations
     *
     * Implement this property if the vehicle has multiple EV ports.
     * Port locations are defined in PortLocationType.
     * For example, a car has one port in front left and one port in rear left:
     *   int32Values[0] = PortLocationType::FRONT_LEFT
     *   int32Values[0] = PortLocationType::REAR_LEFT
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @data_enum PortLocationType
     */
    INFO_MULTI_EV_PORT_LOCATIONS = 0x010C + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Current odometer value of the vehicle
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:KILOMETER
     */
    PERF_ODOMETER = 0x0204 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Speed of the vehicle
     *
     * The value must be positive when the vehicle is moving forward and negative when
     * the vehicle is moving backward. This value is independent of gear value
     * (CURRENT_GEAR or GEAR_SELECTION), for example, if GEAR_SELECTION is GEAR_NEUTRAL,
     * PERF_VEHICLE_SPEED is positive when the vehicle is moving forward, negative when moving
     * backward, and zero when not moving.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:METER_PER_SEC
     */
    PERF_VEHICLE_SPEED = 0x0207 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Speed of the vehicle for displays
     *
     * Some cars display a slightly slower speed than the actual speed.  This is
     * usually displayed on the speedometer.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:METER_PER_SEC
     */
    PERF_VEHICLE_SPEED_DISPLAY = 0x0208 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Front bicycle model steering angle for vehicle
     *
     * Angle is in degrees.  Left is negative.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:DEGREES
     */
    PERF_STEERING_ANGLE = 0x0209 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Rear bicycle model steering angle for vehicle
     *
     * Angle is in degrees.  Left is negative.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:DEGREES
     */
    PERF_REAR_STEERING_ANGLE = 0x0210 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Temperature of engine coolant
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:CELSIUS
     */
    ENGINE_COOLANT_TEMP = 0x0301 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Engine oil level
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleOilLevel
     */
    ENGINE_OIL_LEVEL = 0x0303 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Temperature of engine oil
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:CELSIUS
     */
    ENGINE_OIL_TEMP = 0x0304 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Engine rpm
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:RPM
     */
    ENGINE_RPM = 0x0305 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Reports wheel ticks
     *
     * The first element in the vector is a reset count.  A reset indicates
     * previous tick counts are not comparable with this and future ones.  Some
     * sort of discontinuity in tick counting has occurred.
     *
     * The next four elements represent ticks for individual wheels in the
     * following order: front left, front right, rear right, rear left.  All
     * tick counts are cumulative.  Tick counts increment when the vehicle
     * moves forward, and decrement when vehicles moves in reverse.  The ticks
     * should be reset to 0 when the vehicle is started by the user.
     *
     *  int64Values[0] = reset count
     *  int64Values[1] = front left ticks
     *  int64Values[2] = front right ticks
     *  int64Values[3] = rear right ticks
     *  int64Values[4] = rear left ticks
     *
     * configArray is used to indicate the micrometers-per-wheel-tick value and
     * which wheels are supported.  configArray is set as follows:
     *
     *  configArray[0], bits [0:3] = supported wheels. Uses enum Wheel. For example, if all wheels
     *    are supported, then configArray[0] = VehicleAreaWheel::LEFT_FRONT
     *    | VehicleAreaWheel::RIGHT_FRONT | VehicleAreaWheel::LEFT_REAR
     *    | VehicleAreaWheel::RIGHT_REAR
     *  configArray[1] = micrometers per front left wheel tick
     *  configArray[2] = micrometers per front right wheel tick
     *  configArray[3] = micrometers per rear right wheel tick
     *  configArray[4] = micrometers per rear left wheel tick
     *
     * NOTE:  If a wheel is not supported, its value shall always be set to 0.
     *
     * VehiclePropValue.timestamp must be correctly filled in.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     */
    WHEEL_TICK = 0x0306 + 0x10000000 + 0x01000000
            + 0x00510000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT64_VEC
    /**
     * Fuel remaining in the vehicle, in milliliters
     *
     * Value may not exceed INFO_FUEL_CAPACITY
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:MILLILITER
     */
    FUEL_LEVEL = 0x0307 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Fuel door open
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    FUEL_DOOR_OPEN = 0x0308 + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * Battery level for EV or hybrid vehicle
     *
     * Returns the current battery level, if EV or hybrid. This value will not exceed
     * EV_CURRENT_BATTERY_CAPACITY. To calculate the battery percentage, use:
     * (EV_BATTERY_LEVEL/EV_CURRENT_BATTERY_CAPACITY)*100.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:WH
     */
    EV_BATTERY_LEVEL = 0x0309 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Current battery capacity for EV or hybrid vehicle
     *
     * Returns the actual value of battery capacity, if EV or hybrid. This property captures the
     * real-time battery capacity taking into account factors such as battery aging and temperature
     * dependency. Therefore, this value might be different from INFO_EV_BATTERY_CAPACITY because
     * INFO_EV_BATTERY_CAPACITY returns the nominal battery capacity from when the vehicle was new.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:WH
     */
    EV_CURRENT_BATTERY_CAPACITY =
            0x030D + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.FLOAT,
    /**
     * EV charge port open
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    EV_CHARGE_PORT_OPEN = 0x030A + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * EV charge port connected
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    EV_CHARGE_PORT_CONNECTED = 0x030B + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * EV instantaneous charge rate in milliwatts
     *
     * Positive value indicates battery is being charged.
     * Negative value indicates battery being discharged.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:MW
     */
    EV_BATTERY_INSTANTANEOUS_CHARGE_RATE = 0x030C + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Range remaining
     *
     * Meters remaining of fuel and charge.  Range remaining shall account for
     * all energy sources in a vehicle.  For example, a hybrid car's range will
     * be the sum of the ranges based on fuel and battery.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE because a navigation app could
     * update the range if it has a more accurate estimate based on the upcoming route. However,
     * this property can be implemented as VehiclePropertyAccess.READ only at the OEM's discretion.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:METER
     */
    RANGE_REMAINING = 0x0308 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * EV battery average temperature
     *
     * Exposes the temperature of the battery in an EV. If multiple batteries exist in the EV, or
     * multiple temperature sensors exist, this property should be set to the mean or a meaningful
     * weighted average that best represents the overall temperature of the battery system.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:CELSIUS
     */
    EV_BATTERY_AVERAGE_TEMPERATURE =
            0x030E + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.FLOAT,
    /**
     * Tire pressure
     *
     * Each tires is identified by its areaConfig.areaId config and their
     * minFloatValue/maxFloatValue are used to store OEM recommended pressure
     * range. The minFloatValue and maxFloatValue in VehicleAreaConfig must be defined.
     * The minFloatValue in the areaConfig data represents the lower bound of
     * the recommended tire pressure.
     * The maxFloatValue in the areaConfig data represents the upper bound of
     * the recommended tire pressure.
     * For example:
     * The following areaConfig indicates the recommended tire pressure
     * of left_front tire is from 200.0 KILOPASCAL to 240.0 KILOPASCAL.
     * .areaConfigs = {
     *      VehicleAreaConfig {
     *          .areaId = VehicleAreaWheel::LEFT_FRONT,
     *          .minFloatValue = 200.0,
     *          .maxFloatValue = 240.0,
     *      }
     * },
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:KILOPASCAL
     */
    TIRE_PRESSURE = 0x0309 + 0x10000000 + 0x07000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:WHEEL,VehiclePropertyType:FLOAT
    /**
     * Critically low tire pressure
     *
     * This property indicates the critically low pressure threshold for each tire.
     * It indicates when it is time for tires to be replaced or fixed. The value
     * must be less than or equal to minFloatValue in TIRE_PRESSURE.
     * Minimum and maximum property values (that is, minFloatValue, maxFloatValue)
     * are not applicable to this property.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:KILOPASCAL
     */
    CRITICALLY_LOW_TIRE_PRESSURE = 0x030A + 0x10000000 + 0x07000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:WHEEL,VehiclePropertyType:FLOAT
    /**
     * Represents feature for engine idle automatic stop.
     *
     * If true, the vehicle may automatically shut off the engine when it is not needed and then
     * automatically restart it when needed.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    ENGINE_IDLE_AUTO_STOP_ENABLED =
            0x0320 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,
    /**
     * Impact detected.
     *
     * Bit flag property to relay information on whether an impact has occurred on a particular side
     * of the vehicle as described through the ImpactSensorLocation enum. As a bit flag property,
     * this property can be set to multiple ORed together values of the enum when necessary.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all bit flags of ImpactSensorLocation are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum ImpactSensorLocation
     */
    IMPACT_DETECTED =
            0x0330 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,
    /**
     * Currently selected gear
     *
     * This is the gear selected by the user.
     *
     * Values in the config data must represent the list of supported gears for this vehicle. For
     * example, config data for an automatic transmission must contain {GEAR_NEUTRAL, GEAR_REVERSE,
     * GEAR_PARK, GEAR_DRIVE, GEAR_1, GEAR_2,...} and for manual transmission the list must be
     * {GEAR_NEUTRAL, GEAR_REVERSE, GEAR_1, GEAR_2,...}
     *
     * In the case of an automatic transmission vehicle that allows the driver to select specific
     * gears on demand (i.e. "manual mode"), GEAR_SELECTION's value must be set to the specific gear
     * selected by the driver instead of simply GEAR_DRIVE.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleGear
     */
    GEAR_SELECTION = 0x0400 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Current gear. In non-manual case, selected gear may not
     * match the current gear. For example, if the selected gear is GEAR_DRIVE,
     * the current gear will be one of GEAR_1, GEAR_2 etc, which reflects
     * the actual gear the transmission is currently running in.
     *
     * Values in the config data must represent the list of supported gears
     * for this vehicle.  For example, config data for an automatic transmission
     * must contain {GEAR_NEUTRAL, GEAR_REVERSE, GEAR_PARK, GEAR_1, GEAR_2,...}
     * and for manual transmission the list must be
     * {GEAR_NEUTRAL, GEAR_REVERSE, GEAR_1, GEAR_2,...}. This list need not be the
     * same as that of the supported gears reported in GEAR_SELECTION.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleGear
     */
    CURRENT_GEAR = 0x0401 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Parking brake state.
     *
     * This property is true indicates that the car's parking brake is currently engaged. False
     * implies that the car's parking brake is currently disengaged.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    PARKING_BRAKE_ON = 0x0402 + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * Auto-apply parking brake.
     *
     * This property is true indicates that the car's automatic parking brake feature is currently
     * enabled. False indicates that the car's automatic parking brake feature is currently
     * disabled.
     *
     * This property is often confused with PARKING_BRAKE_ON. The difference is that
     * PARKING_BRAKE_ON describes whether the actual parking brake is currently on/off, whereas
     * PARKING_BRAKE_AUTO_APPLY describes whether the feature of automatic parking brake is enabled/
     * disabled, and does not describe the current state of the actual parking brake.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    PARKING_BRAKE_AUTO_APPLY = 0x0403 + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * Regenerative braking level of a electronic vehicle
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined. All values between
     * minInt32Value and maxInt32Value must be supported. The minInt32Value must be 0.
     *
     * The maxInt32Value indicates the maximum amount of energy regenerated from braking. The
     * minInt32Value indicates no regenerative braking.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    EV_BRAKE_REGENERATION_LEVEL =
            0x040C + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,
    /**
     * Warning for fuel low level.
     *
     * This property corresponds to the low fuel warning on the dashboard.
     * Once FUEL_LEVEL_LOW is set, it should not be cleared until more fuel is
     * added to the vehicle.  This property may take into account all fuel
     * sources for a vehicle - for example:
     *
     *   For a gas powered vehicle, this property is based soley on gas level.
     *   For a battery powered vehicle, this property is based solely on battery level.
     *   For a hybrid vehicle, this property may be based on the combination of gas and battery
     *      levels, at the OEM's discretion.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    FUEL_LEVEL_LOW = 0x0405 + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * Night mode
     *
     * True indicates that the night mode sensor has detected that the car cabin environment has
     * low light. The platform could use this, for example, to enable appropriate UI for
     * better viewing in dark or low light environments.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    NIGHT_MODE = 0x0407 + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * State of the vehicles turn signals
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleTurnSignal
     */
    TURN_SIGNAL_STATE = 0x0408 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Represents ignition state
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleIgnitionState
     */
    IGNITION_STATE = 0x0409 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * ABS is active
     *
     * Set to true when ABS is active.  Reset to false when ABS is off.  This
     * property may be intermittently set (pulsing) based on the real-time
     * state of the ABS system.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    ABS_ACTIVE = 0x040A + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * Traction Control is active
     *
     * Set to true when traction control (TC) is active.  Reset to false when
     * TC is off.  This property may be intermittently set (pulsing) based on
     * the real-time state of the TC system.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    TRACTION_CONTROL_ACTIVE = 0x040B + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * Represents property for the current stopping mode of the vehicle.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues must be defined unless
     * all enum values of EvStoppingMode are supported.
     *
     * The EvStoppingMode enum may be extended to include more states in the future.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum EvStoppingMode
     */
    EV_STOPPING_MODE =
            0x040D + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,
    /**
     * Enable or disable Electronic Stability Control (ESC).
     *
     * Set true to enable ESC and false to disable ESC. When ESC is enabled, a system in the vehicle
     * should be controlling the tires during instances with high risk of skidding to actively
     * prevent the same from happening.
     *
     * In general, ELECTRONIC_STABILITY_CONTROL_ENABLED should always return true or false. If the
     * feature is not available due to some temporary state, such as the vehicle speed being too
     * high, that information must be conveyed through the ErrorState values in the
     * ELECTRONIC_STABILITY_CONTROL_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    ELECTRONIC_STABILITY_CONTROL_ENABLED =
            0x040E + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,
    /**
     * Electronic Stability Control (ESC) state.
     *
     * Returns the current state of ESC. This property must always return a valid state defined in
     * ElectronicStabilityControlState or ErrorState. It must not surface errors through StatusCode
     * and must use the supported error states instead.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both ElectronicStabilityControlState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum ElectronicStabilityControlState
     * @data_enum ErrorState
     */
    ELECTRONIC_STABILITY_CONTROL_STATE =
            0x040F + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,
    /**
     * HVAC Properties
     *
     * Additional rules for mapping non-GLOBAL VehicleArea type HVAC properties
     * to AreaIDs:
     *  - Every “area” for a specific VehicleArea type that is affected by the
     *    property, must be included in an area ID for that property.
     *
     * Example 1: A car has two front seats (ROW_1_LEFT, ROW_1_RIGHT) and three
     *  back seats (ROW_2_LEFT, ROW_2_CENTER, ROW_2_RIGHT). There are two
     *  temperature control units -- driver side and passenger side.
     *   - A valid mapping set of AreaIDs for HVAC_TEMPERATURE_SET would be a
     *     two element array:
     *      - ROW_1_LEFT  | ROW_2_LEFT
     *      - ROW_1_RIGHT | ROW_2_CENTER | ROW_2_RIGHT
     *   - An alternative mapping for the same hardware configuration would be:
     *      - ROW_1_LEFT  | ROW_2_CENTER | ROW_2_LEFT
     *      - ROW_1_RIGHT | ROW_2_RIGHT
     *  The temperature controllers are assigned to the seats which they
     *  "most influence", but every seat must be included exactly once. The
     *  assignment of the center rear seat to the left or right AreaID may seem
     *  arbitrary, but the inclusion of every seat in exactly one AreaID ensures
     *  that the seats in the car are all expressed and that a "reasonable" way
     *  to affect each seat is available.
     *
     * Example 2: A car has three seat rows with two seats in the front row (ROW_1_LEFT,
     *  ROW_1_RIGHT) and three seats in the second (ROW_2_LEFT, ROW_2_CENTER,
     *  ROW_2_RIGHT) and third rows (ROW_3_LEFT, ROW_3_CENTER, ROW_3_RIGHT). There
     *  are three temperature control units -- driver side, passenger side, and rear.
     *   - A reasonable way to map HVAC_TEMPERATURE_SET to AreaIDs is a three
     *     element array:
     *     - ROW_1_LEFT
     *     - ROW_1_RIGHT
     *     - ROW_2_LEFT | ROW_2_CENTER | ROW_2_RIGHT | ROW_3_LEFT | ROW_3_CENTER | ROW_3_RIGHT
     *
     * Example 3: A car has two front seats (ROW_1_LEFT, ROW_1_RIGHT) and three
     *  back seats (ROW_2_LEFT, ROW_2_CENTER, ROW_2_RIGHT). Suppose the car
     *  supports HVAC_AUTO_ON for just the two front seats.
     *   - A valid mapping set of AreaIDs for HVAC_AUTO_ON would be:
     *      - ROW_1_LEFT | ROW_1_RIGHT
     *   - If HVAC_AUTO_ON had two separate control units for the driver side
     *     and passenger side, an alternative mapping would be:
     *      - ROW_1_LEFT
     *      - ROW_1_RIGHT
     */

    /**
     * Fan speed setting
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the lowest fan speed.
     * The maxInt32Value indicates the highest fan speed.
     *
     * This property is not in any particular unit but in a specified range of relative speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_FAN_SPEED = 0x0500 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Fan direction setting
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleHvacFanDirection
     */
    HVAC_FAN_DIRECTION = 0x0501 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * HVAC current temperature.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:CELSIUS
     */
    HVAC_TEMPERATURE_CURRENT = 0x0502 + 0x10000000 + 0x05000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:FLOAT
    /**
     * HVAC, target temperature set.
     *
     * The configArray is used to indicate the valid values for HVAC in Fahrenheit and Celsius.
     * Android might use it in the HVAC app UI.
     * The configArray is set as follows:
     *      configArray[0] = [the lower bound of the supported temperature in Celsius] * 10.
     *      configArray[1] = [the upper bound of the supported temperature in Celsius] * 10.
     *      configArray[2] = [the increment in Celsius] * 10.
     *      configArray[3] = [the lower bound of the supported temperature in Fahrenheit] * 10.
     *      configArray[4] = [the upper bound of the supported temperature in Fahrenheit] * 10.
     *      configArray[5] = [the increment in Fahrenheit] * 10.
     * For example, if the vehicle supports temperature values as:
     *      [16.0, 16.5, 17.0 ,..., 28.0] in Celsius
     *      [60.5, 61.5, 62.5 ,..., 85.5] in Fahrenheit.
     * The configArray should be configArray = {160, 280, 5, 605, 825, 10}.
     *
     * If the vehicle supports HVAC_TEMPERATURE_VALUE_SUGGESTION, the application can use
     * that property to get the suggested value before setting HVAC_TEMPERATURE_SET. Otherwise,
     * the application may choose the value in HVAC_TEMPERATURE_SET configArray by itself.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:CELSIUS
     */
    HVAC_TEMPERATURE_SET = 0x0503 + 0x10000000 + 0x05000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:FLOAT
    /**
     * Fan-based defrost for designated window.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_DEFROSTER = 0x0504 + 0x10000000 + 0x03000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:WINDOW,VehiclePropertyType:BOOLEAN
    /**
     * On/off AC for designated areaId
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @config_flags Supported areaIds
     */
    HVAC_AC_ON = 0x0505 + 0x10000000 + 0x05000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:BOOLEAN
    /**
     * On/off max AC
     *
     * When MAX AC is on, the ECU may adjust the vent position, fan speed,
     * temperature, etc as necessary to cool the vehicle as quickly as possible.
     * Any parameters modified as a side effect of turning on/off the MAX AC
     * parameter shall generate onPropertyEvent() callbacks to the VHAL.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_MAX_AC_ON = 0x0506 + 0x10000000 + 0x05000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:BOOLEAN
    /**
     * On/off max defrost
     *
     * When MAX DEFROST is on, the ECU may adjust the vent position, fan speed,
     * temperature, etc as necessary to defrost the windows as quickly as
     * possible.  Any parameters modified as a side effect of turning on/off
     * the MAX DEFROST parameter shall generate onPropertyEvent() callbacks to
     * the VHAL.
     * The AreaIDs for HVAC_MAX_DEFROST_ON indicate MAX DEFROST can be controlled
     * in the area.
     * For example:
     * areaConfig.areaId = {ROW_1_LEFT | ROW_1_RIGHT} indicates HVAC_MAX_DEFROST_ON
     * only can be controlled for the front rows.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_MAX_DEFROST_ON = 0x0507 + 0x10000000 + 0x05000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:BOOLEAN
    /**
     * Recirculation on/off
     *
     * Controls the supply of exterior air to the cabin.  Recirc “on” means the
     * majority of the airflow into the cabin is originating in the cabin.
     * Recirc “off” means the majority of the airflow into the cabin is coming
     * from outside the car.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_RECIRC_ON = 0x0508 + 0x10000000 + 0x05000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:BOOLEAN
    /**
     * Enable temperature coupling between areas.
     *
     * The AreaIDs for HVAC_DUAL_ON property shall contain a combination of
     * HVAC_TEMPERATURE_SET AreaIDs that can be coupled together. If
     * HVAC_TEMPERATURE_SET is mapped to AreaIDs [a_1, a_2, ..., a_n], and if
     * HVAC_DUAL_ON can be enabled to couple a_i and a_j, then HVAC_DUAL_ON
     * property must be mapped to [a_i | a_j]. Further, if a_k and a_l can also
     * be coupled together separately then HVAC_DUAL_ON must be mapped to
     * [a_i | a_j, a_k | a_l].
     *
     * Example: A car has two front seats (ROW_1_LEFT, ROW_1_RIGHT) and three
     *  back seats (ROW_2_LEFT, ROW_2_CENTER, ROW_2_RIGHT). There are two
     *  temperature control units -- driver side and passenger side -- which can
     *  be optionally synchronized. This may be expressed in the AreaIDs this way:
     *  - HVAC_TEMPERATURE_SET->[ROW_1_LEFT | ROW_2_LEFT, ROW_1_RIGHT | ROW_2_CENTER | ROW_2_RIGHT]
     *  - HVAC_DUAL_ON->[ROW_1_LEFT | ROW_2_LEFT | ROW_1_RIGHT | ROW_2_CENTER | ROW_2_RIGHT]
     *
     * When the property is enabled, the ECU must synchronize the temperature
     * for the affected areas. Any parameters modified as a side effect
     * of turning on/off the DUAL_ON parameter shall generate
     * onPropertyEvent() callbacks to the VHAL. In addition, if setting
     * a temperature (i.e. driver's temperature) changes another temperature
     * (i.e. front passenger's temperature), then the appropriate
     * onPropertyEvent() callbacks must be generated.  If a user changes a
     * temperature that breaks the coupling (e.g. setting the passenger
     * temperature independently) then the VHAL must send the appropriate
     * onPropertyEvent() callbacks (i.e. HVAC_DUAL_ON = false,
     * HVAC_TEMPERATURE_SET[AreaID] = xxx, etc).
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_DUAL_ON = 0x0509 + 0x10000000 + 0x05000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:BOOLEAN
    /**
     * On/off automatic climate control.
     *
     * If true, automatic climate control is on. If false, automatic climate control is off.
     *
     * If the vehicle does not support directly turning off automatic climate control, then OEMs
     * should add logic in their VHAL implementation so that setting HVAC_AUTO_ON to false would
     * change the necessary HVAC settings to indirectly turn off HVAC_AUTO_ON. Ideally, this should
     * not be disruptive to the user, so OEMs should change back to the previous state any settings
     * that were modified once automatic climate control is off. That way the only outcome should be
     * that HVAC_AUTO_ON is off. If restoring the HVAC settings to its previous settings is not
     * possible, then the OEM should choose the least disruptive change and implement that.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_AUTO_ON = 0x050A + 0x10000000 + 0x05000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:BOOLEAN
    /**
     * Seat heating/cooling
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value indicates the maximum seat temperature heating setting.
     * The minInt32Value must be 0, unless the vehicle supports seat cooling as well. In this case,
     * minInt32Value indicates the maximum seat temperature cooling setting.
     *
     * This property is not in any particular unit, but in a specified range of relative temperature
     * settings.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_SEAT_TEMPERATURE = 0x050B + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Side Mirror Heat
     *
     * Increasing values denote higher heating levels for side mirrors.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value in the config data represents the maximum heating level.
     * The minInt32Value in the config data MUST be zero and indicates no heating.
     *
     * This property is not in any particular unit but in a specified range of relative heating
     * settings.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_SIDE_MIRROR_HEAT = 0x050C + 0x10000000 + 0x04000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:MIRROR,VehiclePropertyType:INT32
    /**
     * Steering Wheel Heating/Cooling
     *
     * Sets the amount of heating/cooling for the steering wheel.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value indicates the maximum steering wheel heating setting.
     * The minInt32Value should be 0, unless the vehicle supports steering wheel cooling as well. In
     * such a case, the minInt32Value indicates the maximum steering wheel cooling setting.
     *
     * This property is not in any particular unit but in a specified range of heating settings.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_STEERING_WHEEL_HEAT = 0x050D + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Temperature units for display
     *
     * Indicates whether the vehicle is displaying temperature to the user as
     * Celsius or Fahrenheit.
     * VehiclePropConfig.configArray is used to indicate the supported temperature display units.
     * For example: configArray[0] = CELSIUS
     *              configArray[1] = FAHRENHEIT
     *
     * This parameter MAY be used for displaying any HVAC temperature in the system.
     * Values must be one of VehicleUnit::CELSIUS or VehicleUnit::FAHRENHEIT
     * Note that internally, all temperatures are represented in floating point Celsius.
     *
     * If updating HVAC_TEMPERATURE_DISPLAY_UNITS affects the values of other *_DISPLAY_UNITS
     * properties, then their values must be updated and communicated to the AAOS framework as well.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleUnit
     */
    HVAC_TEMPERATURE_DISPLAY_UNITS = 0x050E + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Actual fan speed
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_ACTUAL_FAN_SPEED_RPM = 0x050F + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Represents global power state for HVAC. Setting this property to false
     * MAY mark some properties that control individual HVAC features/subsystems
     * to UNAVAILABLE state. Setting this property to true MAY mark some
     * properties that control individual HVAC features/subsystems to AVAILABLE
     * state (unless any/all of them are UNAVAILABLE on their own individual
     * merits).
     *
     * [Definition] HvacPower_DependentProperties: Properties that need HVAC to be
     *   powered on in order to enable their functionality. For example, in some cars,
     *   in order to turn on the AC, HVAC must be powered on first.
     *
     * HvacPower_DependentProperties list must be set in the
     * VehiclePropConfig.configArray. HvacPower_DependentProperties must only contain
     * properties that are associated with VehicleArea:SEAT. Properties that are not
     * associated with VehicleArea:SEAT, for example, HVAC_DEFROSTER, must never
     * depend on HVAC_POWER_ON property and must never be part of
     * HvacPower_DependentProperties list.
     *
     * AreaID mapping for HVAC_POWER_ON property must contain all AreaIDs that
     * HvacPower_DependentProperties are mapped to.
     *
     * Example 1: A car has two front seats (ROW_1_LEFT, ROW_1_RIGHT) and three back
     *  seats (ROW_2_LEFT, ROW_2_CENTER, ROW_2_RIGHT). If the HVAC features (AC,
     *  Temperature etc.) throughout the car are dependent on a single HVAC power
     *  controller then HVAC_POWER_ON must be mapped to
     *  [ROW_1_LEFT | ROW_1_RIGHT | ROW_2_LEFT | ROW_2_CENTER | ROW_2_RIGHT].
     *
     * Example 2: A car has two seats in the front row (ROW_1_LEFT, ROW_1_RIGHT) and
     *   three seats in the second (ROW_2_LEFT, ROW_2_CENTER, ROW_2_RIGHT) and third
     *   rows (ROW_3_LEFT, ROW_3_CENTER, ROW_3_RIGHT). If the car has temperature
     *   controllers in the front row which can operate entirely independently of
     *   temperature controllers in the back of the vehicle, then HVAC_POWER_ON
     *   must be mapped to a two element array:
     *   - ROW_1_LEFT | ROW_1_RIGHT
     *   - ROW_2_LEFT | ROW_2_CENTER | ROW_2_RIGHT | ROW_3_LEFT | ROW_3_CENTER | ROW_3_RIGHT
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_POWER_ON = 0x0510 + 0x10000000 + 0x05000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:BOOLEAN
    /**
     * Fan Positions Available
     *
     * This is a bit mask of fan positions available for the zone.  Each
     * available fan direction is denoted by a separate entry in the vector.  A
     * fan direction may have multiple bits from vehicle_hvac_fan_direction set.
     * For instance, a typical car may have the following fan positions:
     *   - FAN_DIRECTION_FACE (0x1)
     *   - FAN_DIRECTION_FLOOR (0x2)
     *   - FAN_DIRECTION_FACE | FAN_DIRECTION_FLOOR (0x3)
     *   - FAN_DIRECTION_DEFROST (0x4)
     *   - FAN_DIRECTION_FLOOR | FAN_DIRECTION_DEFROST (0x6)
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleHvacFanDirection
     */
    HVAC_FAN_DIRECTION_AVAILABLE = 0x0511 + 0x10000000 + 0x05000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32_VEC
    /**
     * Automatic recirculation on/off
     *
     * When automatic recirculation is ON, the HVAC system may automatically
     * switch to recirculation mode if the vehicle detects poor incoming air
     * quality.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_AUTO_RECIRC_ON = 0x0512 + 0x10000000 + 0x05000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:BOOLEAN
    /**
     * Seat ventilation
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value must be 0.
     * The maxInt32Value indicates the maximum ventilation setting available for the seat.
     *
     * This property is not in any particular unit but in the specified range of ventilation
     * settings.
     *
     * Used by HVAC apps and Assistant to enable, change, or read state of seat
     * ventilation.  This is different than seating cooling. It can be on at the
     * same time as cooling, or not.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_SEAT_VENTILATION = 0x0513 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Electric defrosters' status
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HVAC_ELECTRIC_DEFROSTER_ON = 0x0514 + 0x10000000 + 0x03000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:WINDOW,VehiclePropertyType:BOOLEAN
    /**
     * Suggested values for setting HVAC temperature.
     *
     * Implement the property to help applications understand the closest supported temperature
     * value in Celsius or Fahrenheit.
     *
     *      floatValues[0] = the requested value that an application wants to set a temperature to.
     *      floatValues[1] = the unit for floatValues[0]. It should be one of
     *                       {VehicleUnit:CELSIUS, VehicleUnit:FAHRENHEIT}.
     *      floatValues[2] = the value OEMs suggested in CELSIUS. This value is not included
     *                       in the request.
     *      floatValues[3] = the value OEMs suggested in FAHRENHEIT. This value is not included
     *                       in the request.
     *
     * An application calls set(VehiclePropValue propValue) with the requested value and unit for
     * the value. OEMs need to return the suggested values in floatValues[2] and floatValues[3] by
     * onPropertyEvent() callbacks.
     *
     * For example, when a user uses the voice assistant to set HVAC temperature to 66.2 in
     * Fahrenheit.
     * First, an application will set this property with the value
     * [66.2, (float)VehicleUnit:FAHRENHEIT,0,0].
     * If OEMs suggest to set 19.0 in Celsius or 66.5 in Fahrenheit for user's request, then VHAL
     * must generate a callback with property value
     * [66.2, (float)VehicleUnit:FAHRENHEIT, 19.0, 66.5]. After the voice assistant gets the
     * callback, it will inform the user and set HVAC temperature to the suggested value.
     *
     * Another example, an application receives 21 Celsius as the current temperature value by
     * querying HVC_TEMPERATURE_SET. But the application wants to know what value is displayed on
     * the car's UI in Fahrenheit.
     * For this, the application sets the property to [21, (float)VehicleUnit:CELSIUS, 0, 0]. If
     * the suggested value by the OEM for 21 Celsius is 70 Fahrenheit, then VHAL must generate a
     * callback with property value [21, (float)VehicleUnit:CELSIUS, 21.0, 70.0].
     * In this case, the application can know that the value is 70.0 Fahrenheit in the car’s UI.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    HVAC_TEMPERATURE_VALUE_SUGGESTION = 0x0515 + 0x10000000 + 0x01000000
            + 0x00610000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT_VEC
    /**
     * Distance units for display
     *
     * Indicates which units the car is using to display distances to the user. Eg. Mile, Meter
     * Kilometer.
     *
     * Distance units are defined in VehicleUnit.
     * VehiclePropConfig.configArray is used to indicate the supported distance display units.
     * For example: configArray[0] = METER
     *              configArray[1] = KILOMETER
     *              configArray[2] = MILE
     *
     * If updating DISTANCE_DISPLAY_UNITS affects the values of other *_DISPLAY_UNITS properties,
     * then their values must be updated and communicated to the AAOS framework as well.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleUnit
     */
    DISTANCE_DISPLAY_UNITS = 0x0600 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Fuel volume units for display
     *
     * Indicates which units the car is using to display fuel volume to the user. Eg. Liter or
     * Gallon.
     *
     * VehiclePropConfig.configArray is used to indicate the supported fuel volume display units.
     * Volume units are defined in VehicleUnit.
     * For example: configArray[0] = LITER
     *              configArray[1] = GALLON
     *
     * If updating FUEL_VOLUME_DISPLAY_UNITS affects the values of other *_DISPLAY_UNITS properties,
     * then their values must be updated and communicated to the AAOS framework as well.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleUnit
     */
    FUEL_VOLUME_DISPLAY_UNITS = 0x0601 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Tire pressure units for display
     *
     * Indicates which units the car is using to display tire pressure to the user. Eg. PSI, Bar or
     * Kilopascal.
     *
     * VehiclePropConfig.configArray is used to indicate the supported pressure display units.
     * Pressure units are defined in VehicleUnit.
     * For example: configArray[0] = KILOPASCAL
     *              configArray[1] = PSI
     *              configArray[2] = BAR
     *
     * If updating TIRE_PRESSURE_DISPLAY_UNITS affects the values of other *_DISPLAY_UNITS
     * properties, then their values must be updated and communicated to the AAOS framework as well.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleUnit
     */
    TIRE_PRESSURE_DISPLAY_UNITS = 0x0602 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * EV battery units for display
     *
     * Indicates which units the car is using to display EV battery information to the user. Eg.
     * watt-hours(Wh), kilowatt-hours(kWh) or ampere-hours(Ah).
     *
     * VehiclePropConfig.configArray is used to indicate the supported electrical energy units.
     * Electrical energy units are defined in VehicleUnit.
     * For example: configArray[0] = WATT_HOUR
     *              configArray[1] = AMPERE_HOURS
     *              configArray[2] = KILOWATT_HOUR
     *
     * If updating EV_BATTERY_DISPLAY_UNITS affects the values of other *_DISPLAY_UNITS properties,
     * then their values must be updated and communicated to the AAOS framework as well.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleUnit
     */
    EV_BATTERY_DISPLAY_UNITS = 0x0603 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Fuel consumption units for display
     *
     * Indicates type of units the car is using to display fuel consumption information to user
     * True indicates units are distance over volume such as MPG.
     * False indicates units are volume over distance such as L/100KM.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    FUEL_CONSUMPTION_UNITS_DISTANCE_OVER_VOLUME = 0x0604 + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * Speed units for display
     *
     * Indicates type of units the car is using to display speed to user. Eg. m/s, km/h, or mph.
     *
     * VehiclePropConfig.configArray is used to indicate the supported speed display units.
     * Pressure units are defined in VehicleUnit.
     * For example: configArray[0] = METER_PER_SEC
     *              configArray[1] = MILES_PER_HOUR
     *              configArray[2] = KILOMETERS_PER_HOUR
     *
     * If updating VEHICLE_SPEED_DISPLAY_UNITS affects the values of other *_DISPLAY_UNITS
     * properties, then their values must be updated and communicated to the AAOS framework as well.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    VEHICLE_SPEED_DISPLAY_UNITS = 0x0605 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Current date and time suggestion for the Car, encoded as Epoch time
     * (in milliseconds). This value denotes the number of milliseconds seconds
     * that have elapsed since 1/1/1970 UTC.
     *
     * This property signals a change in CarTime to Android. If the property is supported, VHAL
     * must report the most accurate current CarTime when this property is read, and publish a
     * change to this property when the CarTime value has changed. An on-change event for this
     * property must be published when CarTime changes for any reason other than the natural elapse
     * of time (time delta smaller than 500ms should not trigger an on change event). Android will
     * read and subscribe to this property to fetch time from VHAL. This can be useful to
     * synchronize Android's time with other vehicle systems (dash clock etc).
     *     int64Values[0] = provided Epoch time (in milliseconds)
     *
     * Whenever a new Value for the property is received, AAOS will create
     * and send an "ExternalTimeSuggestion" to the "TimeDetectorService".
     * If other sources do not have a higher priority, Android will use this
     * to set the system time. For information on how to adjust time source
     * priorities and how time suggestions are handled (including how Android
     * handles gitter, drift, and minimum resolution) see Time Detector Service
     * documentation.
     *
     * Note that the property may take >0 ms to get propagated through the stack
     * and, having a timestamped property helps reduce any time drift. So,
     * for all reads to the property, the timestamp can be used to negate this
     * drift:
     *     drift = elapsedTime - PropValue.timestamp
     *     effectiveTime = PropValue.value.int64Values[0] + drift
     *
     * It is strongly recommended that this property must not be used to retrieve
     * time from ECUs using protocols (GNSS, NTP, Telephony etc). Since these
     * protocols are already supported by Android, it is recommended to use
     * Android’s own systems for them instead of wiring those through the VHAL
     * using this property.
     *
     * WARNING: The value available through this property should not be dependent
     * on value written by Android to ANDROID_EPOCH_TIME property in any way.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:MILLI_SECS
     */
    EXTERNAL_CAR_TIME = 0x0608 + 0x10000000 // VehiclePropertyGroup:SYSTEM
            + 0x01000000 // VehicleArea:GLOBAL
            + 0x00500000, // VehiclePropertyType:INT64
    /**
     * Current date and time, encoded as Epoch time (in milliseconds).
     * This value denotes the number of milliseconds seconds that have
     * elapsed since 1/1/1970 UTC.
     *
     * CarServices will write to this value to give VHAL the Android system's
     * time, if the VHAL supports this property. This can be useful to
     * synchronize other vehicle systems (dash clock etc) with Android's time.
     *
     * AAOS writes to this property once during boot, and
     * will thereafter write only when some time-source changes are propagated.
     * AAOS will fill in VehiclePropValue.timestamp correctly.
     * Note that AAOS will not send updates for natural elapse of time.
     *     int64Values[0] = provided Unix time (in milliseconds)
     *
     * Note that the property may take >0 ms to get propagated through the stack
     * and, having a timestamped property helps reduce any time drift. So,
     * for all writes to the property, the timestamp can be used to negate this
     * drift:
     *     drift = elapsedTime - PropValue.timestamp
     *     effectiveTime = PropValue.value.int64Values[0] + drift
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     * @unit VehicleUnit:MILLI_SECS
     */
    ANDROID_EPOCH_TIME = 0x0606 + 0x10000000 + 0x01000000
            + 0x00500000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT64
    /**
     * External encryption binding seed.
     *
     * This value is mixed with the local key storage encryption key.
     * This property holds 16 bytes, and is expected to be persisted on an ECU separate from
     * the IVI. The property is initially set by AAOS, who generates it using a CSRNG.
     * AAOS will then read the property on subsequent boots. The binding seed is expected to be
     * reliably persisted. Any loss of the seed results in a factory reset of the IVI.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    STORAGE_ENCRYPTION_BINDING_SEED = 0x0607 + 0x10000000 + 0x01000000
            + 0x00700000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BYTES
    /**
     * Outside temperature
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:CELSIUS
     */
    ENV_OUTSIDE_TEMPERATURE = 0x0703 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT
    /**
     * Property to control power state of application processor
     *
     * It is assumed that AP's power state is controlled by a separate power
     * controller.
     *
     * For configuration information, VehiclePropConfig.configArray must have bit flag combining
     * values in VehicleApPowerStateConfigFlag.
     *
     *   int32Values[0] : VehicleApPowerStateReq enum value
     *   int32Values[1] : additional parameter relevant for each state,
     *                    0 if not used.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    AP_POWER_STATE_REQ = 0x0A00 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Property to report power state of application processor
     *
     * It is assumed that AP's power state is controller by separate power
     * controller.
     *
     *   int32Values[0] : VehicleApPowerStateReport enum value
     *   int32Values[1] : Time in ms to wake up, if necessary.  Otherwise 0.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    AP_POWER_STATE_REPORT = 0x0A01 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Property to report bootup reason for the current power on. This is a
     * static property that will not change for the whole duration until power
     * off. For example, even if user presses power on button after automatic
     * power on with door unlock, bootup reason must stay with
     * VehicleApPowerBootupReason#USER_UNLOCK.
     *
     * int32Values[0] must be VehicleApPowerBootupReason.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    AP_POWER_BOOTUP_REASON = 0x0A02 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Property to represent brightness of the display.
     *
     * Some cars have single control for the brightness of all displays and this
     * property is to share change in that control. In cars which have displays
     * whose brightness is controlled separately, they must use
     * PER_DISPLAY_BRIGHTNESS.
     *
     * Only one of DISPLAY_BRIGHTNESS and PER_DISPLAY_BRIGHTNESS should be
     * implemented. If both are available, PER_DISPLAY_BRIGHTNESS is used by
     * AAOS.
     *
     * If this is writable, android side can set this value when user changes
     * display brightness from Settings. If this is read only, user may still
     * change display brightness from Settings, but that must not be reflected
     * to other displays.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    DISPLAY_BRIGHTNESS = 0x0A03 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Property to represent brightness of the displays which are controlled separately.
     *
     * Some cars have one or more displays whose brightness is controlled
     * separately and this property is to inform the brightness of each
     * passenger display. In cars where all displays' brightness is controlled
     * together, they must use DISPLAY_BRIGHTNESS.
     *
     * Only one of DISPLAY_BRIGHTNESS and PER_DISPLAY_BRIGHTNESS should be
     * implemented. If both are available, PER_DISPLAY_BRIGHTNESS is used by
     * AAOS.
     *
     * The display port uniquely identifies a physical connector on the device
     * for display output, ranging from 0 to 255.
     *
     * int32Values[0] : display port
     * int32Values[1] : brightness
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    PER_DISPLAY_BRIGHTNESS = 0x0A04 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Valet mode enabled
     *
     * This property allows the user to enable/disable valet mode in their vehicle. Valet mode is
     * a privacy and security setting that prevents an untrusted driver to access more private areas
     * in the vehicle, such as the glove box or the trunk(s).
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    VALET_MODE_ENABLED =
            0x0A05 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,
    /**
     * Property to feed H/W input events to android
     *
     * int32Values[0] : action defined by VehicleHwKeyInputAction
     * int32Values[1] : key code, must use standard android key code
     * int32Values[2] : target display defined in VehicleDisplay. Events not
     *                  tied to specific display must be sent to
     *                  VehicleDisplay#MAIN.
     * int32Values[3] : [optional] Number of ticks. The value must be equal or
     *                  greater than 1. When omitted, Android will default to 1.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @config_flags
     */
    HW_KEY_INPUT = 0x0A10 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Property to feed H/W input events to android
     *
     * int32array[0]: target display defined by VehicleDisplay like VehicleDisplay::MAIN,
     *                VehicleDisplay::INSTRUMENT_CLUSTER, VehicleDisplay::AUX
     * int32array[1]: key code, must use standard android key code like KEYCODE_HOME, KEYCODE_BACK
     * int32array[2]: action defined in VehicleHwKeyInputAction like
     *                VehicleHwKeyInputAction::ACTION_UP, VehicleHwKeyInputAction::ACTION_UP
     * int32array[3]: repeat count of the event. For key down events, this is the repeat count
     *                with the first down starting at 0 and counting up from there. For key up
     *                events, this is always equal to 0
     *
     * int64array[0]: down time, elapsed nanoseconds since boot. Denotes the time of the most
     *                recent key down event. For the down event, it will be the event time of the
     *                down event itself
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @config_flags
     */
    HW_KEY_INPUT_V2 =
            0x0A11 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.MIXED,
    /**
     * Property to feed H/W input events to android
     *
     * int32array[0]: target display defined by VehicleDisplay like VehicleDisplay::MAIN,
     *                VehicleDisplay::INSTRUMENT_CLUSTER, VehicleDisplay::AUX
     * int32array[1]: input type defined in VehicleHwMotionInputSource like
     *                VehicleHwMotionInputSource::SOURCE_KEYBOARD,
     *                VehicleHwMotionInputSource::SOURCE_DPAD
     * int32array[2]: action code defined in VehicleHwMotionInputAction like
     *                VehicleHwMotionInputAction::ACTION_UP, VehicleHwMotionInputAction::ACTION_DOWN
     * int32array[3]: button state flag defined in VehicleHwMotionButtonStateFlag like
     *                VehicleHwMotionButtonStateFlag::BUTTON_PRIMARY,
     *                VehicleHwMotionButtonStateFlag::BUTTON_SECONDARY
     * int32array[4]: pointer events count, N. N must be a positive integer
     * int32array[5:5+N-1]: pointer id, length N
     * int32array[5+N:5+2*N-1] : tool type, length N. As defined in VehicleHwMotionToolType like
     *                           VehicleHwMotionToolType::TOOL_TYPE_FINGER,
     *                           VehicleHwMotionToolType::TOOL_TYPE_STYLUS
     *
     * floatArray[0:N-1] : x data, length N
     * floatArray[N:2*N-1] : y data, length N
     * floatArray[2*N:3*N-1] : pressure data, length N
     * floatArray[3*N:4*N-1] : size data, length N
     *
     * int64array[0]: down time, elapsed nanoseconds since boot. Denotes the time when the user
     *                originally pressed down to start a stream of position events. For the down
     *                event, it will be the event time of the down event itself
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @config_flags
     */
    HW_MOTION_INPUT =
            0x0A12 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.MIXED,
    /**
     * Property to feed H/W rotary events to android
     *
     * int32Values[0] : RotaryInputType identifying which rotary knob rotated
     * int32Values[1] : number of detents (clicks), positive for clockwise,
     *                  negative for counterclockwise
     * int32Values[2] : target display defined in VehicleDisplay. Events not
     *                  tied to specific display must be sent to
     *                  VehicleDisplay#MAIN.
     * int32values[3 .. 3 + abs(number of detents) - 2]:
     *                  nanosecond deltas between pairs of consecutive detents,
     *                  if the number of detents is > 1 or < -1
     *
     * VehiclePropValue.timestamp: when the rotation occurred. If the number of
     *                             detents is > 1 or < -1, this is when the
     *                             first detent of rotation occurred.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @data_enum RotaryInputType
     * @access VehiclePropertyAccess.READ
     */
    HW_ROTARY_INPUT = 0x0A20 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Defines a custom OEM partner input event.
     *
     * This input event must be used by OEM partners who wish to propagate events not supported
     * by Android. It is composed by an array of int32 values only.
     *
     * The Android properties are:
     *
     * int32Values[0] : Input code identifying the function representing this event. Valid event
     *                  types are defined by CustomInputType.CUSTOM_EVENT_F1 up to
     *                  CustomInputType.CUSTOM_EVENT_F10. They represent the custom event to be
     *                  defined by OEM partners.
     * int32Values[1] : target display type defined in VehicleDisplay. Events not tied to specific
     *                  display must be sent to VehicleDisplay#MAIN.
     * int32Values[2] : repeat counter, if 0 then event is not repeated. Values 1 or above means
     *                  how many times this event repeated.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @data_enum CustomInputType
     * @access VehiclePropertyAccess.READ
     */
    HW_CUSTOM_INPUT = 0X0A30 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /***************************************************************************
     * Most Car Cabin properties have both a POSition and MOVE parameter.  These
     * are used to control the various movements for seats, doors, and windows
     * in a vehicle.
     *
     * A POS parameter allows the user to set the absolution position.  For
     * instance, for a door, 0 indicates fully closed and max value indicates
     * fully open.  Thus, a value halfway between min and max must indicate
     * the door is halfway open.
     *
     * A MOVE parameter moves the device in a particular direction.  The sign
     * indicates direction, and the magnitude indicates speed (if multiple
     * speeds are available).  For a door, a move of -1 will close the door, and
     * a move of +1 will open it.  Once a door reaches the limit of open/close,
     * the door should automatically stop moving.  The user must NOT need to
     * send a MOVE(0) command to stop the door at the end of its range.
     **************************************************************************/

    /**
     * Door position
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the door is closed. The minInt32Value must be 0.
     * The maxInt32Value indicates the door is fully open.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * closed and fully open positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * Some vehicles (minivans) can open the door electronically. Hence, the
     * ability to write this property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    DOOR_POS = 0x0B00 + 0x10000000 + 0x06000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:DOOR,VehiclePropertyType:INT32
    /**
     * Door move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the door while opening.
     * The minInt32Value represents the maximum movement speed of the door while closing.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the door reaches the positional limit, the value must reset to 0. If DOOR_MOVE's value is
     * currently 0, then that means there is no movement currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    DOOR_MOVE = 0x0B01 + 0x10000000 + 0x06000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:DOOR,VehiclePropertyType:INT32
    /**
     * Door lock
     *
     * 'true' indicates door is locked
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    DOOR_LOCK = 0x0B02 + 0x10000000 + 0x06000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:DOOR,VehiclePropertyType:BOOLEAN
    /**
     * Door child lock feature enabled
     *
     * Returns true if the door child lock feature is enabled and false if it is disabled.
     *
     * If enabled, the door is unable to be opened from the inside.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    DOOR_CHILD_LOCK_ENABLED =
            0x0B03 + VehiclePropertyGroup.SYSTEM + VehicleArea.DOOR + VehiclePropertyType.BOOLEAN,
    /**
     * Mirror Z Position
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the mirror is tilted completely downwards. This must be a
     * non-positive value.
     * The maxInt32Value indicates the mirror is tilted completely upwards. This must be a
     * non-negative value.
     * 0 indicates the mirror is not tilted in either direction.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * fully downward and fully upwards positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    MIRROR_Z_POS = 0x0B40 + 0x10000000 + 0x04000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:MIRROR,VehiclePropertyType:INT32
    /**
     * Mirror Z Move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the mirror while tilting upwards.
     * The minInt32Value represents the maximum movement speed of the mirror while tilting
     * downwards.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the mirror reaches the positional limit, the value must reset to 0. If MIRROR_Z_MOVE's value
     * is currently 0, then that means there is no movement currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    MIRROR_Z_MOVE = 0x0B41 + 0x10000000 + 0x04000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:MIRROR,VehiclePropertyType:INT32
    /**
     * Mirror Y Position
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the mirror is tilted completely to the left. This must be a
     * non-positive value.
     * The maxInt32Value indicates the mirror is tilted completely to the right. This must be a
     * non-negative value.
     * 0 indicates the mirror is not tilted in either direction.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * left extreme and right extreme positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    MIRROR_Y_POS = 0x0B42 + 0x10000000 + 0x04000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:MIRROR,VehiclePropertyType:INT32
    /**
     * Mirror Y Move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the mirror while tilting right.
     * The minInt32Value represents the maximum movement speed of the mirror while tilting left.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the mirror reaches the positional limit, the value must reset to 0. If MIRROR_Y_MOVE's value
     * is currently 0, then that means there is no movement currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    MIRROR_Y_MOVE = 0x0B43 + 0x10000000 + 0x04000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:MIRROR,VehiclePropertyType:INT32
    /**
     * Mirror Lock
     *
     * True indicates mirror positions are locked and not changeable
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    MIRROR_LOCK = 0x0B44 + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * Mirror Fold
     *
     * True indicates mirrors are folded
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    MIRROR_FOLD = 0x0B45 + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN

    /**
     * Represents property for Mirror Auto Fold feature.
     *
     * This property is true when the feature for automatically folding the vehicle's side mirrors
     * (for example, when the mirrors fold inward automatically when one exits and locks the
     * vehicle) is enabled.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */

    MIRROR_AUTO_FOLD_ENABLED =
            0x0B46 + VehiclePropertyGroup.SYSTEM + VehicleArea.MIRROR + VehiclePropertyType.BOOLEAN,

    /**
     * Represents property for Mirror Auto Tilt feature.
     *
     * This property is true when the feature for automatically tilting the vehicle's side mirrors
     * (for example, when the mirrors tilt downward automatically when one reverses the vehicle) is
     * enabled.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */

    MIRROR_AUTO_TILT_ENABLED =
            0x0B47 + VehiclePropertyGroup.SYSTEM + VehicleArea.MIRROR + VehiclePropertyType.BOOLEAN,

    /**
     * Seat memory select
     *
     * This parameter selects the memory preset to use to select the seat position. The
     * maxInt32Value and minInt32Value in VehicleAreaConfig must be defined. All integers between
     * minInt32Value and maxInt32Value must be supported. The minInt32Value is always 0, and the
     * maxInt32Value determines the number of seat preset memory slots available (i.e.
     * numSeatPresets - 1).
     *
     * For instance, if the driver's seat has 3 memory presets, the maxInt32Value will be 2. When
     * the user wants to select a preset, the desired preset number (0, 1, or 2) is set.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     */
    SEAT_MEMORY_SELECT = 0x0B80 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat memory set
     *
     * This setting allows the user to save the current seat position settings into the selected
     * preset slot. The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined. The
     * minInt32Value must be 0, and the maxInt32Value for each seat position must match the
     * maxInt32Value for SEAT_MEMORY_SELECT.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     */
    SEAT_MEMORY_SET = 0x0B81 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seatbelt buckled
     *
     * True indicates belt is buckled.
     *
     * Write access indicates automatic seat buckling capabilities.  There are
     * no known cars at this time, but you never know...
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_BELT_BUCKLED = 0x0B82 + 0x10000000 + 0x05000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:BOOLEAN
    /**
     * Seatbelt height position
     *
     * Adjusts the shoulder belt anchor point.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the seat belt's shoulder anchor is at its lowest position.
     * The maxInt32Value indicates the seat belt's shoulder anchor is at its highest position.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * lowest and highest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_BELT_HEIGHT_POS = 0x0B83 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seatbelt height move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat belt's shoulder anchor
     * while moving upwards.
     * The minInt32Value represents the maximum movement speed of the seat belt's shoulder anchor
     * while moving downwards.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat belt reaches the positional limit, the value must reset to 0. If
     * SEAT_BELT_HEIGHT_MOVE's value is currently 0, then that means there is no movement currently
     * occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_BELT_HEIGHT_MOVE = 0x0B84 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat fore/aft position
     *
     * Sets the seat position forward and backwards.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the seat is at its rearward-most linear position.
     * The maxInt32Value indicates the seat is at its forward-most linear position.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * closest and farthest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_FORE_AFT_POS = 0x0B85 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat fore/aft move
     *
     * This property moves the entire seat forward/backward in the direction that it is facing.
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat while moving forward.
     * The minInt32Value represents the maximum movement speed of the seat while moving backward.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat reaches the positional limit, the value must reset to 0. If SEAT_FORE_AFT_MOVE's
     * value is currently 0, then that means there is no movement currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_FORE_AFT_MOVE = 0x0B86 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat backrest angle 1 position
     *
     * Backrest angle 1 is the actuator closest to the bottom of the seat.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the seat backrest's full recline position w.r.t the
     * actuator at the bottom of the seat.
     * The maxInt32Value indicates the seat backrest's most upright/forward position w.r.t the
     * actuator at the bottom of the seat.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * full recline and upright/forward positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_BACKREST_ANGLE_1_POS = 0x0B87 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat backrest angle 1 move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat backrest while angling
     * forward.
     * The minInt32Value represents the maximum movement speed of the seat backrest while reclining.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat backrest reaches the positional limit, the value must reset to 0. If
     * SEAT_BACKREST_ANGLE_1_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_BACKREST_ANGLE_1_MOVE = 0x0B88 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat backrest angle 2 position
     *
     * Backrest angle 2 is the next actuator up from the bottom of the seat.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the seat backrest's full recline position w.r.t the next
     * actuator in the backrest from the one at the bottom of the seat (see
     * SEAT_BACKREST_ANGLE_1_POS for additional details).
     * The maxInt32Value indicates the seat backrest's most upright/forward position w.r.t the
     * next actuator in the backrest from the one at the bottom of the seat(see
     * SEAT_BACKREST_ANGLE_1_POS for additional details).
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * full recline and upright/forward positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_BACKREST_ANGLE_2_POS = 0x0B89 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat backrest angle 2 move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat backrest while angling
     * forward.
     * The minInt32Value represents the maximum movement speed of the seat backrest while reclining.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat backrest reaches the positional limit, the value must reset to 0. If
     * SEAT_BACKREST_ANGLE_2_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_BACKREST_ANGLE_2_MOVE = 0x0B8A + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat height position
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the seat is in its lowest position.
     * The maxInt32Value indicates the seat is in its highest position.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * lowest and highest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_HEIGHT_POS = 0x0B8B + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat height move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat while moving upward.
     * The minInt32Value represents the maximum movement speed of the seat while moving downward.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat reaches the positional limit, the value must reset to 0. If SEAT_HEIGHT_MOVE's value
     * is currently 0, then that means there is no movement currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_HEIGHT_MOVE = 0x0B8C + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat depth position
     *
     * Sets the seat depth, distance from back rest to front edge of seat.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the seat is in its shallowest position (i.e. the position with
     * the smallest distance between the front edge of the seat cushion and the rear end of the
     * seat).
     * The maxInt32Value indicates the seat is in its deepest position (i.e. the position with the
     * largest distance between the front edge of the seat cushion and the rear end of the seat).
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * shallowest and deepest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_DEPTH_POS = 0x0B8D + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat depth move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat while getting deeper
     * The minInt32Value represents the maximum movement speed of the seat while getting shallower.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat backrest reaches the positional limit, the value must reset to 0. If
     * SEAT_DEPTH_MOVE's value is currently 0, then that means there is no movement currently
     * occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_DEPTH_MOVE = 0x0B8E + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat tilt position
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the seat bottom is angled at its lowest angular position. This
     * corresponds to the seat's front edge at its lowest possible position relative to the rear
     * end of the seat.
     * The maxInt32Value indicates the seat bottom is angled at its highest angular position. This
     * corresponds to the seat's front edge at its highest possible position relative to the rear
     * end of the seat.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * lowest and highest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_TILT_POS = 0x0B8F + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Seat tilt move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the front edge of the seat while
     * moving upward.
     * The minInt32Value represents the maximum movement speed of the front edge of the seat while
     * moving downward.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat bottom reaches the positional limit, the value must reset to 0. If SEAT_TILT_MOVE's
     * value is currently 0, then that means there is no movement currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_TILT_MOVE = 0x0B90 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Lumber fore/aft position
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the lumbar support is in its rearward most position (i.e. least
     * supportive position).
     * The maxInt32Value indicates the lumbar support is in its forward most position (i.e. most
     * supportive position).
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * forward and rearward positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_LUMBAR_FORE_AFT_POS = 0x0B91 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Lumbar fore/aft move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat's lumbar support while
     * moving forward.
     * The minInt32Value represents the maximum movement speed of the seat's lumbar support while
     * moving backward.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat's lumbar support reaches the positional limit, the value must reset to 0. If
     * SEAT_LUMBAR_FORE_AFT_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_LUMBAR_FORE_AFT_MOVE = 0x0B92 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Lumbar side support position
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the lumbar side support is in its thinnest position (i.e.
     * most support).
     * The maxInt32Value indicates the lumbar side support is in its widest position (i.e.
     * least support).
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * thinnest and widest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_LUMBAR_SIDE_SUPPORT_POS = 0x0B93 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Lumbar side support move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat's lumbar side support
     * while getting wider.
     * The minInt32Value represents the maximum movement speed of the seat's lumbar side support
     * while getting thinner.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat's lumbar side support reaches the positional limit, the value must reset to 0. If
     * SEAT_LUMBAR_SIDE_SUPPORT_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_LUMBAR_SIDE_SUPPORT_MOVE = 0x0B94 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * (Deprecated) Headrest height position
     *
     * This property is deprecated because it is defined as type VehicleArea:GLOBAL, which means all
     * seats use the same value. Use SEAT_HEADREST_HEIGHT_POS_V2 instead which fixes this issue by
     * being defined as type VehicleArea:SEAT.
     *
     * Sets the headrest height.
     * Max value indicates tallest setting.
     * Min value indicates shortest setting.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_HEADREST_HEIGHT_POS = 0x0B95 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * Headrest height position
     *
     * Sets the headrest height for supported seats. VehiclePropConfig.areaConfigs specifies which
     * seats are supported.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the headrest is in its lowest position.
     * The maxInt32Value indicates the headrest is in its highest position.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * lowest and highest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_HEADREST_HEIGHT_POS_V2 =
            0x0BA4 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.INT32,

    /**
     * Headrest height move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat's headrest while moving
     * up.
     * The minInt32Value represents the maximum movement speed of the seat's headrest while moving
     * down.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat's headrest reaches the positional limit, the value must reset to 0. If
     * SEAT_HEADREST_HEIGHT_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_HEADREST_HEIGHT_MOVE = 0x0B96 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Headrest angle position
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the headrest is in its full recline position.
     * The maxInt32Value indicates the headrest is in its most upright/forward position.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * full recline and most upright/forward positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_HEADREST_ANGLE_POS = 0x0B97 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Headrest angle move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat's headrest while moving
     * into an upright/forward position.
     * The minInt32Value represents the maximum movement speed of the seat's headrest while moving
     * into a shallow position.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat's headrest reaches the positional limit, the value must reset to 0. If
     * SEAT_HEADREST_ANGLE_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_HEADREST_ANGLE_MOVE = 0x0B98 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Headrest fore/aft position
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the headrest is in its rearward-most linear position.
     * The maxInt32Value indicates the headrest is in its forward-most linear position.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * forward and rearward positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_HEADREST_FORE_AFT_POS = 0x0B99 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Headrest fore/aft move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat's headrest while moving
     * forward.
     * The minInt32Value represents the maximum movement speed of the seat's headrest while moving
     * backward.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat's headrest reaches the positional limit, the value must reset to 0. If
     * SEAT_HEADREST_FORE_AFT_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_HEADREST_FORE_AFT_MOVE = 0x0B9A + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Represents property for the seat footwell lights state.
     *
     * SEAT_FOOTWELL_LIGHTS_STATE reflects the current state of the lights at any point in time.
     * This is different from the function of SEAT_FOOTWELL_LIGHTS_SWITCH which represents the
     * position of the switch controlling the lights. Therefore, SEAT_FOOTWELL_LIGHTS_STATE may not
     * match the value of SEAT_FOOTWELL_LIGHTS_SWITCH (e.g. SEAT_FOOTWELL_LIGHTS_SWITCH=AUTOMATIC
     * and SEAT_FOOTWELL_LIGHTS_STATE=ON).
     *
     * This property should only be implemented if SEAT_FOOTWELL_LIGHTS_STATE's value may be
     * different from that of CABIN_LIGHTS_STATE.
     *
     * For each supported area ID, the VehicleAreaConfig#supportedEnumValues must be defined unless
     * all enum values of VehicleLightState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightState
     */
    SEAT_FOOTWELL_LIGHTS_STATE =
            0x0B9B + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.INT32,
    /**
     * Represents property for the seat footwell lights switch.
     *
     * SEAT_FOOTWELL_LIGHTS_SWITCH represents the position of the switch controlling the lights.
     * This is different from the function of SEAT_FOOTWELL_LIGHTS_STATE which reflects the current
     * state of the lights at any point in time. Therefore, SEAT_FOOTWELL_LIGHTS_SWITCH may not
     * match the value of SEAT_FOOTWELL_LIGHTS_STATE (e.g. SEAT_FOOTWELL_LIGHTS_SWITCH=AUTOMATIC and
     * SEAT_FOOTWELL_LIGHTS_STATE=ON).
     *
     * This property should only be implemented if SEAT_FOOTWELL_LIGHTS_SWITCH's value may be
     * different from that of CABIN_LIGHTS_SWITCH.
     *
     * For each supported area ID, the VehicleAreaConfig#supportedEnumValues must be defined unless
     * all enum values of VehicleLightSwitch are supported.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightSwitch
     */
    SEAT_FOOTWELL_LIGHTS_SWITCH =
            0x0B9C + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.INT32,
    /**
     * Represents property for Seat easy access feature.
     *
     * If true, the seat will automatically adjust to make it easier for the occupant to enter and
     * exit the vehicle. Each area ID must map to the seat that the user is trying to enter/exit
     * with the help of the easy access feature.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_EASY_ACCESS_ENABLED =
            0x0B9D + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.BOOLEAN,
    /**
     * Represents feature to enable/disable a seat's ability to deploy airbag(s) when triggered
     * (e.g. by a crash).
     *
     * If true, it means the seat's airbags are enabled, and if triggered (e.g. by a crash), they
     * will deploy. If false, it means the seat's airbags are disabled, and they will not deploy
     * under any circumstance. This property does not indicate if the airbags are deployed or not.
     *
     * This property can be set to VehiclePropertyAccess.READ read only for the sake of regulation
     * or safety concerns.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_AIRBAG_ENABLED =
            0x0B9E + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.BOOLEAN,
    /**
     * Seat airbags deployed
     *
     * Bit flag property to relay information on which airbags have been deployed in the vehicle at
     * each seat, vs which ones are currently still armed. If SEAT_AIRBAG_ENABLED is set to false at
     * a particular areaId, this property should return status code UNAVAILABLE at that areaId.
     *
     * Enums apply to each seat, not the global vehicle. For example, VehicleAirbagsLocation#CURTAIN
     * at the driver seat areaId represents whether the driver side curtain airbag has been
     * deployed. Multiple bit flags can be set to indicate that multiple different airbags have been
     * deployed for the seat.
     *
     * For each seat area ID, the VehicleAreaConfig#supportedEnumValues array must be defined unless
     * all states of VehicleAirbagLocation are supported (including OTHER, which is not
     * recommended).
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleAirbagLocation
     */
    SEAT_AIRBAGS_DEPLOYED =
            0x0BA5 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.INT32,
    /**
     * Represents property for seat’s hipside (bottom cushion’s side) support position.
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value indicates the seat cushion side support is in its widest position (i.e.
     * least support).
     * The minInt32Value indicates the seat cushion side support is in its thinnest position (i.e.
     * most support).
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * thinnest and widest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_CUSHION_SIDE_SUPPORT_POS =
            0x0B9F + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.INT32,
    /**
     * Represents property for movement direction and speed of seat cushion side support.
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value represents the maximum movement speed of the seat cushion side support when
     * growing wider (i.e. support is decreasing).
     * The minInt32Value represents the maximum movement speed of the seat cushion side support when
     * growing thinner (i.e. support is increasing).
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat cushion side support reaches the positional limit, the value must reset to 0. If
     * SEAT_CUSHION_SIDE_SUPPORT_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_CUSHION_SIDE_SUPPORT_MOVE =
            0x0BA0 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.INT32,
    /**
     * Represents property for seat’s lumbar support vertical position.
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value indicates the lumbar support's highest position.
     * The minInt32Value indicates the lumbar support's lowest position.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * lowest and highest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_LUMBAR_VERTICAL_POS =
            0x0BA1 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.INT32,
    /**
     * Represents property for vertical movement direction and speed of seat lumbar support.
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value indicates the lumbar support is moving at the fastest upward speed.
     * The minInt32Value indicates the lumbar support is moving at the fastest downward speed.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the seat cushion side support reaches the positional limit, the value must reset to 0. If
     * SEAT_LUMBAR_VERTICAL_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_LUMBAR_VERTICAL_MOVE =
            0x0BA2 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.INT32,
    /**
     * Represents property that indicates the current walk-in position of the seat.
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the normal seat position. The minInt32Value must be 0.
     * The maxInt32Value indicates the seat is in the full walk-in position.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * normal and walk-in positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * The area ID must match the seat that actually moves when the walk-in feature activates, not
     * the intended seat the passengers will sit in.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_WALK_IN_POS =
            0x0BA3 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.INT32,
    /**
     * Seat belt pretensioner deployed.
     *
     * Property to relay information on whether the seat belt pretensioner has been deployed for a
     * particular seat due to a collision. This is different from the regular seat belt tightening
     * system that continuously adds tension to the seat belts so that they fit snugly around the
     * person sitting in the seat, nor is it the seat belt retractor system that locks the seat belt
     * in place during sudden brakes or when the user jerks the seat belt.
     *
     * If this property is dependant on the state of other properties, and those properties are
     * currently in the state that doesn't support this property, this should return
     * StatusCode#NOT_AVAILABLE
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    SEAT_BELT_PRETENSIONER_DEPLOYED =
            0x0BA6 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.BOOLEAN,
    /**
     * Seat Occupancy
     *
     * Indicates whether a particular seat is occupied or not, to the best of the car's ability
     * to determine. Valid values are from the VehicleSeatOccupancyState enum.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleSeatOccupancyState
     */
    SEAT_OCCUPANCY = 0x0BB0 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Window Position
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates the window is closed/fully open out of plane. If the window
     * cannot open out of plane, then minInt32Value is the position of the window when fully closed
     * and must be 0. If the window can open out of plane, the minInt32Value indicates the window
     * is fully open in its position out of plane and will be a negative value. See the example
     * below for a more detailed explanation.
     * The maxInt32Value indicates the window is fully open.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * closed/fully open out-of-plane and fully open positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * For example, this is how the property should work for a window that can move out of plane:
     *  For a window that may open out of plane (i.e. vent mode of sunroof) this
     *  parameter will work with negative values as follows:
     *    Max = sunroof completely open
     *    0 = sunroof closed.
     *    Min = sunroof vent completely open
     *
     *    Note that in this mode, 0 indicates the window is closed.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    WINDOW_POS = 0x0BC0 + 0x10000000 + 0x03000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:WINDOW,VehiclePropertyType:INT32
    /**
     * Window Move
     *
     * The maxInt32Value and minInt32Value in each VehicleAreaConfig must be defined. All integers
     * between minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value indicates the window is opening in plane/closing in the out of plane
     * direction at the fastest speed.
     * The minInt32Value indicates the window is closing in plane/opening in the out of plane
     * direction at the fastest speed.
     *
     * Larger absolute values, either positive or negative, indicate a faster movement speed. Once
     * the window reaches the positional limit, the value must reset to 0. If WINDOW_MOVE's value is
     * currently 0, then that means there is no movement currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * For a window that may open out of plane (i.e. vent mode of sunroof) this
     * parameter will work as follows:
     *
     * If sunroof is open:
     *   Max = open the sunroof further, automatically stop when fully open.
     *   Min = close the sunroof, automatically stop when sunroof is closed.
     *
     * If vent is open:
     *   Max = close the vent, automatically stop when vent is closed.
     *   Min = open the vent further, automatically stop when vent is fully open.
     *
     * If sunroof is in the closed position:
     *   Max = open the sunroof, automatically stop when sunroof is fully open.
     *   Min = open the vent, automatically stop when vent is fully open.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    WINDOW_MOVE = 0x0BC1 + 0x10000000 + 0x03000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:WINDOW,VehiclePropertyType:INT32
    /**
     * Window Lock
     *
     * True indicates windows are locked and can't be moved.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    WINDOW_LOCK = 0x0BC4 + 0x10000000 + 0x03000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:WINDOW,VehiclePropertyType:BOOLEAN
    /**
     * Windshield wipers period (milliseconds).
     *
     * Returns the instantaneous time period for 1 full cycle of the windshield wipers in
     * milliseconds. A full cycle is defined as a wiper moving from and returning to its rest
     * position.
     *
     * When an intermittent wiper setting is selected, this property value must be set to 0 during
     * the "pause" period of the intermittent wiping.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined. The maxInt32Value
     * for each area ID must specify the longest wiper period. The minInt32Value must be set to 0
     * for each area ID.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:MILLI_SECS
     */
    WINDSHIELD_WIPERS_PERIOD =
            0x0BC5 + VehiclePropertyGroup.SYSTEM + VehicleArea.WINDOW + VehiclePropertyType.INT32,

    /**
     * Windshield wipers state.
     *
     * Returns the current state of the windshield wipers. The value of WINDSHIELD_WIPERS_STATE may
     * not match the value of WINDSHIELD_WIPERS_SWITCH. (e.g. WINDSHIELD_WIPERS_STATE = ON and
     * WINDSHIELD_WIPERS_SWITCH = WindshieldWipersSwitch#AUTO).
     *
     * If WINDSHIELD_WIPERS_STATE = ON and WINDSHIELD_WIPERS_PERIOD is implemented, then
     * WINDSHIELD_WIPERS_PERIOD must reflect the time period of 1 full cycle of the wipers.
     *
     * For each supported area ID, the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states in WindshieldWipersState are supported (including OTHER, which is not
     * recommended).
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum WindshieldWipersState
     */
    WINDSHIELD_WIPERS_STATE =
            0x0BC6 + VehiclePropertyGroup.SYSTEM + VehicleArea.WINDOW + VehiclePropertyType.INT32,

    /**
     * Windshield wipers switch.
     *
     * Represents the position of the switch controlling the windshield wipers. The value of
     * WINDSHIELD_WIPERS_SWITCH may not match the value of WINDSHIELD_WIPERS_STATE (e.g.
     * WINDSHIELD_WIPERS_SWITCH = AUTO and WINDSHIELD_WIPERS_STATE = WindshieldWipersState#ON).
     *
     * For each supported area ID, the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states in WindshieldWipersSwitch are supported (including OTHER, which is not
     * recommended).
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * If this property is implemented as VehiclePropertyAccess.READ_WRITE and the OTHER state is
     * listed in the VehicleAreaConfig#supportedEnumValues array, then OTHER is not a supported
     * value for writing. It is only a supported value for reading.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum WindshieldWipersSwitch
     */
    WINDSHIELD_WIPERS_SWITCH =
            0x0BC7 + VehiclePropertyGroup.SYSTEM + VehicleArea.WINDOW + VehiclePropertyType.INT32,

    /**
     * Steering wheel depth position
     *
     * All steering wheel properties' unique ids start from 0x0BE0.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined. All values between
     * minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value indicates the steering wheel position furthest from the driver.
     * The minInt32Value indicates the steering wheel position closest to the driver.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * closest and furthest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    STEERING_WHEEL_DEPTH_POS =
            0x0BE0 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,
    /**
     * Steering wheel depth movement
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined. All values between
     * minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value indicates the steering wheel moving away from the driver.
     * The minInt32Value indicates the steering wheel moving towards the driver.
     *
     * Larger integers, either positive or negative, indicate a faster movement speed. Once the
     * steering wheel reaches the positional limit, the value must reset to 0. If
     * STEERING_WHEEL_DEPTH_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    STEERING_WHEEL_DEPTH_MOVE =
            0x0BE1 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,
    /**
     * Steering wheel height position
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined. All values between
     * minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value indicates the steering wheel being in the highest position.
     * The minInt32Value indicates the steering wheel being in the lowest position.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * lowest and highest positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    STEERING_WHEEL_HEIGHT_POS =
            0x0BE2 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,
    /**
     * Steering wheel height movement
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined. All values between
     * minInt32Value and maxInt32Value must be supported.
     *
     * The maxInt32Value indicates the steering wheel moving upwards.
     * The minInt32Value indicates the steering wheel moving downwards.
     *
     * Larger integers, either positive or negative, indicate a faster movement speed. Once the
     * steering wheel reaches the positional limit, the value must reset to 0. If
     * STEERING_WHEEL_HEIGHT_MOVE's value is currently 0, then that means there is no movement
     * currently occurring.
     *
     * This property is not in any particular unit but in a specified range of relative movement
     * speeds.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    STEERING_WHEEL_HEIGHT_MOVE =
            0x0BE3 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,
    /**
     * Steering wheel theft lock feature enabled
     *
     * If true, the steering wheel will lock automatically to prevent theft in certain
     * situations.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    STEERING_WHEEL_THEFT_LOCK_ENABLED =
            0x0BE4 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,
    /**
     * Steering wheel locked
     *
     * If true, the steering wheel's position is locked and not changeable.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    STEERING_WHEEL_LOCKED =
            0x0BE5 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,
    /**
     * Steering wheel easy access feature enabled
     *
     * If true, the driver’s steering wheel will automatically adjust to make it easier for the
     * driver to enter and exit the vehicle.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    STEERING_WHEEL_EASY_ACCESS_ENABLED =
            0x0BE6 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,
    /**
     * Property that represents the current position of the glove box door.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * All integers between minInt32Value and maxInt32Value must be supported.
     *
     * The minInt32Value indicates that the glove box door is closed. The minInt32Value must be 0.
     * The maxInt32Value indicates that the glove box door is in the fully open position.
     *
     * Values in between minInt32Value and maxInt32Value indicate a transition state between the
     * closed and fully open positions.
     *
     * This property is not in any particular unit but in a specified range of relative positions.
     *
     * The area ID must match the seat by which the glove box is intended to be used  (e.g. if the
     * front right dashboard has a glove box embedded in it, then the area ID should be
     * SEAT_1_RIGHT).
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    GLOVE_BOX_DOOR_POS =
            0x0BF0 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.INT32,

    /**
     * Lock or unlock the glove box.
     *
     * If true, the glove box is locked. If false, the glove box is unlocked.
     *
     * The area ID must match the seat by which the glove box is intended to be used (e.g. if the
     * front right dashboard has a glove box embedded in it, then the area ID should be
     * VehicleAreaSeat#ROW_1_RIGHT).
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    GLOVE_BOX_LOCKED =
            0x0BF1 + VehiclePropertyGroup.SYSTEM + VehicleArea.SEAT + VehiclePropertyType.BOOLEAN,

    /**
     * Vehicle Maps Service (VMS) message
     *
     * This property uses MIXED data to communicate vms messages.
     *
     * Its contents are to be interpreted as follows:
     * the indices defined in VmsMessageIntegerValuesIndex are to be used to
     * read from int32Values;
     * bytes is a serialized VMS message as defined in the vms protocol
     * which is opaque to the framework;
     *
     * IVehicle#get must always return StatusCode::NOT_AVAILABLE.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    VEHICLE_MAP_SERVICE = 0x0C00 + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * Characterization of inputs used for computing location.
     *
     * This property must indicate what (if any) data and sensor inputs are considered by the system
     * when computing the vehicle's location that is shared with Android through the GNSS HAL.
     *
     * The value must return a collection of bit flags. The bit flags are defined in
     * LocationCharacterization. The value must also include exactly one of DEAD_RECKONED or
     * RAW_GNSS_ONLY among its collection of bit flags.
     *
     * When this property is not supported, it is assumed that no additional sensor inputs are fused
     * into the GNSS updates provided through the GNSS HAL. That is unless otherwise specified
     * through the GNSS HAL interfaces.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    LOCATION_CHARACTERIZATION =
            0x0C10 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Static data for the position of each ultrasonic sensor installed on the vehicle.
     *
     * Each individual sensor is identified by its unique VehicleAreaConfig#areaId and returns the
     * sensor's position formatted as [x, y, z] where:
     *
     *     int32Values[0] = x, the position of the sensor along the x-axis relative to the origin of
     *                      the Android Automotive sensor coordinate frame in millimeters
     *     int32Values[1] = y, the position of the sensor along the y-axis relative to the origin of
     *                      the Android Automotive sensor coordinate frame in millimeters.
     *     int32Values[2] = z, the position of the sensor along the z-axis relative to the origin of
     *                      the Android Automotive sensor coordinate frame in millimeters.
     *
     * If the data is aggregated by another ECU, then OEMs have the option of reporting the same
     * reading across all included sensors or reporting a virtual representation of all the included
     * sensors as if they were one sensor.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    ULTRASONICS_SENSOR_POSITION = 0x0C20 + VehiclePropertyGroup.SYSTEM + VehicleArea.VENDOR
            + VehiclePropertyType.INT32_VEC,

    /**
     * Static data for the orientation of each ultrasonic sensor installed on the vehicle.
     *
     * Each individual sensor is identified by its VehicleAreaConfig#areaId and returns the sensor's
     * orientation formatted as [qw, qx, qy, qz] where:
     *
     *     int32Values[0] = qw, the quaternion coefficient w within the quaterinion (w + xi + yj +
     *                      zk) describing the rotation of the sensor relative to the Android
     *                      Automotive sensor coordinate frame.
     *     int32Values[1] = qx, the quaternion coefficient x within the quaterinion (w + xi + yj +
     *                      zk) describing the rotation of the sensor relative to the Android
     *                      Automotive sensor coordinate frame.
     *     int32Values[2] = qy, the quaternion coefficient y within the quaterinion (w + xi + yj +
     *                      zk) describing the rotation of the sensor relative to the Android
     *                      Automotive sensor coordinate frame.
     *     int32Values[3] = qz, the quaternion coefficient z within the quaterinion (w + xi + yj +
     *                      zk) describing the rotation of the sensor relative to the Android
     *                      Automotive sensor coordinate frame.
     *
     * This assumes each sensor uses the same axes conventions as Android Automotive.
     *
     * If the data is aggregated by another ECU, then OEMs have the option of reporting the same
     * reading across all included sensors or reporting a virtual representation of all the included
     * sensors as if they were one sensor.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    ULTRASONICS_SENSOR_ORIENTATION = 0x0C21 + VehiclePropertyGroup.SYSTEM + VehicleArea.VENDOR
            + VehiclePropertyType.INT32_VEC,

    /**
     * Static data for the field of view of each ultrasonic sensor in degrees.
     *
     * Each individual sensor is identified by its VehicleAreaConfig#areaId and returns the sensor's
     * field of view formatted as [horizontal, vertical] where:
     *
     *     int32Values[0] = horizontal, the horizontal field of view for the specified ultrasonic
     *                      sensor in degrees.
     *     int32Values[1] = vertical, the vertical field of view for the associated specified
     *                      ultrasonic sensor in degrees.
     *
     * This assumes each sensor uses the same axes conventions as Android Automotive.
     *
     * If the data is aggregated by another ECU, then OEMs have the option of reporting the same
     * reading across all included sensors or reporting a virtual representation of all the included
     * sensors as if they were one sensor.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    ULTRASONICS_SENSOR_FIELD_OF_VIEW = 0x0C22 + VehiclePropertyGroup.SYSTEM + VehicleArea.VENDOR
            + VehiclePropertyType.INT32_VEC,

    /**
     * Static data for the detection range of each ultrasonic sensor in millimeters.
     *
     * Each individual sensor is identified by its VehicleAreaConfig#areaId and returns the sensor's
     * detection range formatted as [minimum, maximum] where:
     *
     *     int32Values[0] = minimum, the minimum range detectable by the ultrasonic sensor in
     *                      millimeters.
     *     int32Values[1] = maximum, the maximum range detectable by the ultrasonic sensor in
     *                      millimeters.
     *
     * If the data is aggregated by another ECU, then OEMs have the option of reporting the same
     * reading across all included sensors or reporting a virtual representation of all the included
     * sensors as if they were one sensor.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    ULTRASONICS_SENSOR_DETECTION_RANGE = 0x0C23 + VehiclePropertyGroup.SYSTEM + VehicleArea.VENDOR
            + VehiclePropertyType.INT32_VEC,

    /**
     * Static data for the supported ranges of each ultrasonic sensor in millimeters.
     *
     * For ultrasonic sensors that only support readings within a specific range. For example, if
     * an ultrasonic sensor detects an object at 700mm, but can only report that an object has been
     * detected between 500mm and 1000mm.
     *
     * Each individual sensor is identified by its VehicleAreaConfig#areaId and returns the sensor's
     * supported ranges formatted as [range_min_1, range_max_1, range_min_2, range_max_2, ...]
     * where:
     *
     *     int32Values[0] = range_min_1, the minimum of one supported range by the specified sensor
     *                      in millimeters, inclusive.
     *     int32Values[1] = range_max_1, the maximum of one supported range by the specified sensor
     *                      in millimeters, inclusive.
     *     int32Values[2] = range_min_2, the minimum of another supported range by the specified
     *                      sensor in millimeters, inclusive.
     *     int32Values[3] = range_max_2, the maximum of another supported range by the specified
                            sensor in millimeters, inclusive.
     *
     * Example:
     *     - Ultrasonic sensor supports the following ranges:
     *           - 150mm to 499mm
     *           - 500mm to 999mm
     *           - 1000mm to 1500mm
     *     - The associated supported ranges should be formatted as:
     *           - int32Values[0] = 150
     *           - int32Values[1] = 499
     *           - int32Values[2] = 500
     *           - int32Values[3] = 999
     *           - int32Values[4] = 1000
     *           - int32Values[5] = 1500
     *
     * If this property is not defined, all the values within the ULTRASONICS_SENSOR_DETECTION_RANGE
     * for the specified sensor are assumed to be supported.
     *
     * If the data is aggregated by another ECU, then OEMs have the option of reporting the same
     * reading across all included sensors or reporting a virtual representation of all the included
     * sensors as if they were one sensor.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    ULTRASONICS_SENSOR_SUPPORTED_RANGES = 0x0C24 + VehiclePropertyGroup.SYSTEM + VehicleArea.VENDOR
            + VehiclePropertyType.INT32_VEC,

    /**
     * OBD2 Live Sensor Data
     *
     * Reports a snapshot of the current (live) values of the OBD2 sensors available.
     *
     * The configArray is set as follows:
     *   configArray[0] = number of vendor-specific integer-valued sensors
     *   configArray[1] = number of vendor-specific float-valued sensors
     *
     * The values of this property are to be interpreted as in the following example.
     * Considering a configArray = {2,3}
     * int32Values must be a vector containing Obd2IntegerSensorIndex.LAST_SYSTEM_INDEX + 2
     * elements (that is, 33 elements);
     * floatValues must be a vector containing Obd2FloatSensorIndex.LAST_SYSTEM_INDEX + 3
     * elements (that is, 73 elements);
     *
     * It is possible for each frame to contain a different subset of sensor values, both system
     * provided sensors, and vendor-specific ones. In order to support that, the bytes element
     * of the property value is used as a bitmask,.
     *
     * bytes must have a sufficient number of bytes to represent the total number of possible
     * sensors (in this case, 14 bytes to represent 106 possible values); it is to be read as
     * a contiguous bitmask such that each bit indicates the presence or absence of a sensor
     * from the frame, starting with as many bits as the size of int32Values, immediately
     * followed by as many bits as the size of floatValues.
     *
     * For example, should bytes[0] = 0x4C (0b01001100) it would mean that:
     *   int32Values[0 and 1] are not valid sensor values
     *   int32Values[2 and 3] are valid sensor values
     *   int32Values[4 and 5] are not valid sensor values
     *   int32Values[6] is a valid sensor value
     *   int32Values[7] is not a valid sensor value
     * Should bytes[5] = 0x61 (0b01100001) it would mean that:
     *   int32Values[32] is a valid sensor value
     *   floatValues[0 thru 3] are not valid sensor values
     *   floatValues[4 and 5] are valid sensor values
     *   floatValues[6] is not a valid sensor value
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    OBD2_LIVE_FRAME = 0x0D00 + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * OBD2 Freeze Frame Sensor Data
     *
     * Reports a snapshot of the value of the OBD2 sensors available at the time that a fault
     * occurred and was detected.
     *
     * A configArray must be provided with the same meaning as defined for OBD2_LIVE_FRAME.
     *
     * The values of this property are to be interpreted in a similar fashion as those for
     * OBD2_LIVE_FRAME, with the exception that the stringValue field may contain a non-empty
     * diagnostic troubleshooting code (DTC).
     *
     * A IVehicle#get request of this property must provide a value for int64Values[0].
     * This will be interpreted as the timestamp of the freeze frame to retrieve. A list of
     * timestamps can be obtained by a IVehicle#get of OBD2_FREEZE_FRAME_INFO.
     *
     * Should no freeze frame be available at the given timestamp, a response of NOT_AVAILABLE
     * must be returned by the implementation. Because vehicles may have limited storage for
     * freeze frames, it is possible for a frame request to respond with NOT_AVAILABLE even if
     * the associated timestamp has been recently obtained via OBD2_FREEZE_FRAME_INFO.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    OBD2_FREEZE_FRAME = 0x0D01 + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * OBD2 Freeze Frame Information
     *
     * This property describes the current freeze frames stored in vehicle
     * memory and available for retrieval via OBD2_FREEZE_FRAME.
     *
     * The values are to be interpreted as follows:
     * each element of int64Values must be the timestamp at which a a fault code
     * has been detected and the corresponding freeze frame stored, and each
     * such element can be used as the key to OBD2_FREEZE_FRAME to retrieve
     * the corresponding freeze frame.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    OBD2_FREEZE_FRAME_INFO = 0x0D02 + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * OBD2 Freeze Frame Clear
     *
     * This property allows deletion of any of the freeze frames stored in
     * vehicle memory, as described by OBD2_FREEZE_FRAME_INFO.
     *
     * The configArray is set as follows:
     *  configArray[0] = 1 if the implementation is able to clear individual freeze frames
     *                   by timestamp, 0 otherwise
     *
     * IVehicle#set of this property is to be interpreted as follows:
     *   if int64Values contains no elements, then all frames stored must be cleared;
     *   if int64Values contains one or more elements, then frames at the timestamps
     *   stored in int64Values must be cleared, and the others not cleared. Should the
     *   vehicle not support selective clearing of freeze frames, this latter mode must
     *   return NOT_AVAILABLE.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     */
    OBD2_FREEZE_FRAME_CLEAR = 0x0D03 + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * Headlights State
     *
     * Return the current state of headlights.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightState
     */
    HEADLIGHTS_STATE = 0x0E00 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * High beam lights state
     *
     * Return the current state of high beam lights.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightState
     */
    HIGH_BEAM_LIGHTS_STATE = 0x0E01 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Fog light state
     *
     * Return the current state of fog lights.
     *
     * If the car has both front and rear fog lights:
     *   If front and rear fog lights can only be controlled together: FOG_LIGHTS_STATE must be
     *   implemented. FRONT_FOG_LIGHTS_STATE and REAR_FOG_LIGHTS_STATE must not be implemented.
     *
     *   If the front and rear fog lights can only be controlled independently: FOG_LIGHTS_STATE
     *   must not be implemented. FRONT_FOG_LIGHTS_STATE and REAR_FOG_LIGHTS_STATE must be
     *   implemented.
     *
     * If the car has only front fog lights:
     * Only one of FOG_LIGHTS_STATE or FRONT_FOG_LIGHTS_STATE must be implemented and not both.
     * REAR_FOG_LIGHTS_STATE must not be implemented.
     *
     * If the car has only rear fog lights:
     * Only one of FOG_LIGHTS_STATE or REAR_FOG_LIGHTS_STATE must be implemented and not both.
     * FRONT_FOG_LIGHTS_STATE must not be implemented.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightState
     */
    FOG_LIGHTS_STATE = 0x0E02 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Hazard light status
     *
     * Return the current status of hazard lights.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightState
     */
    HAZARD_LIGHTS_STATE = 0x0E03 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Headlight switch
     *
     * The setting that the user wants.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightSwitch
     */
    HEADLIGHTS_SWITCH = 0x0E10 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * High beam light switch
     *
     * The setting that the user wants.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightSwitch
     */
    HIGH_BEAM_LIGHTS_SWITCH = 0x0E11 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Fog light switch
     *
     * The setting that the user wants.
     *
     * If the car has both front and rear fog lights:
     *   If front and rear fog lights can only be controlled together: FOG_LIGHTS_SWITCH must be
     *   implemented. FRONT_FOG_LIGHTS_SWITCH and REAR_FOG_LIGHTS_SWITCH must not be implemented.
     *
     *   If the front and rear fog lights can only be controlled independently: FOG_LIGHTS_SWITCH
     *   must not be implemented. FRONT_FOG_LIGHTS_SWITCH and REAR_FOG_LIGHTS_SWITCH must be
     *   implemented.
     *
     * If the car has only front fog lights:
     * Only one of FOG_LIGHTS_SWITCH or FRONT_FOG_LIGHTS_SWITCH must be implemented and not both.
     * REAR_FOG_LIGHTS_SWITCH must not be implemented.
     *
     * If the car has only rear fog lights:
     * Only one of FOG_LIGHTS_SWITCH or REAR_FOG_LIGHTS_SWITCH must be implemented and not both.
     * FRONT_FOG_LIGHTS_SWITCH must not be implemented.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightSwitch
     */
    FOG_LIGHTS_SWITCH = 0x0E12 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Hazard light switch
     *
     * The setting that the user wants.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightSwitch
     */
    HAZARD_LIGHTS_SWITCH = 0x0E13 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Cabin lights
     *
     * Return current status of cabin lights.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightState
     */
    CABIN_LIGHTS_STATE = 0x0F01 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Cabin lights switch
     *
     * The position of the physical switch which controls the cabin lights.
     * This might be different than the CABIN_LIGHTS_STATE if the lights are on because a door
     * is open or because of a voice command.
     * For example, while the switch is in the "off" or "automatic" position.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightSwitch
     */
    CABIN_LIGHTS_SWITCH = 0x0F02 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Reading lights
     *
     * Return current status of reading lights.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightState
     */
    READING_LIGHTS_STATE = 0x0F03 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Reading lights switch
     *
     * The position of the physical switch which controls the reading lights.
     * This might be different than the READING_LIGHTS_STATE if the lights are on because a door
     * is open or because of a voice command.
     * For example, while the switch is in the "off" or "automatic" position.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightSwitch
     */
    READING_LIGHTS_SWITCH = 0x0F04 + 0x10000000 + 0x05000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:SEAT,VehiclePropertyType:INT32
    /**
     * Steering wheel lights state
     *
     * Represents the current state of the steering wheel lights. This is different from
     * STEERING_WHEEL_LIGHTS_SWITCH which represents the position of the switch controlling
     * the lights. Therefore, STEERING_WHEEL_LIGHTS_STATE may not match the value of
     * STEERING_WHEEL_LIGHTS_SWITCH (e.g. STEERING_WHEEL_LIGHTS_SWITCH=AUTOMATIC and
     * STEERING_WHEEL_LIGHTS_STATE=ON).
     *
     * This property should only be implemented if STEERING_WHEEL_LIGHTS_STATE's value may be
     * different from that of CABIN_LIGHTS_STATE.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues must be defined unless
     * all enum values of VehicleLightState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightState
     */
    STEERING_WHEEL_LIGHTS_STATE =
            0x0F0C + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,
    /**
     * Steering wheel lights switch
     *
     * Represents the position of the switch controlling the steering wheel lights. This is
     * different from STEERING_WHEEL_LIGHTS_STATE which represents the current state of the steering
     * wheel lights. Therefore, STEERING_WHEEL_LIGHTS_SWITCH may not match the value of
     * STEERING_WHEEL_LIGHTS_STATE (e.g. STEERING_WHEEL_LIGHTS_SWITCH=AUTOMATIC and
     * STEERING_WHEEL_LIGHTS_STATE=ON).
     *
     * This property should only be implemented if STEERING_WHEEL_LIGHTS_SWITCH's value may be
     * different from that of CABIN_LIGHTS_SWITCH.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues must be defined unless
     * all enum values of VehicleLightSwitch are supported.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightSwitch
     */
    STEERING_WHEEL_LIGHTS_SWITCH =
            0x0F0D + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,
    /**
     * Support customize permissions for vendor properties
     *
     * Implement this property if vehicle hal support customize vendor permissions feature.
     * VehiclePropConfig.configArray is used to indicate vendor properties and permissions
     * which selected for this vendor property. The permission must be one of enum in
     * VehicleVendorPermission.
     * The configArray is set as follows:
     *      configArray[n] = propId : property ID for the vendor property
     *      configArray[n+1] = one of enums in VehicleVendorPermission. It indicates the permission
     *      for reading value of the property.
     *      configArray[n+2] = one of enums in VehicleVendorPermission. It indicates the permission
     *      for writing value of the property.
     *
     * For example:
     * configArray = {
     *      vendor_prop_1, PERMISSION_VENDOR_SEAT_READ, PERMISSION_VENDOR_SEAT_WRITE,
     *      vendor_prop_2, PERMISSION_VENDOR_INFO, PERMISSION_NOT_ACCESSIBLE,
     * }
     * If vendor properties are not in this array, they will have the default vendor permission.
     * If vendor chose PERMISSION_NOT_ACCESSIBLE, android will not have access to the property. In
     * the example, Android can not write value for vendor_prop_2.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    SUPPORT_CUSTOMIZE_VENDOR_PERMISSION = 0x0F05 + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN
    /**
     * Allow disabling optional featurs from vhal.
     *
     * This property reports optional features that should be disabled.
     * All allowed optional features for the system is declared in Car service overlay,
     * config_allowed_optional_car_features.
     * This property allows disabling features defined in the overlay. Without this property,
     * all the features declared in the overlay will be enabled.
     *
     * Value read should include all features disabled with ',' separation.
     * ex) "com.android.car.user.CarUserNoticeService,storage_monitoring"
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    DISABLED_OPTIONAL_FEATURES = 0x0F06 + 0x10000000 + 0x01000000
            + 0x00100000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:STRING
    /**
     * Defines the initial Android user to be used during initialization.
     *
     * This property is called by the Android system when it initializes and it lets the HAL
     * define which Android user should be started.
     *
     * This request is made by setting a VehiclePropValue (defined by InitialUserInfoRequest),
     * and the HAL must respond with a property change event (defined by InitialUserInfoResponse).
     * If the HAL doesn't respond after some time (defined by the Android system), the Android
     * system will proceed as if HAL returned a response of action
     * InitialUserInfoResponseAction:DEFAULT.
     *
     * For example, on first boot, the request could be:
     *
     * int32[0]: 42  // request id (arbitrary number set by Android system)
     * int32[1]: 1   // InitialUserInfoRequestType::FIRST_BOOT
     * int32[2]: 0   // id of current user (usersInfo.currentUser.userId)
     * int32[3]: 1   // flag of current user (usersInfo.currentUser.flags = SYSTEM)
     * int32[4]: 1   // number of existing users (usersInfo.numberUsers);
     * int32[5]: 0   // user #0  (usersInfo.existingUsers[0].userId)
     * int32[6]: 1   // flags of user #0  (usersInfo.existingUsers[0].flags)
     *
     * And if the HAL want to respond with the creation of an admin user called "Owner", the
     * response would be:
     *
     * int32[0]: 42      // must match the request id from the request
     * int32[1]:  2      // action = InitialUserInfoResponseAction::CREATE
     * int32[2]: -10000  // userToSwitchOrCreate.userId (not used as user will be created)
     * int32[3]:  8      // userToSwitchOrCreate.flags = ADMIN
     * string: "||Owner" // userLocales + separator + userNameToCreate
     *
     * Notice the string value represents multiple values, separated by ||. The first value is the
     * (optional) system locales for the user to be created (in this case, it's empty, meaning it
     * will use Android's default value), while the second value is the (also optional) name of the
     * to user to be created (when the type of response is InitialUserInfoResponseAction:CREATE).
     * For example, to create the same "Owner" user with "en-US" and "pt-BR" locales, the string
     * value of the response would be "en-US,pt-BR||Owner". As such, neither the locale nor the
     * name can have || on it, although a single | is fine.
     *
     * NOTE: if the HAL doesn't support user management, then it should not define this property,
     * which in turn would disable the other user-related properties (for example, the Android
     * system would never issue them and user-related requests from the HAL layer would be ignored
     * by the Android System). But if it supports user management, then it must support all core
     * user-related properties (INITIAL_USER_INFO, SWITCH_USER, CREATE_USER, and REMOVE_USER).
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    INITIAL_USER_INFO = 0x0F07 + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * Defines a request to switch the foreground Android user.
     *
     * This property is used primarily by the Android System to inform the HAL that the
     * current foreground Android user is switching, but it could also be used by the HAL to request
     * the Android system to switch users - the
     *
     * When the request is made by Android, it sets a VehiclePropValue and the HAL must responde
     * with a property change event; when the HAL is making the request, it must also do it through
     * a property change event (the main difference is that the request id will be positive in the
     * former case, and negative in the latter; the SwitchUserMessageType will also be different).
     *
     * The format of both request is defined by SwitchUserRequest and the format of the response
     * (when needed) is defined by SwitchUserResponse. How the HAL (or Android System) should
     * proceed depends on the message type (which is defined by the SwitchUserMessageType
     * parameter), as defined below.
     *
     * 1.LEGACY_ANDROID_SWITCH
     * -----------------------
     *
     * Called by the Android System to indicate the Android user is about to change, when the change
     * request was made in a way that is not integrated with the HAL (for example, through
     * adb shell am switch-user).
     *
     * The HAL can switch its internal user once it receives this request, but it doesn't need to
     * reply back to the Android System. If its internal user cannot be changed for some reason,
     * then it must wait for the SWITCH_USER(type=ANDROID_POST_SWITCH) call to recover
     * (for example, it could issue a SWITCH_USER(type=VEHICLE_REQUEST) to switch back to
     * the previous user), but ideally it should never fail (as switching back could result in a
     * confusing experience for the end user).
     *
     * For example, if the system have users (0, 10, 11) and it's switching from 0 to 11 (where none
     * of them have any special flag), the request would be:
     *
     * int32[0]:  42  // request id
     * int32[1]:  1   // SwitchUserMessageType::LEGACY_ANDROID_SWITCH
     * int32[2]:  11  // target user id
     * int32[3]:  0   // target user flags (none)
     * int32[4]:  10  // current user
     * int32[5]:  0   // current user flags (none)
     * int32[6]:  3   // number of users
     * int32[7]:  0   // user #0 (Android user id 0)
     * int32[8]:  0   // flags of user #0 (none)
     * int32[9]:  10  // user #1 (Android user id 10)
     * int32[10]: 0   // flags of user #1 (none)
     * int32[11]: 11  // user #2 (Android user id 11)
     * int32[12]: 0   // flags of user #2 (none)
     *
     * 2.ANDROID_SWITCH
     * ----------------
     * Called by the Android System to indicate the Android user is about to change, but Android
     * will wait for the HAL's response (up to some time) before proceeding.
     *
     * The HAL must switch its internal user once it receives this request, then respond back to
     * Android with a SWITCH_USER(type=VEHICLE_RESPONSE) indicating whether its internal
     * user was switched or not (through the SwitchUserStatus enum).
     *
     * For example, if Android has users (0, 10, 11) and it's switching from 10 to 11 (where
     * none of them have any special flag), the request would be:
     *
     * int32[0]:  42  // request id
     * int32[1]:  2   // SwitchUserMessageType::ANDROID_SWITCH
     * int32[2]:  11  // target user id
     * int32[3]:  0   // target user flags (none)
     * int32[4]:  10  // current user
     * int32[5]:  0   // current user flags (none)
     * int32[6]:  3   // number of users
     * int32[7]:  0   // 1st user (user 0)
     * int32[8]:  1   // 1st user flags (SYSTEM)
     * int32[9]:  10  // 2nd user (user 10)
     * int32[10]: 0   // 2nd user flags (none)
     * int32[11]: 11  // 3rd user (user 11)
     * int32[12]: 0   // 3rd user flags (none)
     *
     * If the request succeeded, the HAL must update the property with:
     *
     * int32[0]: 42  // request id
     * int32[1]: 3   // messageType = SwitchUserMessageType::VEHICLE_RESPONSE
     * int32[2]: 1   // status = SwitchUserStatus::SUCCESS
     *
     * But if it failed, the response would be something like:
     *
     * int32[0]: 42   // request id
     * int32[1]: 3    // messageType = SwitchUserMessageType::VEHICLE_RESPONSE
     * int32[2]: 2    // status = SwitchUserStatus::FAILURE
     * string: "108-D'OH!" // OEM-specific error message
     *
     * 3.VEHICLE_RESPONSE
     * ------------------
     * Called by the HAL to indicate whether a request of type ANDROID_SWITCH should proceed or
     * abort - see the ANDROID_SWITCH section above for more info.
     *
     * 4.VEHICLE_REQUEST
     * ------------------
     * Called by the HAL to request that the current foreground Android user is switched.
     *
     * This is useful in situations where Android started as one user, but the vehicle identified
     * the driver as another user. For example, user A unlocked the car using the key fob of user B;
     * the INITIAL_USER_INFO request returned user B, but then a face recognition subsubsystem
     * identified the user as A.
     *
     * The HAL makes this request by a property change event (passing a negative request id), and
     * the Android system will response by issue an ANDROID_POST_SWITCH call which the same
     * request id.
     *
     * For example, if the current foreground Android user is 10 and the HAL asked it to switch to
     * 11, the request would be:
     *
     * int32[0]: -108  // request id
     * int32[1]: 4     // messageType = SwitchUserMessageType::VEHICLE_REQUEST
     * int32[2]: 11    // Android user id
     *
     * If the request succeeded and Android has 3 users (0, 10, 11), the response would be:
     *
     * int32[0]: -108 // request id
     * int32[1]:  5   // messageType = SwitchUserMessageType::ANDROID_POST_SWITCH
     * int32[2]:  11  // target user id
     * int32[3]:  0   // target user id flags (none)
     * int32[4]:  11  // current user
     * int32[5]:  0   // current user flags (none)
     * int32[6]:  3   // number of users
     * int32[7]:  0   // 1st user (user 0)
     * int32[8]:  0   // 1st user flags (none)
     * int32[9]:  10  // 2nd user (user 10)
     * int32[10]: 4   // 2nd user flags (none)
     * int32[11]: 11  // 3rd user (user 11)
     * int32[12]: 3   // 3rd user flags (none)
     *
     * Notice that both the current and target user ids are the same - if the request failed, then
     * they would be different (i.e, target user would be 11, but current user would still be 10).
     *
     * 5.ANDROID_POST_SWITCH
     * ---------------------
     * Called by the Android System after a request to switch a user was made.
     *
     * This property is called after switch requests of any type (i.e., LEGACY_ANDROID_SWITCH,
     * ANDROID_SWITCH, or VEHICLE_REQUEST) and can be used to determine if the request succeeded or
     * failed:
     *
     * 1. When it succeeded, it's called when the Android user is in the unlocked state and the
     *    value of the current and target users ids in the response are the same. This would be
     *    equivalent to receiving an Intent.ACTION_USER_UNLOCKED in an Android app.
     * 2. When it failed it's called right away and the value of the current and target users ids
     *    in the response are different (as the current user didn't change to the target).
     * 3. If a new switch request is made before the HAL responded to the previous one or before
     *    the user was unlocked, then the ANDROID_POST_SWITCH request is not made. For example,
     *    the driver could accidentally switch to the wrong user which has lock credentials, then
     *    switch to the right one before entering the credentials.
     *
     * The HAL can update its internal state once it receives this request, but it doesn't need to
     * reply back to the Android System.
     *
     * Request: the first N values as defined by INITIAL_USER_INFO (where the request-specific
     * value at index 1 is SwitchUserMessageType::ANDROID_POST_SWITCH), then 2 more values for the
     * target user id (i.e., the Android user id that was requested to be switched to) and its flags
     * (as defined by  UserFlags).
     *
     * Response: none.
     *
     * Example: see VEHICLE_REQUEST section above.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    SWITCH_USER = 0x0F08 + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * Called by the Android System after an Android user was created.
     *
     * The HAL can use this property to create its equivalent user.
     *
     * This is an async request: Android makes the request by setting a VehiclePropValue, and HAL
     * must respond with a property change indicating whether the request succeeded or failed. If
     * it failed, the Android system will remove the user.
     *
     * The format of the request is defined by CreateUserRequest and the format of the response by
     * CreateUserResponse.
     *
     * For example, if system had 2 users (0 and 10) and a 3rd one (which is an ephemeral guest) was
     * created, the request would be:
     *
     * int32[0]: 42  // request id
     * int32[1]: 11  // Android id of the created user
     * int32[2]: 6   // Android flags (ephemeral guest) of the created user
     * int32[3]: 10  // current user
     * int32[4]: 0   // current user flags (none)
     * int32[5]: 3   // number of users
     * int32[6]: 0   // 1st user (user 0)
     * int32[7]: 0   // 1st user flags (none)
     * int32[8]: 10  // 2nd user (user 10)
     * int32[9]: 0   // 2nd user flags (none)
     * int32[19]: 11 // 3rd user (user 11)
     * int32[11]: 6  // 3rd user flags (ephemeral guest)
     * string: "ElGuesto" // name of the new user
     *
     * Then if the request succeeded, the HAL would return:
     *
     * int32[0]: 42  // request id
     * int32[1]: 1   // CreateUserStatus::SUCCESS
     *
     * But if it failed:
     *
     * int32[0]: 42  // request id
     * int32[1]: 2   // CreateUserStatus::FAILURE
     * string: "D'OH!" // The meaning is a blackbox - it's passed to the caller (like Settings UI),
     *                 // which in turn can take the proper action.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    CREATE_USER = 0x0F09 + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * Called by the Android System after an Android user was removed.
     *
     * The HAL can use this property to remove its equivalent user.
     *
     * This is write-only call - the Android System is not expecting a reply from the HAL. Hence,
     * this request should not fail - if the equivalent HAL user cannot be removed, then HAL should
     * mark it as inactive or recover in some other way.
     *
     * The request is made by setting the VehiclePropValue with the contents defined by
     * RemoveUserRequest.
     *
     * For example, if system had 3 users (0, 10, and 11) and user 11 was removed, the request
     * would be:
     *
     * int32[0]: 42  // request id
     * int32[1]: 11  // (Android user id of the removed user)
     * int32[2]: 0   // (Android user flags of the removed user)
     * int32[3]: 10  // current user
     * int32[4]: 0   // current user flags (none)
     * int32[5]: 2   // number of users
     * int32[6]: 0   // 1st user (user 0)
     * int32[7]: 0   // 1st user flags (none)
     * int32[8]: 10  // 2nd user (user 10)
     * int32[9]: 0   // 2nd user flags (none)
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.WRITE
     */
    REMOVE_USER = 0x0F0A + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * Property used to associate (or query the association) the current user with vehicle-specific
     * identification mechanisms (such as key FOB).
     *
     * This is an optional user management property - the OEM could still support user management
     * without defining it. In fact, this property could be used without supporting the core
     * user-related functions described on INITIAL_USER_INFO.
     *
     * To query the association, the Android system gets the property, passing a VehiclePropValue
     * containing the types of associations are being queried, as defined by
     * UserIdentificationGetRequest. The HAL must return right away, returning a VehiclePropValue
     * with a UserIdentificationResponse. Notice that user identification should have already
     * happened while system is booting up and the VHAL implementation should only return the
     * already identified association (like the key FOB used to unlock the car), instead of starting
     * a new association from the get call.
     *
     * To associate types, the Android system sets the property, passing a VehiclePropValue
     * containing the types and values of associations being set, as defined by the
     * UserIdentificationSetRequest. The HAL will then use a property change event (whose
     * VehiclePropValue is defined by UserIdentificationResponse) indicating the current status of
     * the types after the request.
     *
     * For example, to query if the current user (10) is associated with the FOB that unlocked the
     * car and a custom mechanism provided by the OEM, the request would be:
     *
     * int32[0]: 42  // request id
     * int32[1]: 10  (Android user id)
     * int32[2]: 0   (Android user flags)
     * int32[3]: 2   (number of types queried)
     * int32[4]: 1   (1st type queried, UserIdentificationAssociationType::KEY_FOB)
     * int32[5]: 101 (2nd type queried, UserIdentificationAssociationType::CUSTOM_1)
     *
     * If the user is associated with the FOB but not with the custom mechanism, the response would
     * be:
     *
     * int32[0]: 42  // request id
     * int32[1]: 2   (number of associations in the response)
     * int32[2]: 1   (1st type: UserIdentificationAssociationType::KEY_FOB)
     * int32[3]: 2   (1st value: UserIdentificationAssociationValue::ASSOCIATED_CURRENT_USER)
     * int32[4]: 101 (2st type: UserIdentificationAssociationType::CUSTOM_1)
     * int32[5]: 4   (2nd value: UserIdentificationAssociationValue::NOT_ASSOCIATED_ANY_USER)
     *
     * Then to associate the user with the custom mechanism, a set request would be made:
     *
     * int32[0]: 43  // request id
     * int32[1]: 10  (Android user id)
     * int32[2]: 0   (Android user flags)
     * int32[3]: 1   (number of associations being set)
     * int32[4]: 101 (1st type: UserIdentificationAssociationType::CUSTOM_1)
     * int32[5]: 1   (1st value: UserIdentificationAssociationSetValue::ASSOCIATE_CURRENT_USER)
     *
     * If the request succeeded, the response would be simply:
     *
     * int32[0]: 43  // request id
     * int32[1]: 1   (number of associations in the response)
     * int32[2]: 101 (1st type: UserIdentificationAssociationType::CUSTOM_1)
     * int32[3]: 1   (1st value: UserIdentificationAssociationValue::ASSOCIATED_CURRENT_USER)
     *
     * Notice that the set request adds associations, but doesn't remove the existing ones. In the
     * example above, the end state would be 2 associations (FOB and CUSTOM_1). If we wanted to
     * associate the user with just CUSTOM_1 but not FOB, then the request should have been:
     *
     * int32[0]: 43  // request id
     * int32[1]: 10  (Android user id)
     * int32[2]: 2   (number of types set)
     * int32[3]: 1   (1st type: UserIdentificationAssociationType::KEY_FOB)
     * int32[4]: 2   (1st value: UserIdentificationAssociationValue::DISASSOCIATE_CURRENT_USER)
     * int32[5]: 101 (2nd type: UserIdentificationAssociationType::CUSTOM_1)
     * int32[6]: 1   (2nd value: UserIdentificationAssociationValue::ASSOCIATE_CURRENT_USER)
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    USER_IDENTIFICATION_ASSOCIATION = 0x0F0B + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * Enable/request an EVS service.
     *
     * The property provides a generalized way to trigger EVS services.  VHAL
     * should use this property to request Android to start or stop EVS service.
     *
     *  int32Values[0] = a type of the EVS service. The value must be one of enums in
     *                   EvsServiceType.
     *  int32Values[1] = the state of the EVS service. The value must be one of enums in
     *                   EvsServiceState.
     *
     * For example, to enable rear view EVS service, android side can set the property value as
     * [EvsServiceType::REAR_VIEW, EvsServiceState::ON].
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    EVS_SERVICE_REQUEST = 0x0F10 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Defines a request to apply power policy.
     *
     * VHAL sets this property to change car power policy. Car power policy service subscribes to
     * this property and actually changes the power policy.
     * The request is made by setting the VehiclePropValue with the ID of a power policy which is
     * defined at /vendor/etc/automotive/power_policy.xml.
     * If the given ID is not defined, car power policy service ignores the request
     * and the current power policy is maintained.
     *
     *   string: "sample_policy_id" // power policy ID
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    POWER_POLICY_REQ = 0x0F21 + 0x10000000 + 0x01000000
            + 0x00100000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:STRING
    /**
     * Defines a request to set the power polic group used to decide a default power policy per
     * power status transition.
     *
     * VHAL sets this property with the ID of a power policy group in order to set the default power
     * policy applied at power status transition. Power policy groups are defined at
     * /vendor/etc/power_policy.xml. If the given ID is not defined, car power policy service
     * ignores the request.
     * Car power policy service subscribes to this property and sets the power policy group.
     * The actual application of power policy takes place when the system power status changes and
     * there is a valid mapped power policy for the new power status.
     *
     *   string: "sample_policy_group_id" // power policy group ID
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    POWER_POLICY_GROUP_REQ = 0x0F22 + 0x10000000 + 0x01000000
            + 0x00100000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:STRING
    /**
     * Notifies the current power policy to VHAL layer.
     *
     * Car power policy service sets this property when the current power policy is changed.
     *
     *   string: "sample_policy_id" // power policy ID
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     */
    CURRENT_POWER_POLICY = 0x0F23 + 0x10000000 + 0x01000000
            + 0x00100000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:STRING
    /**
     * Defines an event that car watchdog updates to tell it's alive.
     *
     * Car watchdog sets this property to system uptime in milliseconds at every 3 second.
     * During the boot, the update may take longer time.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     */
    WATCHDOG_ALIVE = 0xF31 + 0x10000000 + 0x01000000
            + 0x00500000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT64
    /**
     * Defines a process terminated by car watchdog and the reason of termination.
     *
     *   int32Values[0]: 1         // ProcessTerminationReason showing why a process is terminated.
     *   string: "/system/bin/log" // Process execution command.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     */
    WATCHDOG_TERMINATED_PROCESS = 0x0F32 + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * Defines an event that VHAL signals to car watchdog as a heartbeat.
     *
     * If VHAL supports this property, VHAL should write system uptime to this property at every 3
     * second. Car watchdog subscribes to this property and checks if the property is updated at
     * every 3 second. With the buffer time of 3 second, car watchdog waits for a heart beat to be
     * signaled up to 6 seconds from the last heart beat. If it isn’t, car watchdog considers
     * VHAL unhealthy and terminates it.
     * If this property is not supported by VHAL, car watchdog doesn't check VHAL health status.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    VHAL_HEARTBEAT = 0x0F33 + 0x10000000 + 0x01000000
            + 0x00500000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT64
    /**
     * Starts the ClusterUI in cluster display.
     *
     * int32: the type of ClusterUI to show
     *    0 indicates ClusterHome, that is a home screen of cluster display, and provides
     *        the default UI and a kind of launcher functionality for cluster display.
     *    the other values are followed by OEM's definition.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    CLUSTER_SWITCH_UI = 0x0F34 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Changes the state of the cluster display.
     *
     * Bounds: the area to render the cluster Activity.
     * Inset: the area which Activity should avoid from placing any important
     *     information.
     *
     * int32[0]: on/off: 0 - off, 1 - on, -1 - don't care
     * int32[1]: Bounds - left: positive number - left position in pixels
                                -1 - don't care (should set all Bounds fields)
     * int32[2]: Bounds - top:    same format with 'left'
     * int32[3]: Bounds - right:  same format with 'left'
     * int32[4]: Bounds - bottom: same format with 'left'
     * int32[5]: Inset - left: positive number - actual left inset value in pixels
                               -1 - don't care (should set "don't care" all Inset fields)
     * int32[6]: Inset - top:    same format with 'left'
     * int32[7]: Inset - right:  same format with 'left'
     * int32[8]: Inset - bottom: same format with 'left'
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     */
    CLUSTER_DISPLAY_STATE = 0x0F35 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC
    /**
     * Reports the current display state and ClusterUI state.
     *
     * ClusterHome will send this message when it handles CLUSTER_SWITCH_UI, CLUSTER_DISPLAY_STATE.
     *
     * In addition, ClusterHome should send this message when it starts for the first time.
     * When ClusterOS receives this message and if the internal expectation is different with the
     * received message, then it should send CLUSTER_SWITCH_UI, CLUSTER_DISPLAY_STATE again to
     * match the state.
     *
     * int32[0]: on/off: 0 - off, 1 - on
     * int32[1]: Bounds - left
     * int32[2]: Bounds - top
     * int32[3]: Bounds - right
     * int32[4]: Bounds - bottom
     * int32[5]: Inset - left
     * int32[6]: Inset - top
     * int32[7]: Inset - right
     * int32[8]: Inset - bottom
     * int32[9]: the type of ClusterUI in the fullscreen or main screen.
     *    0 indicates ClusterHome.
     *    the other values are followed by OEM's definition.
     * int32[10]: the type of ClusterUI in sub screen if the currently two UIs are shown.
     *    -1 indicates the area isn't used any more.
     * bytes: the array to represent the availability of ClusterUI.
     *     0 indicates non-available and 1 indicates available.
     *     For example, let's assume a car supports 3 OEM defined ClusterUI like HOME, MAPS, CALL,
     *     and it only supports CALL UI only when the cellular network is available. Then, if the
     *     nework is avaibale, it'll send [1 1 1], and if it's out of network, it'll send [1 1 0].
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     */
    CLUSTER_REPORT_STATE = 0x0F36 + 0x10000000 + 0x01000000
            + 0x00e00000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:MIXED
    /**
     * Requests to change the cluster display state to show some ClusterUI.
     *
     * When the current display state is off and ClusterHome sends this message to ClusterOS to
     * request to turn the display on to show some specific ClusterUI.
     * ClusterOS should response this with CLUSTER_DISPLAY_STATE.
     *
     * int32: the type of ClusterUI to show
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     */
    CLUSTER_REQUEST_DISPLAY = 0x0F37 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Informs the current navigation state.
     *
     * bytes: the serialized message of NavigationStateProto.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     */
    CLUSTER_NAVIGATION_STATE = 0x0F38 + 0x10000000 + 0x01000000
            + 0x00700000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BYTES
    /**
     * Electronic Toll Collection card type.
     *
     * This property indicates the type of ETC card in this vehicle.
     * If the head unit is aware of an ETC card attached to the vehicle, this property should
     * return the type of card attached; otherwise, this property should be UNAVAILABLE.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum ElectronicTollCollectionCardType
     */
    ELECTRONIC_TOLL_COLLECTION_CARD_TYPE = 0x0F39 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Electronic Toll Collection card status.
     *
     * This property indicates the status of ETC card in this vehicle.
     * If the head unit is aware of an ETC card attached to the vehicle,
     * ELECTRONIC_TOLL_COLLECTION_CARD_TYPE gives that status of the card; otherwise,
     * this property should be UNAVAILABLE.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum ElectronicTollCollectionCardStatus
     */
    ELECTRONIC_TOLL_COLLECTION_CARD_STATUS = 0x0F3A + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32
    /**
     * Front fog lights state
     *
     * Return the current state of the front fog lights.
     * Only one of FOG_LIGHTS_STATE or FRONT_FOG_LIGHTS_STATE must be implemented. Please refer to
     * the documentation on FOG_LIGHTS_STATE for more information.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightState
     */
    FRONT_FOG_LIGHTS_STATE = 0x0F3B + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * Front fog lights switch
     *
     * The setting that the user wants.
     * Only one of FOG_LIGHTS_SWITCH or FRONT_FOG_LIGHTS_SWITCH must be implemented. Please refer to
     * the documentation on FOG_LIGHTS_SWITCH for more information.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightSwitch
     */
    FRONT_FOG_LIGHTS_SWITCH = 0x0F3C + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * Rear fog lights state
     *
     * Return the current state of the rear fog lights.
     * Only one of FOG_LIGHTS_STATE or REAR_FOG_LIGHTS_STATE must be implemented. Please refer to
     * the documentation on FOG_LIGHTS_STATE for more information.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightState
     */
    REAR_FOG_LIGHTS_STATE = 0x0F3D + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * Rear fog lights switch
     *
     * The setting that the user wants.
     * Only one of FOG_LIGHTS_SWITCH or REAR_FOG_LIGHTS_SWITCH must be implemented. Please refer to
     * the documentation on FOG_LIGHTS_SWITCH for more information.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleLightSwitch
     */
    REAR_FOG_LIGHTS_SWITCH = 0x0F3E + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * Indicates the maximum current draw threshold for charging set by the user
     *
     * configArray[0] is used to specify the max current draw allowed by
     * the vehicle in Amperes.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:AMPERE
     */
    EV_CHARGE_CURRENT_DRAW_LIMIT = 0x0F3F + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT

    /**
     * Indicates the maximum charge percent threshold set by the user
     *
     * Returns a float value from 0 to 100.
     *
     * configArray is used to specify the valid values.
     *   For example, if the vehicle supports the following charge percent limit values:
     *     [20, 40, 60, 80, 100]
     *   then the configArray should be {20, 40, 60, 80, 100}
     * If the configArray is empty then all values from 0 to 100 must be valid.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    EV_CHARGE_PERCENT_LIMIT = 0x0F40 + 0x10000000 + 0x01000000
            + 0x00600000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:FLOAT

    /**
     * Charging state of the car
     *
     * Returns the current charging state of the car.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum EvChargeState
     */
    EV_CHARGE_STATE = 0x0F41 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * Start or stop charging the EV battery
     *
     * The setting that the user wants. Setting this property to true starts the battery charging
     * and setting to false stops charging.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    EV_CHARGE_SWITCH = 0x0F42 + 0x10000000 + 0x01000000
            + 0x00200000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:BOOLEAN

    /**
     * Estimated charge time remaining in seconds
     *
     * Returns 0 if the vehicle is not charging.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:SECS
     */
    EV_CHARGE_TIME_REMAINING = 0x0F43 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * Regenerative braking or one-pedal drive state of the car
     *
     * Returns the current state associated with the regenerative braking
     * setting in the car
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum EvRegenerativeBrakingState
     */
    EV_REGENERATIVE_BRAKING_STATE = 0x0F44 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * Indicates if there is a trailer present or not.
     *
     * Returns the trailer state of the car.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum TrailerState
     */
    TRAILER_PRESENT = 0x0F45 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * Vehicle’s curb weight
     *
     * Returns the vehicle's curb weight in kilograms. Curb weight is
     * the total weight of the vehicle with standard equipment and all
     * necessary operating consumables such as motor oil,transmission oil,
     * brake fluid, coolant, air conditioning refrigerant, and weight of
     * fuel at nominal tank capacity, while not loaded with either passengers
     * or cargo.
     *
     * configArray[0] is used to specify the vehicle’s gross weight in kilograms.
     * The vehicle’s gross weight is the maximum operating weight of the vehicle
     * as specified by the manufacturer including the vehicle's chassis, body, engine,
     * engine fluids, fuel, accessories, driver, passengers and cargo but excluding
     * that of any trailers.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:KILOGRAM
     */

    VEHICLE_CURB_WEIGHT = 0x0F46 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * EU's General security regulation compliance requirement.
     *
     * Returns whether general security regulation compliance is required, if
     * so, what type of requirement.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     * @data_enum GsrComplianceRequirementType
     */
    GENERAL_SAFETY_REGULATION_COMPLIANCE_REQUIREMENT = 0x0F47 + 0x10000000 + 0x01000000
            + 0x00400000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32

    /**
     * (Deprecated) List of all supported property IDs.
     *
     * A list of all supported property IDs (including this property). This property is required for
     * HIDL VHAL to work with large amount of vehicle prop configs where the getAllPropConfigs
     * payload exceeds the binder limitation. This issue is fixed in AIDL version using
     * LargeParcelable in getAllPropConfigs, so this property is deprecated.
     *
     * In HIDL VHAL implementation, if the amount of data returned in getAllPropConfigs exceeds the
     * binder limitation, vendor must support this property and return all the supported property
     * IDs. Car service will divide this list into smaller sub lists and use getPropConfigs([ids])
     * to query the sub lists. The results will be merged together in Car Service.
     *
     * The config array for this property must contain one int element which is the number of
     * configs per getPropConfigs request by Car Service. This number must be small enough so that
     * each getPropConfigs payload will not exceed binder limitation, however, a smaller number will
     * cause more requests, which increase overhead to fetch all the configs.
     *
     * @change_mode VehiclePropertyChangeMode.STATIC
     * @access VehiclePropertyAccess.READ
     */
    SUPPORTED_PROPERTY_IDS = 0x0F48 + 0x10000000 + 0x01000000
            + 0x00410000, // VehiclePropertyGroup:SYSTEM,VehicleArea:GLOBAL,VehiclePropertyType:INT32_VEC

    /**
     * Request the head unit to be shutdown.
     *
     * <p>This usually involves telling a separate system outside the head unit (e.g. a power
     * controller) to prepare shutting down the head unit.
     *
     * <p>This does not mean the head unit will shutdown immediately.
     *
     * <p>This means that another system will start sending a shutdown signal to the head unit,
     * which will cause VHAL to send SHUTDOWN_PREPARE message to Android. Android will then start
     * the shut down process by handling the message.
     *
     * <p>This property is only for issuing a request and only supports writing. Every time this
     * property value is set, the request to shutdown will be issued no matter what the current
     * property value is. The current property value is meaningless.
     *
     * <p>Since this property is write-only, subscribing is not allowed and no property change
     * event will be generated.
     *
     * <p>The value to set indicates the shutdown option, it must be one of
     * {@code VehicleApPowerStateShutdownParam}, e.g.,
     * VehicleApPowerStateShutdownParam.SLEEP_IMMEDIATELY. This shutdown option might not be honored
     * if the system doesn't support such option. In such case, an error will not be returned.
     *
     * <p>For configuration information, VehiclePropConfig.configArray must have bit flag combining
     * values in {@code VehicleApPowerStateConfigFlag} to indicate which shutdown options are
     * supported.
     *
     * <p>Returns error if failed to send the shutdown request to the other system.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     * @data_enum VehicleApPowerStateShutdownParam
     */
    SHUTDOWN_REQUEST =
            0x0F49 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Whether the vehicle is currently in use.
     *
     * <p>In-use means a human user is present and is intended to use the vehicle. This doesn't
     * necessarily means the human user is in the vehicle. For example, if the human user unlocks
     * the vehicle remotely, the vehicle is considered in use.
     *
     * <p>If this property is supported:
     *
     * <p>Each time user powers on the vehicle or the system detects the user is present,
     * VEHICLE_IN_USE must be set to true. Each time user powers off the vehicle or the system
     * detects the user is not present, VEHICLE_IN_USE must be set to false.
     *
     * <p>This property is different than AP_POWER_BOOTUP_REASON in the sense that
     * AP_POWER_BOOTUP_REASON is only set once during the system bootup. However, this property
     * might change multiple times during a system bootup cycle.
     *
     * <p>For example, a device is currently not in use. The system bootup to execute a remote task.
     * VEHICLE_IN_USE is false. While the remote task is executing, the user enters the vehicle and
     * powers on the vehicle. VEHICLE_IN_USE is set to true. After a driving session, user powers
     * off the vehicle, VEHICLE_IN_USE is set to false.
     *
     * <p>This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    VEHICLE_IN_USE =
            0x0F4A + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Sends the heartbeat signal to ClusterOS.
     *
     * int64[0]: epochTimeNs
     * int64[1]: the visibility of ClusterUI, 0 - invisible, 1 - visible
     * bytes: the app specific metadata, this can be empty when ClusterHomeService use the heartbeat
     *     to deliver the change of the visibility.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     */
    CLUSTER_HEARTBEAT =
            0x0F4B + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.MIXED,

    /**
     * Current state of vehicle autonomy.
     *
     * Defines the level of autonomy currently engaged in the vehicle from the J3016_202104 revision
     * of the SAE standard levels 0-5, with 0 representing no autonomy and 5 representing full
     * driving automation. These levels should be used in accordance with the standards defined in
     * https://www.sae.org/standards/content/j3016_202104/ and
     * https://www.sae.org/blog/sae-j3016-update
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of VehicleAutonomousState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum VehicleAutonomousState
     */
    VEHICLE_DRIVING_AUTOMATION_CURRENT_LEVEL =
            0x0F4C + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /***********************************************************************************************
     * Start of ADAS Properties
     *
     * Allocate IDs in range of 0x1000 (inclusive) to 0x1100 (exclusive) for ADAS properties
     **********************************************************************************************/

    /**
     * Enable or disable Automatic Emergency Braking (AEB).
     *
     * Set true to enable AEB and false to disable AEB. When AEB is enabled, the ADAS system in the
     * vehicle should be turned on and monitoring to avoid potential collisions.
     *
     * In general, AUTOMATIC_EMERGENCY_BRAKING_ENABLED should always return true or false. If the
     * feature is not available due to some temporary state, such as the vehicle speed being too
     * low, that information must be conveyed through the ErrorState values in the
     * AUTOMATIC_EMERGENCY_BRAKING_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    AUTOMATIC_EMERGENCY_BRAKING_ENABLED =
            0x1000 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Automatic Emergency Braking (AEB) state.
     *
     * Returns the current state of AEB. This property must always return a valid state defined in
     * AutomaticEmergencyBrakingState or ErrorState. It must not surface errors through StatusCode
     * and must use the supported error states instead.
     *
     * If AEB includes forward collision warnings before activating the brakes, those warnings must
     * be surfaced through the Forward Collision Warning (FCW) properties.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both AutomaticEmergencyBrakingState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum AutomaticEmergencyBrakingState
     * @data_enum ErrorState
     */
    AUTOMATIC_EMERGENCY_BRAKING_STATE =
            0x1001 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable Forward Collision Warning (FCW).
     *
     * Set true to enable FCW and false to disable FCW. When FCW is enabled, the ADAS system in the
     * vehicle should be turned on and monitoring for potential collisions.
     *
     * In general, FORWARD_COLLISION_WARNING_ENABLED should always return true or false. If the
     * feature is not available due to some temporary state, such as the vehicle speed being too
     * low, that information must be conveyed through the ErrorState values in the
     * FORWARD_COLLISION_WARNING_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    FORWARD_COLLISION_WARNING_ENABLED =
            0x1002 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Forward Collision Warning (FCW) state.
     *
     * Returns the current state of FCW. This property must always return a valid state defined in
     * ForwardCollisionWarningState or ErrorState. It must not surface errors through StatusCode
     * and must use the supported error states instead.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both ForwardCollisionWarningState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum ForwardCollisionWarningState
     * @data_enum ErrorState
     */
    FORWARD_COLLISION_WARNING_STATE =
            0x1003 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable and disable Blind Spot Warning (BSW).
     *
     * Set true to enable BSW and false to disable BSW. When BSW is enabled, the ADAS system in the
     * vehicle should be turned on and monitoring for objects in the vehicle’s blind spots.
     *
     * In general, BLIND_SPOT_WARNING_ENABLED should always return true or false. If the feature is
     * not available due to some temporary state, such as the vehicle speed being too low, that
     * information must be conveyed through the ErrorState values in the BLIND_SPOT_WARNING_STATE
     * property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    BLIND_SPOT_WARNING_ENABLED =
            0x1004 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Blind Spot Warning (BSW) state.
     *
     * Returns the current state of BSW. This property must always return a valid state defined in
     * BlindSpotWarningState or ErrorState. It must not surface errors through StatusCode
     * and must use the supported error states instead.
     *
     * For each supported area ID, the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both BlindSpotWarningState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum BlindSpotWarningState
     * @data_enum ErrorState
     */
    BLIND_SPOT_WARNING_STATE =
            0x1005 + VehiclePropertyGroup.SYSTEM + VehicleArea.MIRROR + VehiclePropertyType.INT32,

    /**
     * Enable or disable Lane Departure Warning (LDW).
     *
     * Set true to enable LDW and false to disable LDW. When LDW is enabled, the ADAS system in the
     * vehicle should be turned on and monitoring if the vehicle is approaching or crossing lane
     * lines, in which case a warning will be given.
     *
     * In general, LANE_DEPARTURE_WARNING_ENABLED should always return true or false. If the feature
     * is not available due to some temporary state, such as the vehicle speed being too low or too
     * high, that information must be conveyed through the ErrorState values in the
     * LANE_DEPARTURE_WARNING_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    LANE_DEPARTURE_WARNING_ENABLED =
            0x1006 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Lane Departure Warning (LDW) state.
     *
     * Returns the current state of LDW. This property must always return a valid state defined in
     * LaneDepartureWarningState or ErrorState. It must not surface errors through StatusCode
     * and must use the supported error states instead.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both LaneDepartureWarningState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum LaneDepartureWarningState
     * @data_enum ErrorState
     */
    LANE_DEPARTURE_WARNING_STATE =
            0x1007 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable Lane Keep Assist (LKA).
     *
     * Set true to enable LKA and false to disable LKA. When LKA is enabled, the ADAS system in the
     * vehicle should be turned on and monitoring if the driver unintentionally drifts toward or
     * over the lane marking. If an unintentional lane departure is detected, the system applies
     * steering control to return the vehicle into the current lane.
     *
     * This is different from Lane Centering Assist (LCA) which, when activated, applies continuous
     * steering control to keep the vehicle centered in the current lane.
     *
     * In general, LANE_KEEP_ASSIST_ENABLED should always return true or false. If the feature is
     * not available due to some temporary state, such as the vehicle speed being too low or too
     * high, that information must be conveyed through the ErrorState values in the
     * LANE_KEEP_ASSIST_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    LANE_KEEP_ASSIST_ENABLED =
            0x1008 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Lane Keep Assist (LKA) state.
     *
     * Returns the current state of LKA. This property must always return a valid state defined in
     * LaneKeepAssistState or ErrorState. It must not surface errors through StatusCode
     * and must use the supported error states instead.
     *
     * If LKA includes lane departure warnings before applying steering corrections, those warnings
     * must be surfaced through the Lane Departure Warning (LDW) properties.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both LaneKeepAssistState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum LaneKeepAssistState
     * @data_enum ErrorState
     */
    LANE_KEEP_ASSIST_STATE =
            0x1009 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable Lane Centering Assist (LCA).
     *
     * Set true to enable LCA and false to disable LCA. When LCA is enabled, the ADAS system in the
     * vehicle should be turned on and waiting for an activation signal from the driver. Once the
     * feature is activated, the ADAS system should be steering the vehicle to keep it centered in
     * its current lane.
     *
     * This is different from Lane Keep Assist (LKA) which monitors if the driver unintentionally
     * drifts toward or over the lane marking. If an unintentional lane departure is detected, the
     * system applies steering control to return the vehicle into the current lane.
     *
     * In general, LANE_CENTERING_ASSIST_ENABLED should always return true or false. If the feature
     * is not available due to some temporary state, such as the vehicle speed being too low or too
     * high, that information must be conveyed through the ErrorState values in the
     * LANE_CENTERING_ASSIST_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    LANE_CENTERING_ASSIST_ENABLED =
            0x100A + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Lane Centering Assist (LCA) commands.
     *
     * Commands to activate and suspend LCA.
     *
     * When the command ACTIVATE from LaneCenteringAssistCommand is sent,
     * LANE_CENTERING_ASSIST_STATE must be set to LaneCenteringAssistState#ACTIVATION_REQUESTED.
     * When the ACTIVATE command succeeds, LANE_CENTERING_ASSIST_STATE must be set to
     * LaneCenteringAssistState#ACTIVATED. When the command DEACTIVATE from
     * LaneCenteringAssistCommand succeeds, LANE_CENTERING_ASSIST_STATE must be set to
     * LaneCenteringAssistState#ENABLED.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues must be defined unless
     * all enum values of LaneCenteringAssistCommand are supported.
     *
     * When this property is not available because LCA is disabled (i.e.
     * LANE_CENTERING_ASSIST_ENABLED is false), this property must return
     * StatusCode#NOT_AVAILABLE_DISABLED. If LANE_CENTERING_ASSIST_STATE is implemented and the
     * state is set to an ErrorState value, then this property must return a StatusCode that aligns
     * with the ErrorState value. For example, if LANE_CENTERING_ASSIST_STATE is set to
     * ErrorState#NOT_AVAILABLE_SPEED_LOW, then this property must return
     * StatusCode#NOT_AVAILABLE_SPEED_LOW.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     * @data_enum LaneCenteringAssistCommand
     */
    LANE_CENTERING_ASSIST_COMMAND =
            0x100B + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Lane Centering Assist (LCA) state.
     *
     * Returns the current state of LCA. This property must always return a valid state defined in
     * LaneCenteringAssistState or ErrorState. It must not surface errors through StatusCode
     * and must use the supported error states instead.
     *
     * If LCA includes lane departure warnings, those warnings must be surfaced through the Lane
     * Departure Warning (LDW) properties.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both LaneCenteringAssistState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum LaneCenteringAssistState
     * @data_enum ErrorState
     */
    LANE_CENTERING_ASSIST_STATE =
            0x100C + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /*
     * Enable or disable Emergency Lane Keep Assist (ELKA).
     *
     * Set true to enable ELKA and false to disable ELKA. When ELKA is enabled, the ADAS system in
     * the vehicle should be on and monitoring for unsafe lane changes by the driver. When an unsafe
     * maneuver is detected, ELKA alerts the driver and applies steering corrections to keep the
     * vehicle in its original lane.
     *
     * In general, EMERGENCY_LANE_KEEP_ASSIST_ENABLED should always return true or false. If the
     * feature is not available due to some temporary state, such as the vehicle speed being too
     * low, that information must be conveyed through the ErrorState values in the
     * EMERGENCY_LANE_KEEP_ASSIST_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    EMERGENCY_LANE_KEEP_ASSIST_ENABLED =
            0x100D + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Emergency Lane Keep Assist (ELKA) state.
     *
     * Returns the current state of ELKA. Generally, this property should return a valid state
     * defined in the EmergencyLaneKeepAssistState or ErrorState. For example, if the feature is not
     * available due to some temporary state, that information should be conveyed through
     * ErrorState.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of EmergencyLaneKeepAssistState (including OTHER, which is not recommended)
     * and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum EmergencyLaneKeepAssistState
     * @data_enum ErrorState
     */
    EMERGENCY_LANE_KEEP_ASSIST_STATE =
            0x100E + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable cruise control (CC).
     *
     * Set true to enable CC and false to disable CC. This property is shared by all forms of
     * CruiseControlType(s).
     *
     * When CC is enabled, the ADAS system in the vehicle should be turned on and responding to
     * commands.
     *
     * In general, CRUISE_CONTROL_ENABLED should always return true or false. If the feature is not
     * available due to some temporary state, such as the vehicle speed being too low, that
     * information must be conveyed through the ErrorState values in the CRUISE_CONTROL_STATE
     * property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    CRUISE_CONTROL_ENABLED =
            0x100F + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Current type of Cruise Control (CC).
     *
     * When CRUISE_CONTROL_ENABLED is true, this property returns the type of CC that is currently
     * enabled (for example, standard CC, adaptive CC, predictive CC, etc.). Generally, this
     * property should return a valid state defined in the CruiseControlType or ErrorState. For
     * example, if the feature is not available due to some temporary state, that information should
     * be conveyed through ErrorState.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of CruiseControlType (including OTHER, which is not recommended) and
     * ErrorState are supported.
     *
     * Trying to write CruiseControlType#OTHER or an ErrorState to this property will throw an
     * IllegalArgumentException.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @data_enum CruiseControlType
     * @data_enum ErrorState
     */
    CRUISE_CONTROL_TYPE =
            0x1010 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Current state of Cruise Control (CC).
     *
     * This property returns the current state of CC. Generally, this property should return a valid
     * state defined in the CruiseControlState or ErrorState. For example, if the feature is not
     * available due to some temporary state, that information should be conveyed through
     * ErrorState.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of CruiseControlState (including OTHER, which is not recommended) and
     * ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum CruiseControlState
     * @data_enum ErrorState
     */
    CRUISE_CONTROL_STATE =
            0x1011 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Write Cruise Control (CC) commands.
     *
     * See CruiseControlCommand for the details about each supported command.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of CruiseControlState are supported. Any unsupported commands sent through
     * this property must return StatusCode#INVALID_ARG.
     *
     * When this property is not available because CC is disabled (i.e. CRUISE_CONTROL_ENABLED is
     * false), this property must return StatusCode#NOT_AVAILABLE_DISABLED. If CRUISE_CONTROL_STATE
     * is implemented and the state is set to an ErrorState value, then this property must return a
     * StatusCode that aligns with the ErrorState value. For example, if CRUISE_CONTROL_STATE is set
     * to ErrorState#NOT_AVAILABLE_SPEED_LOW, then this property must return
     * StatusCode#NOT_AVAILABLE_SPEED_LOW.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.WRITE
     * @data_enum CruiseControlCommand
     */
    CRUISE_CONTROL_COMMAND =
            0x1012 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Current target speed for Cruise Control (CC).
     *
     * OEMs should set the minFloatValue and maxFloatValue values for this property to define the
     * min and max target speed values. These values must be non-negative.
     *
     * The maxFloatValue represents the upper bound of the target speed.
     * The minFloatValue represents the lower bound of the target speed.
     *
     * When this property is not available because CC is disabled (i.e. CRUISE_CONTROL_ENABLED is
     * false), this property must return StatusCode#NOT_AVAILABLE_DISABLED. If CRUISE_CONTROL_STATE
     * is implemented and the state is set to an ErrorState value, then this property must return a
     * StatusCode that aligns with the ErrorState value. For example, if CRUISE_CONTROL_STATE is set
     * to ErrorState#NOT_AVAILABLE_SPEED_LOW, then this property must return
     * StatusCode#NOT_AVAILABLE_SPEED_LOW.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:METER_PER_SEC
     */
    CRUISE_CONTROL_TARGET_SPEED =
            0x1013 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.FLOAT,

    /**
     * Current target time gap for Adaptive Cruise Control (ACC) or Predictive Cruise Control in
     * milliseconds.
     *
     * This property should specify the target time gap to a leading vehicle. This gap is defined as
     * the time to travel the distance between the leading vehicle's rear-most point to the ACC
     * vehicle's front-most point. The actual time gap from a leading vehicle can be above or below
     * this value.
     *
     * The possible values to set for the target time gap should be specified in configArray in
     * ascending order. All values must be positive. If the property is writable, all values must be
     * writable.
     *
     * When this property is not available because CC is disabled (i.e. CRUISE_CONTROL_ENABLED is
     * false), this property must return StatusCode#NOT_AVAILABLE_DISABLED. If CRUISE_CONTROL_STATE
     * is implemented and the state is set to an ErrorState value, then this property must return a
     * StatusCode that aligns with the ErrorState value. For example, if CRUISE_CONTROL_STATE is set
     * to ErrorState#NOT_AVAILABLE_SPEED_LOW, then this property must return
     * StatusCode#NOT_AVAILABLE_SPEED_LOW.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:MILLI_SECS
     */
    ADAPTIVE_CRUISE_CONTROL_TARGET_TIME_GAP =
            0x1014 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Measured distance from leading vehicle when using Adaptive Cruise Control (ACC) or
     * Predictive Cruise Control.
     *
     * Returns the measured distance in millimeters between the rear-most point of the leading
     * vehicle and the front-most point of the ACC vehicle.
     *
     * The maxInt32Value and minInt32Value in VehicleAreaConfig must be defined.
     * The minInt32Value should be 0.
     * The maxInt32Value should be populated with the maximum range the distance sensor can support.
     * This value should be non-negative.
     *
     * When no lead vehicle is detected (that is, when there is no leading vehicle or the leading
     * vehicle is too far away for the sensor to detect), this property should return
     * StatusCode.NOT_AVAILABLE.
     *
     * When this property is not available because CC is disabled (i.e. CRUISE_CONTROL_ENABLED is
     * false), this property must return StatusCode#NOT_AVAILABLE_DISABLED. If CRUISE_CONTROL_STATE
     * is implemented and the state is set to an ErrorState value, then this property must return a
     * StatusCode that aligns with the ErrorState value. For example, if CRUISE_CONTROL_STATE is set
     * to ErrorState#NOT_AVAILABLE_SPEED_LOW, then this property must return
     * StatusCode#NOT_AVAILABLE_SPEED_LOW.
     *
     * @change_mode VehiclePropertyChangeMode.CONTINUOUS
     * @access VehiclePropertyAccess.READ
     * @unit VehicleUnit:MILLIMETER
     */
    ADAPTIVE_CRUISE_CONTROL_LEAD_VEHICLE_MEASURED_DISTANCE =
            0x1015 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable Hands On Detection (HOD).
     *
     * Set true to enable HOD and false to disable HOD. When HOD is enabled, a system inside the
     * vehicle should be monitoring the presence of the driver's hands on the steering wheel and
     * send a warning if it detects that the driver's hands are no longer on the steering wheel.
     *
     * In general, HANDS_ON_DETECTION_ENABLED should always return true or false. If the feature is
     * not available due to some temporary state, that information must be conveyed through the
     * ErrorState values in the HANDS_ON_DETECTION_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    HANDS_ON_DETECTION_ENABLED =
            0x1016 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Hands On Detection (HOD) driver state.
     *
     * Returns whether the driver's hands are on the steering wheel. Generally, this property should
     * return a valid state defined in the HandsOnDetectionDriverState or ErrorState. For example,
     * if the feature is not available due to some temporary state, that information should be
     * conveyed through ErrorState.
     *
     * If the vehicle wants to send a warning to the user because the driver's hands have been off
     * the steering wheel for too long, the warning should be surfaced through
     * HANDS_ON_DETECTION_WARNING.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both HandsOnDetectionDriverState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum HandsOnDetectionDriverState
     * @data_enum ErrorState
     */
    HANDS_ON_DETECTION_DRIVER_STATE =
            0x1017 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Hands On Detection (HOD) warning.
     *
     * Returns whether a warning is being sent to the driver for having their hands off the wheel
     * for too long a duration.
     *
     * Generally, this property should return a valid state defined in HandsOnDetectionWarning or
     * ErrorState. For example, if the feature is not available due to some temporary state, that
     * information should be conveyed through an ErrorState.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both HandsOnDetectionWarning (including OTHER, which is not recommended)
     * and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum HandsOnDetectionWarning
     * @data_enum ErrorState
     */
    HANDS_ON_DETECTION_WARNING =
            0x1018 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable driver drowsiness and attention monitoring.
     *
     * Set true to enable driver drowsiness and attention monitoring and false to disable driver
     * drowsiness and attention monitoring. When driver drowsiness and attention monitoring is
     * enabled, a system inside the vehicle should be monitoring the drowsiness and attention level
     * of the driver and warn the driver if needed.
     *
     * In general, DRIVER_DROWSINESS_ATTENTION_SYSTEM_ENABLED should always return true or false.
     * If the feature is not available due to some temporary state, that information must be
     * conveyed through the ErrorState values in the DRIVER_DROWSINESS_ATTENTION_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    DRIVER_DROWSINESS_ATTENTION_SYSTEM_ENABLED =
            0x1019 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Driver drowsiness and attention level state.
     *
     * Returns the current detected state of driver drowiness and attention level based on the
     * Karolinska Sleepiness scale. If alternative measurement methods are used, the value should be
     * translated to the Karolinska Sleepiness Scale equivalent.
     *
     * Generally, this property should return a valid state defined in the
     * DriverDrowsinessAttentionState or ErrorState. For example, if the feature is not available
     * due to some temporary state, that information should be conveyed through ErrorState.
     *
     * If the vehicle is sending a warning to the user because the driver is too drowsy, the warning
     * should be surfaced through {@link #DRIVER_DROWSINESS_ATTENTION_WARNING}.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both DriverDrowsinessAttentionState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum DriverDrowsinessAttentionState
     * @data_enum ErrorState
     */
    DRIVER_DROWSINESS_ATTENTION_STATE =
            0x101A + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable driver drowsiness and attention warnings.
     *
     * Set true to enable driver drowsiness and attention warnings and false to disable driver
     * drowsiness and attention warnings.
     *
     * When driver drowsiness and attention warnings are enabled, the driver drowsiness and
     * attention monitoring system inside the vehicle should warn the driver when it detects the
     * driver is drowsy or not attentive.
     *
     * In general, DRIVER_DROWSINESS_ATTENTION_WARNING_ENABLED should always return true or false.
     * If the feature is not available due to some temporary state, that information must be
     * conveyed through the ErrorState values in the DRIVER_DROWSINESS_ATTENTION_WARNING property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    DRIVER_DROWSINESS_ATTENTION_WARNING_ENABLED =
            0x101B + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Driver drowsiness and attention warning.
     *
     * Returns whether a warning is being sent to the driver for being drowsy or not attentive.
     *
     * Generally, this property should return a valid state defined in
     * DriverDrowsinessAttentionWarning or ErrorState. For example, if the feature is not available
     * due to some temporary state, that information should be conveyed through an ErrorState.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both DriverDrowsinessAttentionWarning (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum DriverDrowsinessAttentionWarning
     * @data_enum ErrorState
     */
    DRIVER_DROWSINESS_ATTENTION_WARNING =
            0x101C + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable driver distraction monitoring.
     *
     * Set true to enable driver distraction monitoring and false to disable driver
     * distraction monitoring. When driver distraction monitoring is enabled, a system
     * inside the vehicle should be monitoring the distraction level of the driver and
     * warn the driver if needed.
     *
     * In general, DRIVER_DISTRACTION_SYSTEM_ENABLED should always return true or false. If the
     * feature is not available due to some temporary state, that information must be conveyed
     * through the ErrorState values in the DRIVER_DISTRACTION_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    DRIVER_DISTRACTION_SYSTEM_ENABLED =
            0x101D + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Driver distraction state.
     *
     * Returns the current detected driver distraction state.
     *
     * Generally, this property should return a valid state defined in the DriverDistractionState or
     * ErrorState. For example, if the feature is not available due to some temporary state, that
     * information should be conveyed through ErrorState.
     *
     * If the vehicle is sending a warning to the user because the driver is too distracted, the
     * warning should be surfaced through {@link #DRIVER_DISTRACTION_WARNING}.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both DriverDistractionState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum DriverDistractionState
     * @data_enum ErrorState
     */
    DRIVER_DISTRACTION_STATE =
            0x101E + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable driver distraction warnings.
     *
     * Set true to enable driver distraction warnings and false to disable driver distraction
     * warnings.
     *
     * When driver distraction warnings are enabled, the driver distraction monitoring system inside
     * the vehicle should warn the driver when it detects the driver is distracted.
     *
     * In general, DRIVER_DISTRACTION_WARNING_ENABLED should always return true or false. If the
     * feature is not available due to some temporary state, that information must be conveyed
     * through the ErrorState values in the DRIVER_DISTRACTION_WARNING property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    DRIVER_DISTRACTION_WARNING_ENABLED =
            0x101F + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Driver distraction warning.
     *
     * Returns whether a warning is being sent to the driver for being distracted.
     *
     * Generally, this property should return a valid state defined in DriverDistractionWarning or
     * ErrorState. For example, if the feature is not available due to some temporary state, that
     * information should be conveyed through an ErrorState.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both DriverDistractionWarning (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum DriverDistractionWarning
     * @data_enum ErrorState
     */
    DRIVER_DISTRACTION_WARNING =
            0x1020 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable Low Speed Collision Warning.
     *
     * Set true to enable low speed collision warning and false to disable low speed collision
     * warning. When low speed collision warning is enabled, the ADAS system in the vehicle should
     * warn the driver of potential collisions at low speeds. This property is different from the
     * pre-existing FORWARD_COLLISION_WARNING_ENABLED, which should apply to higher speed
     * applications only. If the vehicle doesn't have a separate collision detection system for low
     * speed environments, this property should not be implemented.
     *
     * In general, LOW_SPEED_COLLISION_WARNING_ENABLED should always return true or false. If the
     * feature is not available due to some temporary state, such as the vehicle speed being too
     * high, that information must be conveyed through the ErrorState values in the
     * LOW_SPEED_COLLISION_WARNING_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    LOW_SPEED_COLLISION_WARNING_ENABLED =
            0x1021 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Low Speed Collision Warning state.
     *
     * Returns the current state of Low Speed Collision Warning. This property must always return a
     * valid state defined in LowSpeedCollisionWarningState or ErrorState. It must not surface
     * errors through StatusCode and must use the supported error states instead. This property is
     * different from the pre-existing FORWARD_COLLISION_WARNING_STATE, which should apply to higher
     * speed applications only. If the vehicle doesn't have a separate collision detection system
     * for low speed environments, this property should not be implemented.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both LowSpeedCollisionWarningState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum LowSpeedCollisionWarningState
     * @data_enum ErrorState
     */
    LOW_SPEED_COLLISION_WARNING_STATE =
            0x1022 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /**
     * Enable or disable Cross Traffic Monitoring.
     *
     * Set true to enable Cross Traffic Monitoring and false to disable Cross Traffic Monitoring.
     * When Cross Traffic Monitoring is enabled, the ADAS system in the vehicle should be turned on
     * and monitoring for potential sideways collisions.
     *
     * In general, CROSS_TRAFFIC_MONITORING_ENABLED should always return true or false. If the
     * feature is not available due to some temporary state, such as the vehicle speed being too
     * high, that information must be conveyed through the ErrorState values in the
     * CROSS_TRAFFIC_MONITORING_STATE property.
     *
     * This property is defined as VehiclePropertyAccess.READ_WRITE, but OEMs have the option to
     * implement it as VehiclePropertyAccess.READ only.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ_WRITE
     * @access VehiclePropertyAccess.READ
     */
    CROSS_TRAFFIC_MONITORING_ENABLED =
            0x1023 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.BOOLEAN,

    /**
     * Cross Traffic Monitoring warning state.
     *
     * Returns the current state of Cross Traffic Monitoring Warning. This property must always
     * return a valid state defined in CrossTrafficMonitoringWarningState or ErrorState. It must not
     * surface errors through StatusCode and must use the supported error states instead.
     *
     * For the global area ID (0), the VehicleAreaConfig#supportedEnumValues array must be defined
     * unless all states of both CrossTrafficMonitoringWarningState (including OTHER, which is not
     * recommended) and ErrorState are supported.
     *
     * @change_mode VehiclePropertyChangeMode.ON_CHANGE
     * @access VehiclePropertyAccess.READ
     * @data_enum CrossTrafficMonitoringWarningState
     * @data_enum ErrorState
     */
    CROSS_TRAFFIC_MONITORING_WARNING_STATE =
            0x1024 + VehiclePropertyGroup.SYSTEM + VehicleArea.GLOBAL + VehiclePropertyType.INT32,

    /***************************************************************************
     * End of ADAS Properties
     **************************************************************************/
}
