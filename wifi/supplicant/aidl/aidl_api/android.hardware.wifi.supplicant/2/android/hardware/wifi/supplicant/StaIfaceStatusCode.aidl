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
enum StaIfaceStatusCode {
  SUCCESS = 0,
  UNSPECIFIED_FAILURE = 1,
  TDLS_WAKEUP_ALTERNATE = 2,
  TDLS_WAKEUP_REJECT = 3,
  SECURITY_DISABLED = 5,
  UNACCEPTABLE_LIFETIME = 6,
  NOT_IN_SAME_BSS = 7,
  CAPS_UNSUPPORTED = 10,
  REASSOC_NO_ASSOC = 11,
  ASSOC_DENIED_UNSPEC = 12,
  NOT_SUPPORTED_AUTH_ALG = 13,
  UNKNOWN_AUTH_TRANSACTION = 14,
  CHALLENGE_FAIL = 15,
  AUTH_TIMEOUT = 16,
  AP_UNABLE_TO_HANDLE_NEW_STA = 17,
  ASSOC_DENIED_RATES = 18,
  ASSOC_DENIED_NOSHORT = 19,
  SPEC_MGMT_REQUIRED = 22,
  PWR_CAPABILITY_NOT_VALID = 23,
  SUPPORTED_CHANNEL_NOT_VALID = 24,
  ASSOC_DENIED_NO_SHORT_SLOT_TIME = 25,
  ASSOC_DENIED_NO_HT = 27,
  R0KH_UNREACHABLE = 28,
  ASSOC_DENIED_NO_PCO = 29,
  ASSOC_REJECTED_TEMPORARILY = 30,
  ROBUST_MGMT_FRAME_POLICY_VIOLATION = 31,
  UNSPECIFIED_QOS_FAILURE = 32,
  DENIED_INSUFFICIENT_BANDWIDTH = 33,
  DENIED_POOR_CHANNEL_CONDITIONS = 34,
  DENIED_QOS_NOT_SUPPORTED = 35,
  REQUEST_DECLINED = 37,
  INVALID_PARAMETERS = 38,
  REJECTED_WITH_SUGGESTED_CHANGES = 39,
  INVALID_IE = 40,
  GROUP_CIPHER_NOT_VALID = 41,
  PAIRWISE_CIPHER_NOT_VALID = 42,
  AKMP_NOT_VALID = 43,
  UNSUPPORTED_RSN_IE_VERSION = 44,
  INVALID_RSN_IE_CAPAB = 45,
  CIPHER_REJECTED_PER_POLICY = 46,
  TS_NOT_CREATED = 47,
  DIRECT_LINK_NOT_ALLOWED = 48,
  DEST_STA_NOT_PRESENT = 49,
  DEST_STA_NOT_QOS_STA = 50,
  ASSOC_DENIED_LISTEN_INT_TOO_LARGE = 51,
  INVALID_FT_ACTION_FRAME_COUNT = 52,
  INVALID_PMKID = 53,
  INVALID_MDIE = 54,
  INVALID_FTIE = 55,
  REQUESTED_TCLAS_NOT_SUPPORTED = 56,
  INSUFFICIENT_TCLAS_PROCESSING_RESOURCES = 57,
  TRY_ANOTHER_BSS = 58,
  GAS_ADV_PROTO_NOT_SUPPORTED = 59,
  NO_OUTSTANDING_GAS_REQ = 60,
  GAS_RESP_NOT_RECEIVED = 61,
  STA_TIMED_OUT_WAITING_FOR_GAS_RESP = 62,
  GAS_RESP_LARGER_THAN_LIMIT = 63,
  REQ_REFUSED_HOME = 64,
  ADV_SRV_UNREACHABLE = 65,
  REQ_REFUSED_SSPN = 67,
  REQ_REFUSED_UNAUTH_ACCESS = 68,
  INVALID_RSNIE = 72,
  U_APSD_COEX_NOT_SUPPORTED = 73,
  U_APSD_COEX_MODE_NOT_SUPPORTED = 74,
  BAD_INTERVAL_WITH_U_APSD_COEX = 75,
  ANTI_CLOGGING_TOKEN_REQ = 76,
  FINITE_CYCLIC_GROUP_NOT_SUPPORTED = 77,
  CANNOT_FIND_ALT_TBTT = 78,
  TRANSMISSION_FAILURE = 79,
  REQ_TCLAS_NOT_SUPPORTED = 80,
  TCLAS_RESOURCES_EXCHAUSTED = 81,
  REJECTED_WITH_SUGGESTED_BSS_TRANSITION = 82,
  REJECT_WITH_SCHEDULE = 83,
  REJECT_NO_WAKEUP_SPECIFIED = 84,
  SUCCESS_POWER_SAVE_MODE = 85,
  PENDING_ADMITTING_FST_SESSION = 86,
  PERFORMING_FST_NOW = 87,
  PENDING_GAP_IN_BA_WINDOW = 88,
  REJECT_U_PID_SETTING = 89,
  REFUSED_EXTERNAL_REASON = 92,
  REFUSED_AP_OUT_OF_MEMORY = 93,
  REJECTED_EMERGENCY_SERVICE_NOT_SUPPORTED = 94,
  QUERY_RESP_OUTSTANDING = 95,
  REJECT_DSE_BAND = 96,
  TCLAS_PROCESSING_TERMINATED = 97,
  TS_SCHEDULE_CONFLICT = 98,
  DENIED_WITH_SUGGESTED_BAND_AND_CHANNEL = 99,
  MCCAOP_RESERVATION_CONFLICT = 100,
  MAF_LIMIT_EXCEEDED = 101,
  MCCA_TRACK_LIMIT_EXCEEDED = 102,
  DENIED_DUE_TO_SPECTRUM_MANAGEMENT = 103,
  ASSOC_DENIED_NO_VHT = 104,
  ENABLEMENT_DENIED = 105,
  RESTRICTION_FROM_AUTHORIZED_GDB = 106,
  AUTHORIZATION_DEENABLED = 107,
  FILS_AUTHENTICATION_FAILURE = 112,
  UNKNOWN_AUTHENTICATION_SERVER = 113,
}
