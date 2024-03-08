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
  UWB_CONFIG_0 = 0,
  UWB_CONFIG_1 = 1,
  PULSE_SHAPE_SYMMETRICAL_ROOT_RAISED_COSINE = 0,
  PULSE_SHAPE_PRECURSOR_FREE = 1,
  PULSE_SHAPE_PRECURSOR_FREE_SPECIAL = 2,
  CHAPS_PER_SLOT_3 = 1,
  CHAPS_PER_SLOT_4 = (1 << 1) /* 2 */,
  CHAPS_PER_SLOT_6 = (1 << 2) /* 4 */,
  CHAPS_PER_SLOT_8 = (1 << 3) /* 8 */,
  CHAPS_PER_SLOT_9 = (1 << 4) /* 16 */,
  CHAPS_PER_SLOT_12 = (1 << 5) /* 32 */,
  CHAPS_PER_SLOT_24 = (1 << 6) /* 64 */,
  HOPPING_SEQUENCE_DEFAULT = (1 << 4) /* 16 */,
  HOPPING_SEQUENCE_AES = (1 << 3) /* 8 */,
  HOPPING_CONFIG_MODE_NONE = (1 << 7) /* 128 */,
  HOPPING_CONFIG_MODE_CONTINUOUS = (1 << 6) /* 64 */,
  HOPPING_CONFIG_MODE_ADAPTIVE = (1 << 5) /* 32 */,
  CCC_CHANNEL_5 = 1,
  CCC_CHANNEL_9 = (1 << 1) /* 2 */,
  RADAR_NOT_SUPPORTED = 0,
  RADAR_SWEEP_SAMPLES_SUPPORTED = 1,
}
