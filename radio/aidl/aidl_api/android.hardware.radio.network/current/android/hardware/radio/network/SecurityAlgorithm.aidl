/*
 * Copyright 2023 The Android Open Source Project
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
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum SecurityAlgorithm {
  A50 = 0,
  A51 = 1,
  A52 = 2,
  A53 = 3,
  A54 = 4,
  GEA0 = 14,
  GEA1 = 15,
  GEA2 = 16,
  GEA3 = 17,
  GEA4 = 18,
  GEA5 = 19,
  UEA0 = 29,
  UEA1 = 30,
  UEA2 = 31,
  EEA0 = 41,
  EEA1 = 42,
  EEA2 = 43,
  EEA3 = 44,
  NEA0 = 55,
  NEA1 = 56,
  NEA2 = 57,
  NEA3 = 58,
  SIP_NULL = 68,
  AES_GCM = 69,
  AES_GMAC = 70,
  AES_CBC = 71,
  DES_EDE3_CBC = 72,
  AES_EDE3_CBC = 73,
  HMAC_SHA1_96 = 74,
  HMAC_SHA1_96_null = 75,
  HMAC_MD5_96 = 76,
  HMAC_MD5_96_null = 77,
  SRTP_AES_COUNTER = 87,
  SRTP_AES_F8 = 88,
  SRTP_HMAC_SHA1 = 89,
  ENCR_AES_GCM_16 = 99,
  ENCR_AES_CBC = 100,
  AUTH_HMAC_SHA2_256_128 = 101,
  UNKNOWN = 113,
  OTHER = 114,
  ORYX = 124,
}
