/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.radio.sap;

import android.hardware.radio.sap.ISapCallback;
import android.hardware.radio.sap.SapApduType;
import android.hardware.radio.sap.SapTransferProtocol;

@VintfStability
oneway interface ISap {
    /**
     * TRANSFER_APDU_REQ from SAP 1.1 spec 5.1.6
     *
     * @param serial Id to match req-resp. Resp must include same serial.
     * @param type APDU command type
     * @param command CommandAPDU/CommandAPDU7816 parameter depending on type
     */
    void apduReq(in int serial, in SapApduType type, in byte[] command);

    /**
     * CONNECT_REQ from SAP 1.1 spec 5.1.1
     *
     * @param serial Id to match req-resp. Resp must include same serial.
     * @param maxMsgSizeBytes MaxMsgSize to be used for SIM Access Profile connection
     */
    void connectReq(in int serial, in int maxMsgSizeBytes);

    /**
     * DISCONNECT_REQ from SAP 1.1 spec 5.1.3
     *
     * @param serial Id to match req-resp. Resp must include same serial.
     */
    void disconnectReq(in int serial);

    /**
     * POWER_SIM_OFF_REQ and POWER_SIM_ON_REQ from SAP 1.1 spec 5.1.10 + 5.1.12
     *
     * @param serial Id to match req-resp. Resp must include same serial.
     * @param powerOn true for on, false for off
     */
    void powerReq(in int serial, in boolean powerOn);

    /**
     * RESET_SIM_REQ from SAP 1.1 spec 5.1.14
     *
     * @param serial Id to match req-resp. Resp must include same serial.
     */
    void resetSimReq(in int serial);

    /**
     * Set callback that has response and unsolicited indication functions
     *
     * @param sapCallback Object containing response and unosolicited indication callbacks
     */
    void setCallback(in ISapCallback sapCallback);

    /**
     * SET_TRANSPORT_PROTOCOL_REQ from SAP 1.1 spec 5.1.20
     *
     * @param serial Id to match req-resp. Resp must include same serial.
     * @param transferProtocol Transport Protocol
     */
    void setTransferProtocolReq(in int serial, in SapTransferProtocol transferProtocol);

    /**
     * TRANSFER_ATR_REQ from SAP 1.1 spec 5.1.8
     *
     * @param serial Id to match req-resp. Resp must include same serial.
     */
    void transferAtrReq(in int serial);

    /**
     * TRANSFER_CARD_READER_STATUS_REQ from SAP 1.1 spec 5.1.17
     *
     * @param serial Id to match req-resp. Resp must include same serial.
     */
    void transferCardReaderStatusReq(in int serial);
}
