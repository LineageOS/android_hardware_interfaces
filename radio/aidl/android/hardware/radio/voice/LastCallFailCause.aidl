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

package android.hardware.radio.voice;

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
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
    /**
     * STK Call Control
     */
    DIAL_MODIFIED_TO_USSD = 244,
    DIAL_MODIFIED_TO_SS = 245,
    DIAL_MODIFIED_TO_DIAL = 246,
    /**
     * Radio is off
     */
    RADIO_OFF = 247,
    /**
     * No cellular coverage
     */
    OUT_OF_SERVICE = 248,
    /**
     * No valid SIM is present
     */
    NO_VALID_SIM = 249,
    /**
     * Internal error at modem
     */
    RADIO_INTERNAL_ERROR = 250,
    /**
     * No response from network
     */
    NETWORK_RESP_TIMEOUT = 251,
    /**
     * Explicit network reject
     */
    NETWORK_REJECT = 252,
    /**
     * RRC connection failure. Eg.RACH
     */
    RADIO_ACCESS_FAILURE = 253,
    /**
     * Radio link failure
     */
    RADIO_LINK_FAILURE = 254,
    /**
     * Radio link lost due to poor coverage
     */
    RADIO_LINK_LOST = 255,
    /**
     * Radio uplink failure
     */
    RADIO_UPLINK_FAILURE = 256,
    /**
     * RRC connection setup failure
     */
    RADIO_SETUP_FAILURE = 257,
    /**
     * RRC connection release, normal
     */
    RADIO_RELEASE_NORMAL = 258,
    /**
     * RRC connection release, abnormal
     */
    RADIO_RELEASE_ABNORMAL = 259,
    /**
     * Access class barring
     */
    ACCESS_CLASS_BLOCKED = 260,
    /**
     * Explicit network detach
     */
    NETWORK_DETACH = 261,
    CDMA_LOCKED_UNTIL_POWER_CYCLE = 1000,
    CDMA_DROP = 1001,
    CDMA_INTERCEPT = 1002,
    CDMA_REORDER = 1003,
    CDMA_SO_REJECT = 1004,
    CDMA_RETRY_ORDER = 1005,
    CDMA_ACCESS_FAILURE = 1006,
    CDMA_PREEMPTED = 1007,
    /**
     * For non-emergency number dialed during emergency callback mode
     */
    CDMA_NOT_EMERGENCY = 1008,
    CDMA_ACCESS_BLOCKED = 1009,
    /**
     * OEM specific error codes. Used to distinguish error from
     * CALL_FAIL_ERROR_UNSPECIFIED and help assist debugging
     */
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
    /**
     * This error will be deprecated soon, vendor code must make sure to map error code to specific
     * error
     */
    ERROR_UNSPECIFIED = 0xffff,
}
