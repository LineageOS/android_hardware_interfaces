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

package android.hardware.wifi.supplicant;
@Backing(type="int") @VintfStability
enum P2pStatusCode {
  SUCCESS = 0,
  FAIL_INFO_CURRENTLY_UNAVAILABLE = 1,
  FAIL_INCOMPATIBLE_PARAMS = 2,
  FAIL_LIMIT_REACHED = 3,
  FAIL_INVALID_PARAMS = 4,
  FAIL_UNABLE_TO_ACCOMMODATE = 5,
  FAIL_PREV_PROTOCOL_ERROR = 6,
  FAIL_NO_COMMON_CHANNELS = 7,
  FAIL_UNKNOWN_GROUP = 8,
  FAIL_BOTH_GO_INTENT_15 = 9,
  FAIL_INCOMPATIBLE_PROV_METHOD = 10,
  FAIL_REJECTED_BY_USER = 11,
  SUCCESS_DEFERRED = 12,
}
