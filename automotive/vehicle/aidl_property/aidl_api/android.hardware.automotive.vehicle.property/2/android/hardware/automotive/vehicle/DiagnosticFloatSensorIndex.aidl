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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.automotive.vehicle;
@Backing(type="int") @VintfStability
enum DiagnosticFloatSensorIndex {
  CALCULATED_ENGINE_LOAD = 0,
  ENGINE_COOLANT_TEMPERATURE = 1,
  SHORT_TERM_FUEL_TRIM_BANK1 = 2,
  LONG_TERM_FUEL_TRIM_BANK1 = 3,
  SHORT_TERM_FUEL_TRIM_BANK2 = 4,
  LONG_TERM_FUEL_TRIM_BANK2 = 5,
  FUEL_PRESSURE = 6,
  INTAKE_MANIFOLD_ABSOLUTE_PRESSURE = 7,
  ENGINE_RPM = 8,
  VEHICLE_SPEED = 9,
  TIMING_ADVANCE = 10,
  MAF_AIR_FLOW_RATE = 11,
  THROTTLE_POSITION = 12,
  OXYGEN_SENSOR1_VOLTAGE = 13,
  OXYGEN_SENSOR1_SHORT_TERM_FUEL_TRIM = 14,
  OXYGEN_SENSOR1_FUEL_AIR_EQUIVALENCE_RATIO = 15,
  OXYGEN_SENSOR2_VOLTAGE = 16,
  OXYGEN_SENSOR2_SHORT_TERM_FUEL_TRIM = 17,
  OXYGEN_SENSOR2_FUEL_AIR_EQUIVALENCE_RATIO = 18,
  OXYGEN_SENSOR3_VOLTAGE = 19,
  OXYGEN_SENSOR3_SHORT_TERM_FUEL_TRIM = 20,
  OXYGEN_SENSOR3_FUEL_AIR_EQUIVALENCE_RATIO = 21,
  OXYGEN_SENSOR4_VOLTAGE = 22,
  OXYGEN_SENSOR4_SHORT_TERM_FUEL_TRIM = 23,
  OXYGEN_SENSOR4_FUEL_AIR_EQUIVALENCE_RATIO = 24,
  OXYGEN_SENSOR5_VOLTAGE = 25,
  OXYGEN_SENSOR5_SHORT_TERM_FUEL_TRIM = 26,
  OXYGEN_SENSOR5_FUEL_AIR_EQUIVALENCE_RATIO = 27,
  OXYGEN_SENSOR6_VOLTAGE = 28,
  OXYGEN_SENSOR6_SHORT_TERM_FUEL_TRIM = 29,
  OXYGEN_SENSOR6_FUEL_AIR_EQUIVALENCE_RATIO = 30,
  OXYGEN_SENSOR7_VOLTAGE = 31,
  OXYGEN_SENSOR7_SHORT_TERM_FUEL_TRIM = 32,
  OXYGEN_SENSOR7_FUEL_AIR_EQUIVALENCE_RATIO = 33,
  OXYGEN_SENSOR8_VOLTAGE = 34,
  OXYGEN_SENSOR8_SHORT_TERM_FUEL_TRIM = 35,
  OXYGEN_SENSOR8_FUEL_AIR_EQUIVALENCE_RATIO = 36,
  FUEL_RAIL_PRESSURE = 37,
  FUEL_RAIL_GAUGE_PRESSURE = 38,
  COMMANDED_EXHAUST_GAS_RECIRCULATION = 39,
  EXHAUST_GAS_RECIRCULATION_ERROR = 40,
  COMMANDED_EVAPORATIVE_PURGE = 41,
  FUEL_TANK_LEVEL_INPUT = 42,
  EVAPORATION_SYSTEM_VAPOR_PRESSURE = 43,
  CATALYST_TEMPERATURE_BANK1_SENSOR1 = 44,
  CATALYST_TEMPERATURE_BANK2_SENSOR1 = 45,
  CATALYST_TEMPERATURE_BANK1_SENSOR2 = 46,
  CATALYST_TEMPERATURE_BANK2_SENSOR2 = 47,
  ABSOLUTE_LOAD_VALUE = 48,
  FUEL_AIR_COMMANDED_EQUIVALENCE_RATIO = 49,
  RELATIVE_THROTTLE_POSITION = 50,
  ABSOLUTE_THROTTLE_POSITION_B = 51,
  ABSOLUTE_THROTTLE_POSITION_C = 52,
  ACCELERATOR_PEDAL_POSITION_D = 53,
  ACCELERATOR_PEDAL_POSITION_E = 54,
  ACCELERATOR_PEDAL_POSITION_F = 55,
  COMMANDED_THROTTLE_ACTUATOR = 56,
  ETHANOL_FUEL_PERCENTAGE = 57,
  ABSOLUTE_EVAPORATION_SYSTEM_VAPOR_PRESSURE = 58,
  SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK1 = 59,
  SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK2 = 60,
  SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK3 = 61,
  SHORT_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK4 = 62,
  LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK1 = 63,
  LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK2 = 64,
  LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK3 = 65,
  LONG_TERM_SECONDARY_OXYGEN_SENSOR_TRIM_BANK4 = 66,
  RELATIVE_ACCELERATOR_PEDAL_POSITION = 67,
  HYBRID_BATTERY_PACK_REMAINING_LIFE = 68,
  FUEL_INJECTION_TIMING = 69,
  ENGINE_FUEL_RATE = 70,
}
