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
enum StaIfaceReasonCode {
  UNSPECIFIED = 1,
  PREV_AUTH_NOT_VALID = 2,
  DEAUTH_LEAVING = 3,
  DISASSOC_DUE_TO_INACTIVITY = 4,
  DISASSOC_AP_BUSY = 5,
  CLASS2_FRAME_FROM_NONAUTH_STA = 6,
  CLASS3_FRAME_FROM_NONASSOC_STA = 7,
  DISASSOC_STA_HAS_LEFT = 8,
  STA_REQ_ASSOC_WITHOUT_AUTH = 9,
  PWR_CAPABILITY_NOT_VALID = 10,
  SUPPORTED_CHANNEL_NOT_VALID = 11,
  BSS_TRANSITION_DISASSOC = 12,
  INVALID_IE = 13,
  MICHAEL_MIC_FAILURE = 14,
  FOURWAY_HANDSHAKE_TIMEOUT = 15,
  GROUP_KEY_UPDATE_TIMEOUT = 16,
  IE_IN_4WAY_DIFFERS = 17,
  GROUP_CIPHER_NOT_VALID = 18,
  PAIRWISE_CIPHER_NOT_VALID = 19,
  AKMP_NOT_VALID = 20,
  UNSUPPORTED_RSN_IE_VERSION = 21,
  INVALID_RSN_IE_CAPAB = 22,
  IEEE_802_1X_AUTH_FAILED = 23,
  CIPHER_SUITE_REJECTED = 24,
  TDLS_TEARDOWN_UNREACHABLE = 25,
  TDLS_TEARDOWN_UNSPECIFIED = 26,
  SSP_REQUESTED_DISASSOC = 27,
  NO_SSP_ROAMING_AGREEMENT = 28,
  BAD_CIPHER_OR_AKM = 29,
  NOT_AUTHORIZED_THIS_LOCATION = 30,
  SERVICE_CHANGE_PRECLUDES_TS = 31,
  UNSPECIFIED_QOS_REASON = 32,
  NOT_ENOUGH_BANDWIDTH = 33,
  DISASSOC_LOW_ACK = 34,
  EXCEEDED_TXOP = 35,
  STA_LEAVING = 36,
  END_TS_BA_DLS = 37,
  UNKNOWN_TS_BA = 38,
  TIMEOUT = 39,
  PEERKEY_MISMATCH = 45,
  AUTHORIZED_ACCESS_LIMIT_REACHED = 46,
  EXTERNAL_SERVICE_REQUIREMENTS = 47,
  INVALID_FT_ACTION_FRAME_COUNT = 48,
  INVALID_PMKID = 49,
  INVALID_MDE = 50,
  INVALID_FTE = 51,
  MESH_PEERING_CANCELLED = 52,
  MESH_MAX_PEERS = 53,
  MESH_CONFIG_POLICY_VIOLATION = 54,
  MESH_CLOSE_RCVD = 55,
  MESH_MAX_RETRIES = 56,
  MESH_CONFIRM_TIMEOUT = 57,
  MESH_INVALID_GTK = 58,
  MESH_INCONSISTENT_PARAMS = 59,
  MESH_INVALID_SECURITY_CAP = 60,
  MESH_PATH_ERROR_NO_PROXY_INFO = 61,
  MESH_PATH_ERROR_NO_FORWARDING_INFO = 62,
  MESH_PATH_ERROR_DEST_UNREACHABLE = 63,
  MAC_ADDRESS_ALREADY_EXISTS_IN_MBSS = 64,
  MESH_CHANNEL_SWITCH_REGULATORY_REQ = 65,
  MESH_CHANNEL_SWITCH_UNSPECIFIED = 66,
}
