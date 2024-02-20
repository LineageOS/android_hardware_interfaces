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
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum LastCallFailCause {
  UNOBTAINABLE_NUMBER = 1,
  NO_ROUTE_TO_DESTINATION = 3,
  CHANNEL_UNACCEPTABLE = 6,
  OPERATOR_DETERMINED_BARRING = 8,
  NORMAL = 16,
  BUSY = 17,
  NO_USER_RESPONDING = 18,
  NO_ANSWER_FROM_USER = 19,
  CALL_REJECTED = 21,
  NUMBER_CHANGED = 22,
  PREEMPTION = 25,
  DESTINATION_OUT_OF_ORDER = 27,
  INVALID_NUMBER_FORMAT = 28,
  FACILITY_REJECTED = 29,
  RESP_TO_STATUS_ENQUIRY = 30,
  NORMAL_UNSPECIFIED = 31,
  CONGESTION = 34,
  NETWORK_OUT_OF_ORDER = 38,
  TEMPORARY_FAILURE = 41,
  SWITCHING_EQUIPMENT_CONGESTION = 42,
  ACCESS_INFORMATION_DISCARDED = 43,
  REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE = 44,
  RESOURCES_UNAVAILABLE_OR_UNSPECIFIED = 47,
  QOS_UNAVAILABLE = 49,
  REQUESTED_FACILITY_NOT_SUBSCRIBED = 50,
  INCOMING_CALLS_BARRED_WITHIN_CUG = 55,
  BEARER_CAPABILITY_NOT_AUTHORIZED = 57,
  BEARER_CAPABILITY_UNAVAILABLE = 58,
  SERVICE_OPTION_NOT_AVAILABLE = 63,
  BEARER_SERVICE_NOT_IMPLEMENTED = 65,
  ACM_LIMIT_EXCEEDED = 68,
  REQUESTED_FACILITY_NOT_IMPLEMENTED = 69,
  ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE = 70,
  SERVICE_OR_OPTION_NOT_IMPLEMENTED = 79,
  INVALID_TRANSACTION_IDENTIFIER = 81,
  USER_NOT_MEMBER_OF_CUG = 87,
  INCOMPATIBLE_DESTINATION = 88,
  INVALID_TRANSIT_NW_SELECTION = 91,
  SEMANTICALLY_INCORRECT_MESSAGE = 95,
  INVALID_MANDATORY_INFORMATION = 96,
  MESSAGE_TYPE_NON_IMPLEMENTED = 97,
  MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 98,
  INFORMATION_ELEMENT_NON_EXISTENT = 99,
  CONDITIONAL_IE_ERROR = 100,
  MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 101,
  RECOVERY_ON_TIMER_EXPIRED = 102,
  PROTOCOL_ERROR_UNSPECIFIED = 111,
  INTERWORKING_UNSPECIFIED = 127,
  CALL_BARRED = 240,
  FDN_BLOCKED = 241,
  IMSI_UNKNOWN_IN_VLR = 242,
  IMEI_NOT_ACCEPTED = 243,
  DIAL_MODIFIED_TO_USSD = 244,
  DIAL_MODIFIED_TO_SS = 245,
  DIAL_MODIFIED_TO_DIAL = 246,
  RADIO_OFF = 247,
  OUT_OF_SERVICE = 248,
  NO_VALID_SIM = 249,
  RADIO_INTERNAL_ERROR = 250,
  NETWORK_RESP_TIMEOUT = 251,
  NETWORK_REJECT = 252,
  RADIO_ACCESS_FAILURE = 253,
  RADIO_LINK_FAILURE = 254,
  RADIO_LINK_LOST = 255,
  RADIO_UPLINK_FAILURE = 256,
  RADIO_SETUP_FAILURE = 257,
  RADIO_RELEASE_NORMAL = 258,
  RADIO_RELEASE_ABNORMAL = 259,
  ACCESS_CLASS_BLOCKED = 260,
  NETWORK_DETACH = 261,
  CDMA_LOCKED_UNTIL_POWER_CYCLE = 1000,
  CDMA_DROP = 1001,
  CDMA_INTERCEPT = 1002,
  CDMA_REORDER = 1003,
  CDMA_SO_REJECT = 1004,
  CDMA_RETRY_ORDER = 1005,
  CDMA_ACCESS_FAILURE = 1006,
  CDMA_PREEMPTED = 1007,
  CDMA_NOT_EMERGENCY = 1008,
  CDMA_ACCESS_BLOCKED = 1009,
  OEM_CAUSE_1 = 0xf001,
  OEM_CAUSE_2 = 0xf002,
  OEM_CAUSE_3 = 0xf003,
  OEM_CAUSE_4 = 0xf004,
  OEM_CAUSE_5 = 0xf005,
  OEM_CAUSE_6 = 0xf006,
  OEM_CAUSE_7 = 0xf007,
  OEM_CAUSE_8 = 0xf008,
  OEM_CAUSE_9 = 0xf009,
  OEM_CAUSE_10 = 0xf00a,
  OEM_CAUSE_11 = 0xf00b,
  OEM_CAUSE_12 = 0xf00c,
  OEM_CAUSE_13 = 0xf00d,
  OEM_CAUSE_14 = 0xf00e,
  OEM_CAUSE_15 = 0xf00f,
  ERROR_UNSPECIFIED = 0xffff,
}
