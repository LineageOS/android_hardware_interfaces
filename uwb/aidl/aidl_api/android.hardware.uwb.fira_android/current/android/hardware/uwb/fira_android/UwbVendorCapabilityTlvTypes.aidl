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
  SUPPORTED_CHANNELS = 0,
  SUPPORTED_AOA_MODES = 1,
  SUPPORTED_DEVICE_ROLES = 2,
  SUPPORTS_BLOCK_STRIDING = 3,
  SUPPORTS_NON_DEFERRED_MODE = 4,
  SUPPORTS_ADAPTIVE_PAYLOAD_POWER = 5,
  INITIATION_TIME_MS = 6,
  SUPPORTED_MAC_FCS_CRC_TYPES = 7,
  SUPPORTED_MULTI_NODE_MODES = 8,
  SUPPORTED_PREAMBLE_MODES = 9,
  SUPPORTED_PRF_MODES = 10,
  SUPPORTED_RANGING_ROUND_USAGE_MODES = 11,
  SUPPORTED_RFRAME_MODES = 12,
  SUPPORTED_SFD_IDS = 13,
  SUPPORTED_STS_MODES = 14,
  SUPPORTED_STS_SEGEMENTS = 15,
  SUPPORTED_BPRF_PHR_DATA_RATES = 16,
  SUPPORTED_PSDU_DATA_RATES = 17,
  CCC_SUPPORTED_VERSIONS = 160,
  CCC_SUPPORTED_UWB_CONFIGS = 161,
  CCC_SUPPORTED_PULSE_SHAPE_COMBOS = 162,
  CCC_SUPPORTED_RAN_MULTIPLIER = 163,
  CCC_SUPPORTED_CHAPS_PER_SLOT = 164,
  CCC_SUPPORTED_SYNC_CODES = 165,
  CCC_SUPPORTED_CHANNELS = 166,
  CCC_SUPPORTED_HOPPING_SEQUENCES = 167,
  CCC_SUPPORTED_HOPPING_CONFIG_MODES = 168,
}
