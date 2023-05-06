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

package android.hardware.radio.network;
/* @hide */
@JavaDerive(toString=true) @VintfStability
parcelable BarringInfo {
  int serviceType;
  int barringType;
  @nullable android.hardware.radio.network.BarringTypeSpecificInfo barringTypeSpecificInfo;
  const int BARRING_TYPE_NONE = 0;
  const int BARRING_TYPE_CONDITIONAL = 1;
  const int BARRING_TYPE_UNCONDITIONAL = 2;
  const int SERVICE_TYPE_CS_SERVICE = 0;
  const int SERVICE_TYPE_PS_SERVICE = 1;
  const int SERVICE_TYPE_CS_VOICE = 2;
  const int SERVICE_TYPE_MO_SIGNALLING = 3;
  const int SERVICE_TYPE_MO_DATA = 4;
  const int SERVICE_TYPE_CS_FALLBACK = 5;
  const int SERVICE_TYPE_MMTEL_VOICE = 6;
  const int SERVICE_TYPE_MMTEL_VIDEO = 7;
  const int SERVICE_TYPE_EMERGENCY = 8;
  const int SERVICE_TYPE_SMS = 9;
  const int SERVICE_TYPE_OPERATOR_1 = 1001;
  const int SERVICE_TYPE_OPERATOR_2 = 1002;
  const int SERVICE_TYPE_OPERATOR_3 = 1003;
  const int SERVICE_TYPE_OPERATOR_4 = 1004;
  const int SERVICE_TYPE_OPERATOR_5 = 1005;
  const int SERVICE_TYPE_OPERATOR_6 = 1006;
  const int SERVICE_TYPE_OPERATOR_7 = 1007;
  const int SERVICE_TYPE_OPERATOR_8 = 1008;
  const int SERVICE_TYPE_OPERATOR_9 = 1009;
  const int SERVICE_TYPE_OPERATOR_10 = 1010;
  const int SERVICE_TYPE_OPERATOR_11 = 1011;
  const int SERVICE_TYPE_OPERATOR_12 = 1012;
  const int SERVICE_TYPE_OPERATOR_13 = 1013;
  const int SERVICE_TYPE_OPERATOR_14 = 1014;
  const int SERVICE_TYPE_OPERATOR_15 = 1015;
  const int SERVICE_TYPE_OPERATOR_16 = 1016;
  const int SERVICE_TYPE_OPERATOR_17 = 1017;
  const int SERVICE_TYPE_OPERATOR_18 = 1018;
  const int SERVICE_TYPE_OPERATOR_19 = 1019;
  const int SERVICE_TYPE_OPERATOR_20 = 1020;
  const int SERVICE_TYPE_OPERATOR_21 = 1021;
  const int SERVICE_TYPE_OPERATOR_22 = 1022;
  const int SERVICE_TYPE_OPERATOR_23 = 1023;
  const int SERVICE_TYPE_OPERATOR_24 = 1024;
  const int SERVICE_TYPE_OPERATOR_25 = 1025;
  const int SERVICE_TYPE_OPERATOR_26 = 1026;
  const int SERVICE_TYPE_OPERATOR_27 = 1027;
  const int SERVICE_TYPE_OPERATOR_28 = 1028;
  const int SERVICE_TYPE_OPERATOR_29 = 1029;
  const int SERVICE_TYPE_OPERATOR_30 = 1030;
  const int SERVICE_TYPE_OPERATOR_31 = 1031;
  const int SERVICE_TYPE_OPERATOR_32 = 1032;
}
