/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;
/* @hide */
@Backing(type="int") @VintfStability
enum FrontendAnalogSifStandard {
  UNDEFINED = 0,
  AUTO = (1 << 0) /* 1 */,
  BG = (1 << 1) /* 2 */,
  BG_A2 = (1 << 2) /* 4 */,
  BG_NICAM = (1 << 3) /* 8 */,
  I = (1 << 4) /* 16 */,
  DK = (1 << 5) /* 32 */,
  DK1_A2 = (1 << 6) /* 64 */,
  DK2_A2 = (1 << 7) /* 128 */,
  DK3_A2 = (1 << 8) /* 256 */,
  DK_NICAM = (1 << 9) /* 512 */,
  L = (1 << 10) /* 1024 */,
  M = (1 << 11) /* 2048 */,
  M_BTSC = (1 << 12) /* 4096 */,
  M_A2 = (1 << 13) /* 8192 */,
  M_EIAJ = (1 << 14) /* 16384 */,
  I_NICAM = (1 << 15) /* 32768 */,
  L_NICAM = (1 << 16) /* 65536 */,
  L_PRIME = (1 << 17) /* 131072 */,
}
