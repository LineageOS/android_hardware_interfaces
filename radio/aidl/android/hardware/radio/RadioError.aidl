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

package android.hardware.radio;

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum RadioError {
    /**
     * Success
     */
    NONE = 0,
    /**
     * If radio did not start or is resetting
     */
    RADIO_NOT_AVAILABLE = 1,
    GENERIC_FAILURE = 2,
    /**
     * For PIN/PIN2 methods only
     */
    PASSWORD_INCORRECT = 3,
    /**
     * Operation requires SIM PIN2 to be entered
     */
    SIM_PIN2 = 4,
    /**
     * Operation requires SIM PUK2 to be entered
     */
    SIM_PUK2 = 5,
    /**
     * Optional API
     */
    REQUEST_NOT_SUPPORTED = 6,
    CANCELLED = 7,
    /**
     * Data ops are not allowed during voice call on a Class C GPRS device
     */
    OP_NOT_ALLOWED_DURING_VOICE_CALL = 8,
    /**
     * Data ops are not allowed before device registers in network
     */
    OP_NOT_ALLOWED_BEFORE_REG_TO_NW = 9,
    /**
     * Fail to send SMS and need to retry
     */
    SMS_SEND_FAIL_RETRY = 10,
    /**
     * Fail to set the location where CDMA subscription shall be retrieved because of SIM or
     * RUIM card absent
     */
    SIM_ABSENT = 11,
    /**
     * Fail to find CDMA subscription from specified location
     */
    SUBSCRIPTION_NOT_AVAILABLE = 12,
    /**
     * HW does not support preferred network type
     */
    MODE_NOT_SUPPORTED = 13,
    /**
     * Command failed becausee recipient is not on FDN list
     */
    FDN_CHECK_FAILURE = 14,
    /**
     * Network selection failed due to illegal SIM or ME
     */
    ILLEGAL_SIM_OR_ME = 15,
    /**
     * No logical channel available
     */
    MISSING_RESOURCE = 16,
    /**
     * Application not found on SIM
     */
    NO_SUCH_ELEMENT = 17,
    /**
     * DIAL request modified to USSD
     */
    DIAL_MODIFIED_TO_USSD = 18,
    /**
     * DIAL request modified to SS
     */
    DIAL_MODIFIED_TO_SS = 19,
    /**
     * DIAL request modified to DIAL with different data
     */
    DIAL_MODIFIED_TO_DIAL = 20,
    /**
     * USSD request modified to DIAL
     */
    USSD_MODIFIED_TO_DIAL = 21,
    /**
     * USSD request modified to SS
     */
    USSD_MODIFIED_TO_SS = 22,
    /**
     * USSD request modified to different USSD request
     */
    USSD_MODIFIED_TO_USSD = 23,
    /**
     * SS request modified to DIAL
     */
    SS_MODIFIED_TO_DIAL = 24,
    /**
     * SS request modified to USSD
     */
    SS_MODIFIED_TO_USSD = 25,
    /**
     * Subscription not supported by RIL
     */
    SUBSCRIPTION_NOT_SUPPORTED = 26,
    /**
     * SS request modified to different SS request
     */
    SS_MODIFIED_TO_SS = 27,
    /**
     * LCE service not supported(36 in RILConstants.java)
     */
    LCE_NOT_SUPPORTED = 36,
    /**
     * Not sufficieent memory to process the request
     */
    NO_MEMORY = 37,
    /**
     * Modem hit unexpected error scenario while handling this request
     */
    INTERNAL_ERR = 38,
    /**
     * Hit platform or system error
     */
    SYSTEM_ERR = 39,
    /**
     * Vendor RIL got unexpected or incorrect response from modem for this request
     */
    MODEM_ERR = 40,
    /**
     * Unexpected request for the current state
     */
    INVALID_STATE = 41,
    /**
     * Not sufficient resource to process the request
     */
    NO_RESOURCES = 42,
    /**
     * Received error from SIM card
     */
    SIM_ERR = 43,
    /**
     * Received invalid arguments in request
     */
    INVALID_ARGUMENTS = 44,
    /**
     * Cannot process the request in current SIM state
     */
    INVALID_SIM_STATE = 45,
    /**
     * Cannot process the request in current modem state
     */
    INVALID_MODEM_STATE = 46,
    /**
     * Received invalid call ID in request
     */
    INVALID_CALL_ID = 47,
    /**
     * ACK received when there is no SMS to ack
     */
    NO_SMS_TO_ACK = 48,
    /**
     * Received error from network. This generic error code should be used only when the error
     * cannot be mapped to other specific network error codes.
     */
    NETWORK_ERR = 49,
    /**
     * Operation denied due to overly-frequent requests
     */
    REQUEST_RATE_LIMITED = 50,
    /**
     * SIM is busy
     */
    SIM_BUSY = 51,
    /**
     * The target EF is full
     */
    SIM_FULL = 52,
    /**
     * Request is rejected by network
     */
    NETWORK_REJECT = 53,
    /**
     * Not allowed the request not
     */
    OPERATION_NOT_ALLOWED = 54,
    /**
     * The request record is empty
     */
    EMPTY_RECORD = 55,
    /**
     * Invalid SMS format
     */
    INVALID_SMS_FORMAT = 56,
    /**
     * Message not encoded properly
     */
    ENCODING_ERR = 57,
    /**
     * SMSC addrss specified is invalid
     */
    INVALID_SMSC_ADDRESS = 58,
    /**
     * No such entry present to perform the request
     */
    NO_SUCH_ENTRY = 59,
    /**
     * Network is not ready to perform the request
     */
    NETWORK_NOT_READY = 60,
    /**
     * Device does not have this value provisioned
     */
    NOT_PROVISIONED = 61,
    /**
     * Device does not have subscription
     */
    NO_SUBSCRIPTION = 62,
    /**
     * Network cannot be found
     */
    NO_NETWORK_FOUND = 63,
    /**
     * Operation cannot be performed because the device is currently in use
     */
    DEVICE_IN_USE = 64,
    /**
     * Operation aborted
     */
    ABORTED = 65,
    /**
     * Response from vendor had invalid data
     */
    INVALID_RESPONSE = 66,
    OEM_ERROR_1 = 501,
    OEM_ERROR_2 = 502,
    OEM_ERROR_3 = 503,
    OEM_ERROR_4 = 504,
    OEM_ERROR_5 = 505,
    OEM_ERROR_6 = 506,
    OEM_ERROR_7 = 507,
    OEM_ERROR_8 = 508,
    OEM_ERROR_9 = 509,
    OEM_ERROR_10 = 510,
    OEM_ERROR_11 = 511,
    OEM_ERROR_12 = 512,
    OEM_ERROR_13 = 513,
    OEM_ERROR_14 = 514,
    OEM_ERROR_15 = 515,
    OEM_ERROR_16 = 516,
    OEM_ERROR_17 = 517,
    OEM_ERROR_18 = 518,
    OEM_ERROR_19 = 519,
    OEM_ERROR_20 = 520,
    OEM_ERROR_21 = 521,
    OEM_ERROR_22 = 522,
    OEM_ERROR_23 = 523,
    OEM_ERROR_24 = 524,
    OEM_ERROR_25 = 525,
    /**
     * 1X voice and SMS are not allowed simulteneously.
     */
    SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED = 67,
    /**
     * Access is barred.
     */
    ACCESS_BARRED = 68,
    /**
     * SMS is blocked due to call control, e.g., resource unavailable
     * in the SMR entity.
     */
    BLOCKED_DUE_TO_CALL = 69,
    /**
     * Returned from setRadioPowerResponse when detecting RF HW issues. Some RF Front-End (RFFE)
     * components like antenna are considered critical for modem to provide telephony service.
     * This RadioError is used when modem detect such RFFE problem.
     */
    RF_HARDWARE_ISSUE = 70,
    /**
     * Returned from setRadioPowerResponse when detecting no RF calibration issue.
     * Unlike RF_HARDWARE_ISSUE, this is a SW problem and no HW repair is needed.
     */
    NO_RF_CALIBRATION_INFO = 71,
}
