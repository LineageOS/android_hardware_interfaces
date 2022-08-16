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
 * This enum provides the canonical mapping for sensor properties that have an integer value.
 * The ordering of the values is taken from the OBD2 specification.
 * Some of the properties are represented as an integer mapping to another enum. In those cases
 * expect a comment by the property definition describing the enum to look at for the mapping.
 * Any value greater than the last reserved index is available to vendors to map their extensions.
 * While these values do not directly map to SAE J1979 PIDs, an equivalence is listed next
 * to each one to aid implementors.
 */
@VintfStability
@Backing(type="int")
enum DiagnosticIntegerSensorIndex {
    /**
     * refer to FuelSystemStatus for a description of this value.
     */
    FUEL_SYSTEM_STATUS = 0,
    /*
     * PID 0x03
     */
    MALFUNCTION_INDICATOR_LIGHT_ON = 1,
    /*
     * PID 0x01
     *
     *
     * refer to IgnitionMonitorKind for a description of this value.
     */
    IGNITION_MONITORS_SUPPORTED = 2,
    /*
     * PID 0x01
     *
     *
     * The value of this sensor is a bitmask that specifies whether ignition-specific
     * tests are available and whether they are complete. The semantics of the individual
     * bits in this value are given by, respectively, SparkIgnitionMonitors and
     * CompressionIgnitionMonitors depending on the value of IGNITION_MONITORS_SUPPORTED.
     */
    IGNITION_SPECIFIC_MONITORS = 3,
    /*
     * PID 0x01
     */
    INTAKE_AIR_TEMPERATURE = 4,
    /*
     * PID 0x0F
     *
     *
     * refer to SecondaryAirStatus for a description of this value.
     */
    COMMANDED_SECONDARY_AIR_STATUS = 5,
    /*
     * PID 0x12
     */
    NUM_OXYGEN_SENSORS_PRESENT = 6,
    /*
     * PID 0x13
     */
    RUNTIME_SINCE_ENGINE_START = 7,
    /*
     * PID 0x1F
     */
    DISTANCE_TRAVELED_WITH_MALFUNCTION_INDICATOR_LIGHT_ON = 8,
    /*
     * PID 0x21
     */
    WARMUPS_SINCE_CODES_CLEARED = 9,
    /*
     * PID 0x30
     */
    DISTANCE_TRAVELED_SINCE_CODES_CLEARED = 10,
    /*
     * PID 0x31
     */
    ABSOLUTE_BAROMETRIC_PRESSURE = 11,
    /*
     * PID 0x33
     */
    CONTROL_MODULE_VOLTAGE = 12,
    /*
     * PID 0x42
     */
    AMBIENT_AIR_TEMPERATURE = 13,
    /*
     * PID 0x46
     */
    TIME_WITH_MALFUNCTION_LIGHT_ON = 14,
    /*
     * PID 0x4D
     */
    TIME_SINCE_TROUBLE_CODES_CLEARED = 15,
    /*
     * PID 0x4E
     */
    MAX_FUEL_AIR_EQUIVALENCE_RATIO = 16,
    /*
     * PID 0x4F
     */
    MAX_OXYGEN_SENSOR_VOLTAGE = 17,
    /*
     * PID 0x4F
     */
    MAX_OXYGEN_SENSOR_CURRENT = 18,
    /*
     * PID 0x4F
     */
    MAX_INTAKE_MANIFOLD_ABSOLUTE_PRESSURE = 19,
    /*
     * PID 0x4F
     */
    MAX_AIR_FLOW_RATE_FROM_MASS_AIR_FLOW_SENSOR = 20,
    /*
     * PID 0x50
     *
     *
     * refer to FuelType for a description of this value.
     */
    FUEL_TYPE = 21,
    /*
     * PID 0x51
     */
    FUEL_RAIL_ABSOLUTE_PRESSURE = 22,
    /*
     * PID 0x59
     */
    ENGINE_OIL_TEMPERATURE = 23,
    /*
     * PID 0x5C
     */
    DRIVER_DEMAND_PERCENT_TORQUE = 24,
    /*
     * PID 0x61
     */
    ENGINE_ACTUAL_PERCENT_TORQUE = 25,
    /*
     * PID 0x62
     */
    ENGINE_REFERENCE_PERCENT_TORQUE = 26,
    /*
     * PID 0x63
     */
    ENGINE_PERCENT_TORQUE_DATA_IDLE = 27,
    /*
     * PID 0x64
     */
    ENGINE_PERCENT_TORQUE_DATA_POINT1 = 28,
    /*
     * PID 0x64
     */
    ENGINE_PERCENT_TORQUE_DATA_POINT2 = 29,
    /*
     * PID 0x64
     */
    ENGINE_PERCENT_TORQUE_DATA_POINT3 = 30,
    /*
     * PID 0x64
     */
    ENGINE_PERCENT_TORQUE_DATA_POINT4 = 31,
    /*
     * PID 0x64
     */
    // LAST_SYSTEM_INDEX = 31,
}
