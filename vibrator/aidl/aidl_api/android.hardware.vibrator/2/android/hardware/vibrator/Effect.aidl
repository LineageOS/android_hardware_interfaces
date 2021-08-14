/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hardware.vibrator;
@Backing(type="int") @VintfStability
enum Effect {
  CLICK = 0,
  DOUBLE_CLICK = 1,
  TICK = 2,
  THUD = 3,
  POP = 4,
  HEAVY_CLICK = 5,
  RINGTONE_1 = 6,
  RINGTONE_2 = 7,
  RINGTONE_3 = 8,
  RINGTONE_4 = 9,
  RINGTONE_5 = 10,
  RINGTONE_6 = 11,
  RINGTONE_7 = 12,
  RINGTONE_8 = 13,
  RINGTONE_9 = 14,
  RINGTONE_10 = 15,
  RINGTONE_11 = 16,
  RINGTONE_12 = 17,
  RINGTONE_13 = 18,
  RINGTONE_14 = 19,
  RINGTONE_15 = 20,
  TEXTURE_TICK = 21,
}
