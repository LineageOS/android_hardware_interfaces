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
@Backing(type="long") @VintfStability
enum UwbVendorCapabilityTlvValues {
  HAS_AZIMUTH_SUPPORT = 1,
  HAS_ELEVATION_SUPPORT = 2,
  HAS_FOM_SUPPORT = 4,
  HAS_FULL_AZIMUTH_SUPPORT = 8,
  HAS_CONTROLEE_INITIATOR_SUPPORT = 1,
  HAS_CONTROLEE_RESPONDER_SUPPORT = 2,
  HAS_CONTROLLER_INITIATOR_SUPPORT = 4,
  HAS_CONTROLLER_RESPONDER_SUPPORT = 8,
  HAS_CRC_16_SUPPORT = 1,
  HAS_CRC_32_SUPPORT = 2,
  HAS_UNICAST_SUPPORT = 1,
  HAS_ONE_TO_MANY_SUPPORT = 2,
  HAS_MANY_TO_MANY_SUPPORT = 4,
  SUPPORTED_PREAMBLE_HAS_32_SYMBOLS_SUPPORT = 1,
  SUPPORTED_PREAMBLE_HAS_64_SYMBOLS_SUPPORT = 2,
  HAS_BPRF_SUPPORT = 1,
  HAS_HPRF_SUPPORT = 2,
  HAS_DS_TWR_SUPPORT = 1,
  HAS_SS_TWR_SUPPORT = 2,
  HAS_SP0_RFRAME_SUPPORT = 1,
  HAS_SP1_RFRAME_SUPPORT = 2,
  HAS_SP3_RFRAME_SUPPORT = 8,
  HAS_SFD0_SUPPORT = 1,
  HAS_SFD1_SUPPORT = 2,
  HAS_SFD2_SUPPORT = 4,
  HAS_SFD3_SUPPORT = 8,
  HAS_SFD4_SUPPORT = 16,
  HAS_STATIC_STS_SUPPORT = 1,
  HAS_DYNAMIC_STS_SUPPORT = 2,
  HAS_DYNAMIC_STS_INDIVIDUAL_CONTROLEE_KEY_SUPPORT = 4,
  HAS_0_SEGMENT_SUPPORT = 1,
  HAS_1_SEGMENT_SUPPORT = 2,
  HAS_2_SEGMENT_SUPPORT = 4,
  HAS_6M81_SUPPORT = 1,
  HAS_850K_SUPPORT = 2,
  HAS_27M2_SUPPORT = 4,
  HAS_31M2_SUPPORT = 8,
  UWB_CONFIG_0 = 0,
  UWB_CONFIG_1 = 1,
  PULSE_SHAPE_SYMMETRICAL_ROOT_RAISED_COSINE = 1,
  PULSE_SHAPE_PRECURSOR_FREE = 2,
  PULSE_SHAPE_PRECURSOR_FREE_SPECIAL = 3,
  CHAPS_PER_SLOT_3 = 3,
  CHAPS_PER_SLOT_4 = 4,
  CHAPS_PER_SLOT_6 = 6,
  CHAPS_PER_SLOT_8 = 8,
  CHAPS_PER_SLOT_9 = 9,
  CHAPS_PER_SLOT_12 = 12,
  CHAPS_PER_SLOT_24 = 24,
  HOPPING_SEQUENCE_DEFAULT = 0,
  HOPPING_SEQUENCE_AES = 1,
  HOPPING_CONFIG_MODE_NONE = 0,
  HOPPING_CONFIG_MODE_CONTINUOUS = 1,
  HOPPING_CONFIG_MODE_ADAPTIVE = 2,
}
