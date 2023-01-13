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
enum DiagnosticIntegerSensorIndex {
  FUEL_SYSTEM_STATUS = 0,
  MALFUNCTION_INDICATOR_LIGHT_ON = 1,
  IGNITION_MONITORS_SUPPORTED = 2,
  IGNITION_SPECIFIC_MONITORS = 3,
  INTAKE_AIR_TEMPERATURE = 4,
  COMMANDED_SECONDARY_AIR_STATUS = 5,
  NUM_OXYGEN_SENSORS_PRESENT = 6,
  RUNTIME_SINCE_ENGINE_START = 7,
  DISTANCE_TRAVELED_WITH_MALFUNCTION_INDICATOR_LIGHT_ON = 8,
  WARMUPS_SINCE_CODES_CLEARED = 9,
  DISTANCE_TRAVELED_SINCE_CODES_CLEARED = 10,
  ABSOLUTE_BAROMETRIC_PRESSURE = 11,
  CONTROL_MODULE_VOLTAGE = 12,
  AMBIENT_AIR_TEMPERATURE = 13,
  TIME_WITH_MALFUNCTION_LIGHT_ON = 14,
  TIME_SINCE_TROUBLE_CODES_CLEARED = 15,
  MAX_FUEL_AIR_EQUIVALENCE_RATIO = 16,
  MAX_OXYGEN_SENSOR_VOLTAGE = 17,
  MAX_OXYGEN_SENSOR_CURRENT = 18,
  MAX_INTAKE_MANIFOLD_ABSOLUTE_PRESSURE = 19,
  MAX_AIR_FLOW_RATE_FROM_MASS_AIR_FLOW_SENSOR = 20,
  FUEL_TYPE = 21,
  FUEL_RAIL_ABSOLUTE_PRESSURE = 22,
  ENGINE_OIL_TEMPERATURE = 23,
  DRIVER_DEMAND_PERCENT_TORQUE = 24,
  ENGINE_ACTUAL_PERCENT_TORQUE = 25,
  ENGINE_REFERENCE_PERCENT_TORQUE = 26,
  ENGINE_PERCENT_TORQUE_DATA_IDLE = 27,
  ENGINE_PERCENT_TORQUE_DATA_POINT1 = 28,
  ENGINE_PERCENT_TORQUE_DATA_POINT2 = 29,
  ENGINE_PERCENT_TORQUE_DATA_POINT3 = 30,
  ENGINE_PERCENT_TORQUE_DATA_POINT4 = 31,
}
