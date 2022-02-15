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

import android.hardware.radio.messaging.CdmaBroadcastSmsConfigInfo;
import android.hardware.radio.messaging.CdmaSmsAck;
import android.hardware.radio.messaging.CdmaSmsMessage;
import android.hardware.radio.messaging.CdmaSmsWriteArgs;
import android.hardware.radio.messaging.GsmBroadcastSmsConfigInfo;
import android.hardware.radio.messaging.GsmSmsMessage;
import android.hardware.radio.messaging.IRadioMessagingIndication;
import android.hardware.radio.messaging.IRadioMessagingResponse;
import android.hardware.radio.messaging.ImsSmsMessage;
import android.hardware.radio.messaging.SmsAcknowledgeFailCause;
import android.hardware.radio.messaging.SmsWriteArgs;

/**
 * This interface is used by telephony and telecom to talk to cellular radio for messaging APIs.
 * All the functions have minimum one parameter:
 * serial: which corresponds to serial no. of request. Serial numbers must only be memorized for the
 * duration of a method call. If clients provide colliding serials (including passing the same
 * serial to different methods), multiple responses (one for each method call) must still be served.
 * setResponseFunctions must work with IRadioMessagingResponse and IRadioMessagingIndication.
 */
@VintfStability
oneway interface IRadioMessaging {
    /**
     * Acknowledge successful or failed receipt of SMS previously indicated via unsol
     * responseNewSms(), including acknowledgement TPDU to send as the RP-User-Data element of the
     * RP-ACK or RP-ERROR PDU.
     *
     * @param serial Serial number of request.
     * @param success true on successful receipt (send RP-ACK)
     *        false on failed receipt (send RP-ERROR)
     * @param ackPdu acknowledgement TPDU in hexadecimal format
     *
     * Response function is IRadioMessagingResponse.acknowledgeIncomingGsmSmsWithPduResponse()
     */
    void acknowledgeIncomingGsmSmsWithPdu(in int serial, in boolean success, in String ackPdu);

    /**
     * Acknowledge the success or failure in the receipt of SMS previously indicated
     * via responseCdmaNewSms()
     *
     * @param serial Serial number of request.
     * @param smsAck Cdma Sms ack to be sent described by CdmaSmsAck
     *
     * Response function is IRadioMessagingResponse.acknowledgeLastIncomingCdmaSmsResponse()
     */
    void acknowledgeLastIncomingCdmaSms(in int serial, in CdmaSmsAck smsAck);

    /**
     * Acknowledge successful or failed receipt of SMS previously indicated via unsolResponseNewSms
     *
     * @param serial Serial number of request.
     * @param success is true on successful receipt
     *        (basically, AT+CNMA=1 from TS 27.005 is 0 on failed receipt
     *        (basically, AT+CNMA=2 from TS 27.005)
     * @param cause: if success is false, this contains the failure cause as defined
     *        in TS 23.040, 9.2.3.22.
     *
     * Response function is IRadioMessagingResponse.acknowledgeLastIncomingGsmSmsResponse()
     */
    void acknowledgeLastIncomingGsmSms(
            in int serial, in boolean success, in SmsAcknowledgeFailCause cause);

    /**
     * Deletes a CDMA SMS message from RUIM memory.
     *
     * @param serial Serial number of request.
     * @param index record index of the message to delete
     *
     * Response function is IRadioMessagingResponse.deleteSmsOnRuimResponse()
     */
    void deleteSmsOnRuim(in int serial, in int index);

    /**
     * Deletes a SMS message from SIM memory.
     *
     * @param serial Serial number of request.
     * @param index Record index of the message to delete.
     *
     * Response function is IRadioMessagingResponse.deleteSmsOnSimResponse()
     */
    void deleteSmsOnSim(in int serial, in int index);

    /**
     * Request the setting of CDMA Broadcast SMS config
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioMessagingResponse.getCdmaBroadcastConfigResponse()
     */
    void getCdmaBroadcastConfig(in int serial);

    /**
     * Request the setting of GSM/WCDMA Cell Broadcast SMS config.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioMessagingResponse.getGsmBroadcastConfigResponse()
     */
    void getGsmBroadcastConfig(in int serial);

    /**
     * Get the default Short Message Service Center address on the device.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioMessagingResponse.getSmscAddressResponse()
     */
    void getSmscAddress(in int serial);

    /**
     * Indicates whether there is storage available for new SMS messages.
     *
     * @param serial Serial number of request.
     * @param available true if memory is available for storing new messages,
     *        false if memory capacity is exceeded
     *
     * Response function is IRadioMessagingResponse.reportSmsMemoryStatusResponse()
     */
    void reportSmsMemoryStatus(in int serial, in boolean available);

    /**
     * When response type received from a radio indication or radio response is
     * RadioIndicationType:UNSOLICITED_ACK_EXP or RadioResponseType:SOLICITED_ACK_EXP respectively,
     * acknowledge the receipt of those messages by sending responseAcknowledgement().
     */
    void responseAcknowledgement();

    /**
     * Send a CDMA SMS message
     *
     * @param serial Serial number of request.
     * @param sms CdmaSmsMessage to be sent
     *
     * Response function is IRadioMessagingResponse.sendCdmaSmsResponse()
     */
    void sendCdmaSms(in int serial, in CdmaSmsMessage sms);

    /**
     * Send an SMS message. Identical to sendCdmaSms, except that more messages are expected to be
     * sent soon.
     *
     * @param serial Serial number of request.
     * @param sms CdmaSmsMessage to be sent
     *
     * Response function is IRadioMessagingResponse.sendCdmaSmsExpectMoreResponse()
     */
    void sendCdmaSmsExpectMore(in int serial, in CdmaSmsMessage sms);

    /**
     * Send a SMS message over IMS. Based on the return error, caller decides to resend if sending
     * sms fails. SMS_SEND_FAIL_RETRY means retry, and other errors means no retry.
     * In case of retry, data is encoded based on Voice Technology available.
     *
     * @param serial Serial number of request.
     * @param message ImsSmsMessage to be sent
     *
     * Response function is IRadioMessagingResponse.sendImsSmsResponse()
     */
    void sendImsSms(in int serial, in ImsSmsMessage message);

    /**
     * Send an SMS message. Based on the returned error, caller decides to resend if sending sms
     * fails. RadioError:SMS_SEND_FAIL_RETRY means retry (i.e. error cause is 332) and
     * RadioError:GENERIC_FAILURE means no retry (i.e. error cause is 500)
     *
     * @param serial Serial number of request.
     * @param message GsmSmsMessage to be sent
     *
     * Response function is IRadioMessagingResponse.sendSmsResponse()
     */
    void sendSms(in int serial, in GsmSmsMessage message);

    /**
     * Send an SMS message. Identical to sendSms, except that more messages are expected to be sent
     * soon. If possible, keep SMS relay protocol link open (eg TS 27.005 AT+CMMS command).
     * Based on the return error, caller decides to resend if sending sms fails.
     * RadioError:SMS_SEND_FAIL_RETRY means retry (i.e. error cause is 332) and
     * RadioError:GENERIC_FAILURE means no retry (i.e. error cause is 500)
     *
     * @param serial Serial number of request.
     * @param message GsmSmsMessage to be sent
     *
     * Response function is IRadioMessagingResponse.sendSmsExpectMoreResponse()
     */
    void sendSmsExpectMore(in int serial, in GsmSmsMessage message);

    /**
     * Enable or disable the reception of CDMA Cell Broadcast SMS
     *
     * @param serial Serial number of request.
     * @param activate indicates to activate or turn off the reception of CDMA Cell Broadcast SMS.
     *        true = activate, false = turn off
     *
     * Response function is IRadioMessagingResponse.setCdmaBroadcastActivationResponse()
     */
    void setCdmaBroadcastActivation(in int serial, in boolean activate);

    /**
     * Set CDMA Broadcast SMS config
     *
     * @param serial Serial number of request.
     * @param configInfo CDMA Broadcast SMS config to be set.
     *
     * Response function is IRadioMessagingResponse.setCdmaBroadcastConfigResponse()
     */
    void setCdmaBroadcastConfig(in int serial, in CdmaBroadcastSmsConfigInfo[] configInfo);

    /**
     * Enable or disable the reception of GSM/WCDMA Cell Broadcast SMS
     *
     * @param serial Serial number of request.
     * @param activate indicates to activate or turn off the reception of GSM/WCDMA
     *        Cell Broadcast SMS. true = activate, false = turn off
     *
     * Response function is IRadioMessagingResponse.setGsmBroadcastActivationResponse()
     */
    void setGsmBroadcastActivation(in int serial, in boolean activate);

    /**
     * Set GSM/WCDMA Cell Broadcast SMS config
     *
     * @param serial Serial number of request.
     * @param configInfo Setting of GSM/WCDMA Cell broadcast config
     *
     * Response function is IRadioMessagingResponse.setGsmBroadcastConfigResponse()
     */
    void setGsmBroadcastConfig(in int serial, in GsmBroadcastSmsConfigInfo[] configInfo);

    /**
     * Set response functions for messaging radio requests and indications.
     *
     * @param radioMessagingResponse Object containing response functions
     * @param radioMessagingIndication Object containing radio indications
     */
    void setResponseFunctions(in IRadioMessagingResponse radioMessagingResponse,
            in IRadioMessagingIndication radioMessagingIndication);

    /**
     * Set the default Short Message Service Center address on the device.
     *
     * @param serial Serial number of request.
     * @param smsc Short Message Service Center address to set
     *
     * Response function is IRadioMessagingResponse.setSmscAddressResponse()
     */
    void setSmscAddress(in int serial, in String smsc);

    /**
     * Stores a CDMA SMS message to RUIM memory.
     *
     * @param serial Serial number of request.
     * @param cdmaSms CdmaSmsWriteArgs
     *
     * Response function is IRadioMessagingResponse.writeSmsToRuimResponse()
     */
    void writeSmsToRuim(in int serial, in CdmaSmsWriteArgs cdmaSms);

    /**
     * Stores a SMS message to SIM memory.
     *
     * @param serial Serial number of request.
     * @param smsWriteArgs SmsWriteArgs
     *
     * Response function is IRadioMessagingResponse.writeSmsToSimResponse()
     */
    void writeSmsToSim(in int serial, in SmsWriteArgs smsWriteArgs);
}
