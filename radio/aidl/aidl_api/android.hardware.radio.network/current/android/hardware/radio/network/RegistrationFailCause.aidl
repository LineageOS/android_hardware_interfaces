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
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum RegistrationFailCause {
  NONE = 0,
  IMSI_UNKNOWN_IN_HLR = 2,
  ILLEGAL_MS = 3,
  IMSI_UNKNOWN_IN_VLR = 4,
  IMEI_NOT_ACCEPTED = 5,
  ILLEGAL_ME = 6,
  GPRS_SERVICES_NOT_ALLOWED = 7,
  GPRS_AND_NON_GPRS_SERVICES_NOT_ALLOWED = 8,
  MS_IDENTITY_CANNOT_BE_DERIVED_BY_NETWORK = 9,
  IMPLICITLY_DETACHED = 10,
  PLMN_NOT_ALLOWED = 11,
  LOCATION_AREA_NOT_ALLOWED = 12,
  ROAMING_NOT_ALLOWED = 13,
  GPRS_SERVICES_NOT_ALLOWED_IN_PLMN = 14,
  NO_SUITABLE_CELLS = 15,
  MSC_TEMPORARILY_NOT_REACHABLE = 15,
  NETWORK_FAILURE = 17,
  MAC_FAILURE = 20,
  SYNC_FAILURE = 21,
  CONGESTION = 22,
  GSM_AUTHENTICATION_UNACCEPTABLE = 23,
  NOT_AUTHORIZED_FOR_THIS_CSG = 25,
  SMS_PROVIDED_BY_GPRS_IN_ROUTING_AREA,
  SERVICE_OPTION_NOT_SUPPORTED = 32,
  SERVICE_OPTION_NOT_SUBSCRIBED = 33,
  SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER = 34,
  CALL_CANNOT_BE_IDENTIFIED = 38,
  NO_PDP_CONTEXT_ACTIVATED = 40,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_1 = 48,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_2 = 49,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_3 = 50,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_4 = 51,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_5 = 52,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_6 = 53,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_7 = 54,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_8 = 55,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_9 = 56,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_10 = 57,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_11 = 58,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_12 = 59,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_13 = 60,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_14 = 61,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_15 = 62,
  RETRY_UPON_ENTRY_INTO_NEW_CELL_16 = 63,
  SEMANTICALLY_INCORRECT_MESSAGE = 95,
  INVALID_MANDATORY_INFORMATION = 96,
  MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED = 97,
  MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 98,
  INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED = 99,
  CONDITIONAL_IE_ERROR = 100,
  MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 101,
  PROTOCOL_ERROR_UNSPECIFIED = 111,
}
