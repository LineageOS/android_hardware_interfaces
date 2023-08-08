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
enum UwbVendorCapabilityTlvTypes {
  SUPPORTED_POWER_STATS_QUERY = 0xC0,
  CCC_SUPPORTED_CHAPS_PER_SLOT = 0xA0,
  CCC_SUPPORTED_SYNC_CODES = 0xA1,
  CCC_SUPPORTED_HOPPING_CONFIG_MODES_AND_SEQUENCES = 0xA2,
  CCC_SUPPORTED_CHANNELS = 0xA3,
  CCC_SUPPORTED_VERSIONS = 0xA4,
  CCC_SUPPORTED_UWB_CONFIGS = 0xA5,
  CCC_SUPPORTED_PULSE_SHAPE_COMBOS = 0xA6,
  CCC_SUPPORTED_RAN_MULTIPLIER = 0xA7,
  CCC_SUPPORTED_MAX_RANGING_SESSION_NUMBER = 0xA8,
  CCC_SUPPORTED_MIN_UWB_INITIATION_TIME_MS = 0xA9,
  CCC_PRIORITIZED_CHANNEL_LIST = 0xAA,
  RADAR_SUPPORT = 0xB0,
  SUPPORTED_AOA_RESULT_REQ_ANTENNA_INTERLEAVING = 0xE3,
  SUPPORTED_MIN_RANGING_INTERVAL_MS = 0xE4,
  SUPPORTED_RANGE_DATA_NTF_CONFIG = 0xE5,
  SUPPORTED_RSSI_REPORTING = 0xE6,
  SUPPORTED_DIAGNOSTICS = 0xE7,
  SUPPORTED_MIN_SLOT_DURATION_RSTU = 0xE8,
  SUPPORTED_MAX_RANGING_SESSION_NUMBER = 0xE9,
  SUPPORTED_CHANNELS_AOA = 0xEA,
}
