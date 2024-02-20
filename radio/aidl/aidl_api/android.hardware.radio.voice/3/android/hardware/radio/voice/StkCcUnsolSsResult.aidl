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

package android.hardware.radio.voice;
/* @hide */
@JavaDerive(toString=true) @VintfStability
parcelable StkCcUnsolSsResult {
  int serviceType;
  int requestType;
  int teleserviceType;
  int serviceClass;
  android.hardware.radio.RadioError result;
  android.hardware.radio.voice.SsInfoData[] ssInfo;
  android.hardware.radio.voice.CfData[] cfData;
  const int REQUEST_TYPE_ACTIVATION = 0;
  const int REQUEST_TYPE_DEACTIVATION = 1;
  const int REQUEST_TYPE_INTERROGATION = 2;
  const int REQUEST_TYPE_REGISTRATION = 3;
  const int REQUEST_TYPE_ERASURE = 4;
  const int SERVICE_TYPE_CFU = 0;
  const int SERVICE_TYPE_CF_BUSY = 1;
  const int SERVICE_TYPE_CF_NO_REPLY = 2;
  const int SERVICE_TYPE_CF_NOT_REACHABLE = 3;
  const int SERVICE_TYPE_CF_ALL = 4;
  const int SERVICE_TYPE_CF_ALL_CONDITIONAL = 5;
  const int SERVICE_TYPE_CLIP = 6;
  const int SERVICE_TYPE_CLIR = 7;
  const int SERVICE_TYPE_COLP = 8;
  const int SERVICE_TYPE_COLR = 9;
  const int SERVICE_TYPE_WAIT = 10;
  const int SERVICE_TYPE_BAOC = 11;
  const int SERVICE_TYPE_BAOIC = 12;
  const int SERVICE_TYPE_BAOIC_EXC_HOME = 13;
  const int SERVICE_TYPE_BAIC = 14;
  const int SERVICE_TYPE_BAIC_ROAMING = 15;
  const int SERVICE_TYPE_ALL_BARRING = 16;
  const int SERVICE_TYPE_OUTGOING_BARRING = 17;
  const int SERVICE_TYPE_INCOMING_BARRING = 18;
  const int TELESERVICE_TYPE_ALL_TELE_AND_BEARER_SERVICES = 0;
  const int TELESERVICE_TYPE_ALL_TELESEVICES = 1;
  const int TELESERVICE_TYPE_TELEPHONY = 2;
  const int TELESERVICE_TYPE_ALL_DATA_TELESERVICES = 3;
  const int TELESERVICE_TYPE_SMS_SERVICES = 4;
  const int TELESERVICE_TYPE_ALL_TELESERVICES_EXCEPT_SMS = 5;
  const int SUPP_SERVICE_CLASS_NONE = 0;
  const int SUPP_SERVICE_CLASS_VOICE = (1 << 0) /* 1 */;
  const int SUPP_SERVICE_CLASS_DATA = (1 << 1) /* 2 */;
  const int SUPP_SERVICE_CLASS_FAX = (1 << 2) /* 4 */;
  const int SUPP_SERVICE_CLASS_SMS = (1 << 3) /* 8 */;
  const int SUPP_SERVICE_CLASS_DATA_SYNC = (1 << 4) /* 16 */;
  const int SUPP_SERVICE_CLASS_DATA_ASYNC = (1 << 5) /* 32 */;
  const int SUPP_SERVICE_CLASS_PACKET = (1 << 6) /* 64 */;
  const int SUPP_SERVICE_CLASS_PAD = (1 << 7) /* 128 */;
  const int SUPP_SERVICE_CLASS_MAX = (1 << 7) /* 128 */;
}
