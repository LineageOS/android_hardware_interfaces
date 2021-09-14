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

/**
 * This enum provides the canonical mapping for sensor properties that have a floating-point value.
 * The ordering of the values is taken from the OBD2 specification.
 * Any value greater than the last reserved index is available to vendors to map their extensions.
 * While these values do not directly map to SAE J1979 PIDs, an equivalence is listed next
 * to each one to aid implementors.
 */
@VintfStability
@Backing(type="int")
enum DiagnosticFloatSensorIndex {
    CALCULATED_ENGINE_LOAD = 0,
    /*
     * PID 0x04
     */
    ENGINE_COOLANT_TEMPERATURE = 1,
    /*
     * PID 0x05
     */
    SHORT_TERM_FUEL_TRIM_BANK1 = 2,
    /*
     * PID 0x06
     */
    LONG_TERM_FUEL_TRIM_BANK1 = 3,
    /*
     * PID 0x07
     */
    SHORT_TERM_FUEL_TRIM_BANK2 = 4,
    /*
     * PID 0x08
     */
    LONG_TERM_FUEL_TRIM_BANK2 = 5,
    /*
     * PID 0x09
     */
    FUEL_PRESSURE = 6,
    /*
     * PID 0x0A
     */
    INTAKE_MANIFOLD_ABSOLUTE_PRESSURE = 7,
    /*
     * PID 0x0B
     */
    ENGINE_RPM = 8,
    /*
     * PID 0x0C
     */
    VEHICLE_SPEED = 9,
    /*
     * PID 0x0D
     */
    TIMING_ADVANCE = 10,
    /*
     * PID 0x0E
     */
    MAF_AIR_FLOW_RATE = 11,
    /*
     * PID 0x10
     */
    THROTTLE_POSITION = 12,
    /*
     * PID 0x11
     */
    OXYGEN_SENSOR1_VOLTAGE = 13,
    /*
     * PID 0x14
     */
    OXYGEN_SENSOR1_SHORT_TERM_FUEL_TRIM = 14,
    /*
     * PID 0x14
     */
    OXYGEN_SENSOR1_FUEL_AIR_EQUIVALENCE_RATIO = 15,
    /*
     * PID 0x24
     */
    OXYGEN_SENSOR2_VOLTAGE = 16,
    /*
     * PID 0x15
     */
    OXYGEN_SENSOR2_SHORT_TERM_FUEL_TRIM = 17,
    /*
     * PID 0x15
     */
    OXYGEN_SENSOR2_FUEL_AIR_EQUIVALENCE_RATIO = 18,
    /*
     * PID 0x25
     */
    OXYGEN_SENSOR3_VOLTAGE = 19,
    /*
     * PID 0x16
     */
    OXYGEN_SENSOR3_SHORT_TERM_FUEL_TRIM = 20,
    /*
     * PID 0x16
     */
    OXYGEN_SENSOR3_FUEL_AIR_EQUIVALENCE_RATIO = 21,
    /*
     * PID 0x26
     */
    OXYGEN_SENSOR4_VOLTAGE = 22,
    /*
     * PID 0x17
     */
    OXYGEN_SENSOR4_SHORT_TERM_FUEL_TRIM = 23,
    /*
     * PID 0x17
     */
    OXYGEN_SENSOR4_FUEL_AIR_EQUIVALENCE_RATIO = 24,
    /*
     * PID 0x27
     */
    OXYGEN_SENSOR5_VOLTAGE = 25,
    /*
     * PID 0x18
     */
    OXYGEN_SENSOR5_SHORT_TERM_FUEL_TRIM = 26,
    /*
     * PID 0x18
     */
    OXYGEN_SENSOR5_FUEL_AIR_EQUIVALENCE_RATIO = 27,
    /*
     * PID 0x28
     */
    OXYGEN_SENSOR6_VOLTAGE = 28,
    /*
     * PID 0x19
     */
    OXYGEN_SENSOR6_SHORT_TERM_FUEL_TRIM = 29,
    /*
     * PID 0x19
     */
    OXYGEN_SENSOR6_FUEL_AIR_EQUIVALENCE_RATIO = 30,
    /*
     * PID 0x29
     */
    OXYGEN_SENSOR7_VOLTAGE = 31,
    /*
     * PID 0x1A
     */
    OXYGEN_SENSOR7_SHORT_TERM_FUEL_TRIM = 32,
    /*
     * PID 0x1A
     */
    OXYGEN_SENSOR7_FUEL_AIR_EQUIVALENCE_RATIO = 33,
    /*
     * PID 0x2A
     */
    OXYGEN_SENSOR8_VOLTAGE = 34,
    /*
     * PID 0x1B
     */
    OXYGEN_SENSOR8_SHORT_TERM_FUEL_TRIM = 35,
    /*
     * PID 0x1B
     */
    OXYGEN_SENSOR8_FUEL_AIR_EQUIVALENCE_RATIO = 36,
    /*
     * PID 0x2B
     */
    FUEL_RAIL_PRESSURE = 37,
    /*
     * PID 0x22
     */
    FUEL_RAIL_GAUGE_PRESSURE = 38,
    /*
     * PID 0x23
     */
    COMMANDED_EXHAUST_GAS_RECIRCULATION = 39,
    /*
     * PID 0x2C
     */
    EXHAUST_GAS_RECIRCULATION_ERROR = 40,
    /*
     * PID 0x2D
     */
    COMMANDED_EVAPORATIVE_PURGE = 41,
    /*
     * PID 0x2E
     */
    FUEL_TANK_LEVEL_INPUT = 42,
    /*
     * PID 0x2F
     */
    EVAPORATION_SYSTEM_VAPOR_PRESSURE = 43,
    /*
     * PID 0x32
     */
    CATALYST_TEMPERATURE_BANK1_SENSOR1 = 44,
    /*
     * PID 0x3C
     */
    CATALYST_TEMPERATURE_BANK2_SENSOR1 = 45,
    /*
     * PID 0x3D
     */
    CATALYST_TEMPERATURE_BANK1_SENSOR2 = 46,
    /*
     * PID 0x3E
     */
    CATALYST_TEMPERATURE_BANK2_SENSOR2 = 47,
    /*
     * PID 0x3F
     */
    ABSOLUTE_LOAD_VALUE = 48,
    /*
     * PID 0x43
     */
    FUEL_AIR_COMMANDED_EQUIVALENCE_RATIO = 49,
    /*
     * PID 0x44
     */
    RELATIVE_THROTTLE_POSITION = 50,
    /*
     * PID 0x45
     */
    ABSOLUTE_THROTTLE_POSITION_B = 51,
    /*
     * PID 0x47
     */
    ABSOLUTE_THROTTLE_POSITION_C = 52,
    /*
     * PID 0x48
     */
    ACCELERATOR_PEDAL_POSITION_D = 53,
    /*
     * PID 0x49
     */
    ACCELERATOR_PEDAL_POSITION_E = 54,
    /*
     * PID 0x4A
     */
    ACCELERATOR_PEDAL_POSITION_F = 55,
    /*
     * PID 0x4B
     */
    COMMANDED_THROTTLE_ACTUATOR = 56,
    /*
     * PID 0x4C
     */
    ETHANOL_FUEL_PERCENTAGE = 57,
    /*
     * PID 0x52
     */
    ABSOLUTE_EVAPORATION_SYSTEM_VAPOR_PRESSURE = 58,
    /*
     * PID 0x53
     */
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK1 = 59,
    /*
     * PID 0x55
     */
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK2 = 60,
    /*
     * PID 0x57
     */
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK3 = 61,
    /*
     * PID 0x55
     */
    SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK4 = 62,
    /*
     * PID 0x57
     */
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK1 = 63,
    /*
     * PID 0x56
     */
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK2 = 64,
    /*
     * PID 0x58
     */
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK3 = 65,
    /*
     * PID 0x56
     */
    LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK4 = 66,
    /*
     * PID 0x58
     */
    RELATIVE_ACCELERATOR_PEDAL_POSITION = 67,
    /*
     * PID 0x5A
     */
    HYBRID_BATTERY_PACK_REMAINING_LIFE = 68,
    /*
     * PID 0x5B
     */
    FUEL_INJECTION_TIMING = 69,
    /*
     * PID 0x5D
     */
    ENGINE_FUEL_RATE = 70,
    /*
     * PID 0x5E
     */
    // LAST_SYSTEM_INDEX = ENGINE_FUEL_RATE,
}
