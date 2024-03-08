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

import android.hardware.radio.RadioIndicationType;
import android.hardware.radio.messaging.CdmaSmsMessage;

/**
 * Interface declaring unsolicited radio indications for messaging APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioMessagingIndication {
    /**
     * Indicates when new CDMA SMS is received. Callee must subsequently confirm the receipt of the
     * SMS with acknowledgeLastIncomingCdmaSms(). Server must not send cdmaNewSms() messages until
     * acknowledgeLastIncomingCdmaSms() has been received.
     *
     * @param type Type of radio indication
     * @param msg Cdma Sms Message
     */
    void cdmaNewSms(in RadioIndicationType type, in CdmaSmsMessage msg);

    /**
     * Indicates that SMS storage on the RUIM is full. Messages cannot be saved on the RUIM until
     * space is freed.
     *
     * @param type Type of radio indication
     */
    void cdmaRuimSmsStorageFull(in RadioIndicationType type);

    /**
     * Indicates when new Broadcast SMS is received
     *
     * @param type Type of radio indication
     * @param data If received from GSM network, "data" is byte array of 88 bytes which indicates
     *        each page of a CBS Message sent to the MS by the BTS as coded in 3GPP 23.041 Section
     *        9.4.1.2. If received from UMTS network, "data" is byte array of 90 up to 1252 bytes
     *        which contain between 1 and 15 CBS Message pages sent as one packet to the MS by the
     *        BTS as coded in 3GPP 23.041 Section 9.4.2.2
     */
    void newBroadcastSms(in RadioIndicationType type, in byte[] data);

    /**
     * Indicates when new SMS is received. Callee must subsequently confirm the receipt of the SMS
     * with a acknowledgeLastIncomingGsmSms(). Server must not send newSms() or newSmsStatusReport()
     * messages until an acknowledgeLastIncomingGsmSms() has been received.
     *
     * @param type Type of radio indication
     * @param pdu PDU of SMS-DELIVER represented as byte array.
     *        The PDU starts with the SMSC address per TS 27.005 (+CMT:)
     */
    void newSms(in RadioIndicationType type, in byte[] pdu);

    /**
     * Indicates when new SMS has been stored on SIM card
     *
     * @param type Type of radio indication
     * @param recordNumber Record number on the sim
     */
    void newSmsOnSim(in RadioIndicationType type, in int recordNumber);

    /**
     * Indicates when new SMS Status Report is received. Callee must subsequently confirm the
     * receipt of the SMS with a acknowledgeLastIncomingGsmSms(). Server must not send newSms() or
     * newSmsStatusReport() messages until an acknowledgeLastIncomingGsmSms() has been received
     *
     * @param type Type of radio indication
     * @param pdu PDU of SMS-STATUS-REPORT represented as byte array.
     *        The PDU starts with the SMSC address per TS 27.005 (+CMT:)
     */
    void newSmsStatusReport(in RadioIndicationType type, in byte[] pdu);

    /**
     * Indicates that SMS storage on the SIM is full. Sent when the network attempts to deliver a
     * new SMS message. Messages cannot be saved on the SIM until space is freed. In particular,
     * incoming Class 2 messages must not be stored.
     *
     * @param type Type of radio indication
     */
    void simSmsStorageFull(in RadioIndicationType type);
}
