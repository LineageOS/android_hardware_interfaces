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
enum VehiclePropertyStatus {
  AVAILABLE = 0x00,
  UNAVAILABLE = 0x01,
  NOT_AVAILABLE_GENERAL = 0x01,
  ERROR = 0x02,
  NOT_AVAILABLE_DISABLED = (0x1000 | 0x01) /* 4097 */,
  NOT_AVAILABLE_SPEED_LOW = (0x1000 | 0x02) /* 4098 */,
  NOT_AVAILABLE_SPEED_HIGH = (0x1000 | 0x03) /* 4099 */,
  NOT_AVAILABLE_POOR_VISIBILITY = (0x1000 | 0x04) /* 4100 */,
  NOT_AVAILABLE_SAFETY = (0x1000 | 0x05) /* 4101 */,
  NOT_AVAILABLE_SUBSYSTEM_NOT_CONNECTED = (0x1000 | 0x06) /* 4102 */,
}
