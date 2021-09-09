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

package android.hardware.radio;
@Backing(type="int") @VintfStability
enum RadioCdmaSmsConst {
  ADDRESS_MAX = 36,
  SUBADDRESS_MAX = 36,
  BEARER_DATA_MAX = 255,
  UDH_MAX_SND_SIZE = 128,
  UDH_EO_DATA_SEGMENT_MAX = 131,
  MAX_UD_HEADERS = 7,
  USER_DATA_MAX = 229,
  UDH_LARGE_PIC_SIZE = 128,
  UDH_SMALL_PIC_SIZE = 32,
  UDH_VAR_PIC_SIZE = 134,
  UDH_ANIM_NUM_BITMAPS = 4,
  UDH_LARGE_BITMAP_SIZE = 32,
  UDH_SMALL_BITMAP_SIZE = 8,
  UDH_OTHER_SIZE = 226,
  IP_ADDRESS_SIZE = 4,
}
