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

package android.hardware.radio.network;

/**
 * Call fail causes for Circuit-switched service enumerated in 3GPP TS 24.008, 10.5.3.6 and
 * 10.5.147. Additional detail is available in 3GPP TS 24.008 Annex G.
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum RegistrationFailCause {
    /**
     * 0 - None
     */
    NONE = 0,
    /**
     * 2 - IMSI unknown in HLR
     */
    IMSI_UNKNOWN_IN_HLR = 2,
    /**
     * 3 - Illegal MS
     */
    ILLEGAL_MS = 3,
    /**
     * 4 - Illegal ME
     */
    IMSI_UNKNOWN_IN_VLR = 4,
    /**
     * 5 - PLMN not allowed
     */
    IMEI_NOT_ACCEPTED = 5,
    /**
     * 6 - Location area not allowed
     */
    ILLEGAL_ME = 6,
    /**
     * 7 - Roaming not allowed
     */
    GPRS_SERVICES_NOT_ALLOWED = 7,
    /**
     * 8 - No Suitable Cells in this Location Area
     */
    GPRS_AND_NON_GPRS_SERVICES_NOT_ALLOWED = 8,
    /**
     * 9 - Network failure
     */
    MS_IDENTITY_CANNOT_BE_DERIVED_BY_NETWORK = 9,
    /**
     * 10 - Persistent location update reject
     */
    IMPLICITLY_DETACHED = 10,
    /**
     * 11 - PLMN not allowed
     */
    PLMN_NOT_ALLOWED = 11,
    /**
     * 12 - Location area not allowed
     */
    LOCATION_AREA_NOT_ALLOWED = 12,
    /**
     * 13 - Roaming not allowed in this Location Area
     */
    ROAMING_NOT_ALLOWED = 13,
    /**
     * 14 - GPRS Services not allowed in this PLMN
     */
    GPRS_SERVICES_NOT_ALLOWED_IN_PLMN = 14,
    /**
     * 15 - No Suitable Cells in this Location Area
     */
    NO_SUITABLE_CELLS = 15,
    /**
     * 16 - MSC temporarily not reachable
     */
    MSC_TEMPORARILY_NOT_REACHABLE = 15,
    /**
     * 17 - Network Failure
     */
    NETWORK_FAILURE = 17,
    /**
     * 20 - MAC Failure
     */
    MAC_FAILURE = 20,
    /**
     * 21 - Sync Failure
     */
    SYNC_FAILURE = 21,
    /**
     * 22 - Congestion
     */
    CONGESTION = 22,
    /**
     * 23 - GSM Authentication unacceptable
     */
    GSM_AUTHENTICATION_UNACCEPTABLE = 23,
    /**
     * 25 - Not Authorized for this CSG
     */
    NOT_AUTHORIZED_FOR_THIS_CSG = 25,
    /**
     * 28 SMS provided via GPRS in this routing area
     */
    SMS_PROVIDED_BY_GPRS_IN_ROUTING_AREA,
    /**
     * 32 - Service option not supported
     */
    SERVICE_OPTION_NOT_SUPPORTED = 32,
    /**
     * 33 - Requested service option not subscribed
     */
    SERVICE_OPTION_NOT_SUBSCRIBED = 33,
    /**
     * 34 - Service option temporarily out of order
     */
    SERVICE_OPTION_TEMPORARILY_OUT_OF_ORDER = 34,
    /**
     * 38 - Call cannot be identified
     */
    CALL_CANNOT_BE_IDENTIFIED = 38,
    /**
     * 40 No PDP context activated
     */
    NO_PDP_CONTEXT_ACTIVATED = 40,
    /**
     * 48-63 - Retry upon entry into a new cell
     */
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
    /**
     * 95 - Semantically incorrect message
     */
    SEMANTICALLY_INCORRECT_MESSAGE = 95,
    /**
     * 96 - Invalid mandatory information
     */
    INVALID_MANDATORY_INFORMATION = 96,
    /**
     * 97 - Message type non-existent or not implemented
     */
    MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED = 97,
    /**
     * 98 - Message type not compatible with protocol state
     */
    MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 98,
    /**
     * 99 - Information element non-existent or not implemented
     */
    INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED = 99,
    /**
     * 100 - Conditional IE error
     */
    CONDITIONAL_IE_ERROR = 100,
    /**
     * 101 - Message not compatible with protocol state
     */
    MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 101,
    /**
     * 111 - Protocol error, unspecified
     */
    PROTOCOL_ERROR_UNSPECIFIED = 111,
}
