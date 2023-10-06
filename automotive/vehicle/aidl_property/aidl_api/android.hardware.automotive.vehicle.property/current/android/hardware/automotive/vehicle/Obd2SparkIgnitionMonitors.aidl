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
enum Obd2SparkIgnitionMonitors {
  COMPONENTS_AVAILABLE = (0x1 << 0) /* 1 */,
  COMPONENTS_INCOMPLETE = (0x1 << 1) /* 2 */,
  FUEL_SYSTEM_AVAILABLE = (0x1 << 2) /* 4 */,
  FUEL_SYSTEM_INCOMPLETE = (0x1 << 3) /* 8 */,
  MISFIRE_AVAILABLE = (0x1 << 4) /* 16 */,
  MISFIRE_INCOMPLETE = (0x1 << 5) /* 32 */,
  EGR_AVAILABLE = (0x1 << 6) /* 64 */,
  EGR_INCOMPLETE = (0x1 << 7) /* 128 */,
  OXYGEN_SENSOR_HEATER_AVAILABLE = (0x1 << 8) /* 256 */,
  OXYGEN_SENSOR_HEATER_INCOMPLETE = (0x1 << 9) /* 512 */,
  OXYGEN_SENSOR_AVAILABLE = (0x1 << 10) /* 1024 */,
  OXYGEN_SENSOR_INCOMPLETE = (0x1 << 11) /* 2048 */,
  AC_REFRIGERANT_AVAILABLE = (0x1 << 12) /* 4096 */,
  AC_REFRIGERANT_INCOMPLETE = (0x1 << 13) /* 8192 */,
  SECONDARY_AIR_SYSTEM_AVAILABLE = (0x1 << 14) /* 16384 */,
  SECONDARY_AIR_SYSTEM_INCOMPLETE = (0x1 << 15) /* 32768 */,
  EVAPORATIVE_SYSTEM_AVAILABLE = (0x1 << 16) /* 65536 */,
  EVAPORATIVE_SYSTEM_INCOMPLETE = (0x1 << 17) /* 131072 */,
  HEATED_CATALYST_AVAILABLE = (0x1 << 18) /* 262144 */,
  HEATED_CATALYST_INCOMPLETE = (0x1 << 19) /* 524288 */,
  CATALYST_AVAILABLE = (0x1 << 20) /* 1048576 */,
  CATALYST_INCOMPLETE = (0x1 << 21) /* 2097152 */,
}
