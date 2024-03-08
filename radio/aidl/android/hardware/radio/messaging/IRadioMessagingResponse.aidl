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

package android.hardware.radio.messaging;

import android.hardware.radio.RadioResponseInfo;
import android.hardware.radio.messaging.CdmaBroadcastSmsConfigInfo;
import android.hardware.radio.messaging.GsmBroadcastSmsConfigInfo;
import android.hardware.radio.messaging.SendSmsResult;

/**
 * Interface declaring response functions to solicited radio requests for messaging APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioMessagingResponse {
    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void acknowledgeIncomingGsmSmsWithPduResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_SMS_TO_ACK
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INTERNAL_ERR
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void acknowledgeLastIncomingCdmaSmsResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void acknowledgeLastIncomingGsmSmsResponse(in RadioResponseInfo info);

    /**
     * Acknowledge the receipt of radio request sent to the vendor. This must be sent only for
     * radio request which take long time to respond. For more details, refer
     * https://source.android.com/devices/tech/connect/ril.html
     *
     * @param serial Serial no. of the request whose acknowledgement is sent.
     */
    void acknowledgeRequest(in int serial);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_SUCH_ENTRY
     *   RadioError:INTERNAL_ERR
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:SIM_ABSENT
     */
    void deleteSmsOnRuimResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_FULL
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_SUCH_ENTRY
     *   RadioError:INTERNAL_ERR
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:SIM_ABSENT
     */
    void deleteSmsOnSimResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param configs Vector of CDMA Broadcast SMS configs.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void getCdmaBroadcastConfigResponse(
            in RadioResponseInfo info, in CdmaBroadcastSmsConfigInfo[] configs);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param configs Vector of GSM/WCDMA Cell broadcast configs
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void getGsmBroadcastConfigResponse(
            in RadioResponseInfo info, in GsmBroadcastSmsConfigInfo[] configs);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param smsc Short Message Service Center address on the device
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NOT_PROVISIONED
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void getSmscAddressResponse(in RadioResponseInfo info, in String smsc);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_STATE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void reportSmsMemoryStatusResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Response to sms sent as defined by SendSmsResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:ENCODING_ERR
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     *   RadioError:SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED
     *   RadioError:ACCESS_BARRED
     *   RadioError:BLOCKED_DUE_TO_CALL
     */
    void sendCdmaSmsExpectMoreResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Sms result struct as defined by SendSmsResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:ENCODING_ERR
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:ENCODING_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     *   RadioError:SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED
     *   RadioError:ACCESS_BARRED
     *   RadioError:BLOCKED_DUE_TO_CALL
     */
    void sendCdmaSmsResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Response to sms sent as defined by SendSmsResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.ims is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:ENCODING_ERR
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void sendImsSmsResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Response to sms sent as defined by SendSmsResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:ENCODING_ERR
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     *   RadioError:ACCESS_BARRED
     *   RadioError:BLOCKED_DUE_TO_CALL
     */
    void sendSmsExpectMoreResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Response to sms sent as defined by SendSmsResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:ENCODING_ERR
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     *   RadioError:ACCESS_BARRED
     *   RadioError:BLOCKED_DUE_TO_CALL
     */
    void sendSmsResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void setCdmaBroadcastActivationResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void setCdmaBroadcastConfigResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void setGsmBroadcastActivationResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void setGsmBroadcastConfigResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:INTERNAL_ERR
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void setSmscAddressResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param index record index where the cmda sms message is stored
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SIM_FULL
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:ENCODING_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:SYSTEM_ERR
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:SIM_ABSENT
     */
    void writeSmsToRuimResponse(in RadioResponseInfo info, in int index);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param index record index where the message is stored
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.messaging is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_FULL
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:ENCODING_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SYSTEM_ERR
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:SIM_ABSENT
     */
    void writeSmsToSimResponse(in RadioResponseInfo info, in int index);
}
