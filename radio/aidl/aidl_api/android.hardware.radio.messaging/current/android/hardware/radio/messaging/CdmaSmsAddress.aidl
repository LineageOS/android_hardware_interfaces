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

package android.hardware.radio.messaging;
/* @hide */
@JavaDerive(toString=true) @VintfStability
parcelable CdmaSmsAddress {
  int digitMode;
  boolean isNumberModeDataNetwork;
  int numberType;
  int numberPlan;
  byte[] digits;
  const int DIGIT_MODE_FOUR_BIT = 0;
  const int DIGIT_MODE_EIGHT_BIT = 1;
  const int NUMBER_PLAN_UNKNOWN = 0;
  const int NUMBER_PLAN_TELEPHONY = 1;
  const int NUMBER_PLAN_RESERVED_2 = 2;
  const int NUMBER_PLAN_DATA = 3;
  const int NUMBER_PLAN_TELEX = 4;
  const int NUMBER_PLAN_RESERVED_5 = 5;
  const int NUMBER_PLAN_RESERVED_6 = 6;
  const int NUMBER_PLAN_RESERVED_7 = 7;
  const int NUMBER_PLAN_RESERVED_8 = 8;
  const int NUMBER_PLAN_PRIVATE = 9;
  const int NUMBER_PLAN_RESERVED_10 = 10;
  const int NUMBER_PLAN_RESERVED_11 = 11;
  const int NUMBER_PLAN_RESERVED_12 = 12;
  const int NUMBER_PLAN_RESERVED_13 = 13;
  const int NUMBER_PLAN_RESERVED_14 = 14;
  const int NUMBER_PLAN_RESERVED_15 = 15;
  const int NUMBER_TYPE_UNKNOWN = 0;
  const int NUMBER_TYPE_INTERNATIONAL_OR_DATA_IP = 1;
  const int NUMBER_TYPE_NATIONAL_OR_INTERNET_MAIL = 2;
  const int NUMBER_TYPE_NETWORK = 3;
  const int NUMBER_TYPE_SUBSCRIBER = 4;
  const int NUMBER_TYPE_ALPHANUMERIC = 5;
  const int NUMBER_TYPE_ABBREVIATED = 6;
  const int NUMBER_TYPE_RESERVED_7 = 7;
}
