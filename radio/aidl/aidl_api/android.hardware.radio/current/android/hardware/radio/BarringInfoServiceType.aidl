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
enum BarringInfoServiceType {
  CS_SERVICE = 0,
  PS_SERVICE = 1,
  CS_VOICE = 2,
  MO_SIGNALLING = 3,
  MO_DATA = 4,
  CS_FALLBACK = 5,
  MMTEL_VOICE = 6,
  MMTEL_VIDEO = 7,
  EMERGENCY = 8,
  SMS = 9,
  OPERATOR_1 = 1001,
  OPERATOR_2 = 1002,
  OPERATOR_3 = 1003,
  OPERATOR_4 = 1004,
  OPERATOR_5 = 1005,
  OPERATOR_6 = 1006,
  OPERATOR_7 = 1007,
  OPERATOR_8 = 1008,
  OPERATOR_9 = 1009,
  OPERATOR_10 = 1010,
  OPERATOR_11 = 1011,
  OPERATOR_12 = 1012,
  OPERATOR_13 = 1013,
  OPERATOR_14 = 1014,
  OPERATOR_15 = 1015,
  OPERATOR_16 = 1016,
  OPERATOR_17 = 1017,
  OPERATOR_18 = 1018,
  OPERATOR_19 = 1019,
  OPERATOR_20 = 1020,
  OPERATOR_21 = 1021,
  OPERATOR_22 = 1022,
  OPERATOR_23 = 1023,
  OPERATOR_24 = 1024,
  OPERATOR_25 = 1025,
  OPERATOR_26 = 1026,
  OPERATOR_27 = 1027,
  OPERATOR_28 = 1028,
  OPERATOR_29 = 1029,
  OPERATOR_30 = 1030,
  OPERATOR_31 = 1031,
  OPERATOR_32 = 1032,
}
