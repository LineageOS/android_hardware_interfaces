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
enum KeyMgmtMask {
  WPA_EAP = (1 << 0) /* 1 */,
  WPA_PSK = (1 << 1) /* 2 */,
  NONE = (1 << 2) /* 4 */,
  IEEE8021X = (1 << 3) /* 8 */,
  FT_EAP = (1 << 5) /* 32 */,
  FT_PSK = (1 << 6) /* 64 */,
  OSEN = (1 << 15) /* 32768 */,
  WPA_EAP_SHA256 = (1 << 7) /* 128 */,
  WPA_PSK_SHA256 = (1 << 8) /* 256 */,
  SAE = (1 << 10) /* 1024 */,
  SUITE_B_192 = (1 << 17) /* 131072 */,
  OWE = (1 << 22) /* 4194304 */,
  DPP = (1 << 23) /* 8388608 */,
  WAPI_PSK = (1 << 12) /* 4096 */,
  WAPI_CERT = (1 << 13) /* 8192 */,
  FILS_SHA256 = (1 << 18) /* 262144 */,
  FILS_SHA384 = (1 << 19) /* 524288 */,
}
