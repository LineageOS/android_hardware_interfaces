/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
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

package android.hardware.uwb.fira_android;
@Backing(type="int") @VintfStability
enum UwbVendorSessionAppConfigTlvTypes {
  CCC_HOP_MODE_KEY = 0xA0,
  CCC_UWB_TIME0 = 0xA1,
  CCC_RANGING_PROTOCOL_VER = 0xA3,
  CCC_UWB_CONFIG_ID = 0xA4,
  CCC_PULSESHAPE_COMBO = 0xA5,
  CCC_URSK_TTL = 0xA6,
  CCC_LAST_INDEX_USED = 0xA8,
  NB_OF_RANGE_MEASUREMENTS = 0xE3,
  NB_OF_AZIMUTH_MEASUREMENTS = 0xE4,
  NB_OF_ELEVATION_MEASUREMENTS = 0xE5,
  ENABLE_DIAGNOSTICS = 0xE8,
  DIAGRAMS_FRAME_REPORTS_FIELDS = 0xE9,
}
