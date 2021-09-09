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

import android.hardware.radio.ISapCallback;
import android.hardware.radio.SapApduType;
import android.hardware.radio.SapTransferProtocol;

/**
 * Empty top level interface.
 */
@VintfStability
interface ISap {
    /**
     * TRANSFER_APDU_REQ from SAP 1.1 spec 5.1.6
     *
     * @param token Id to match req-resp. Resp must include same token.
     * @param type APDU command type
     * @param command CommandAPDU/CommandAPDU7816 parameter depending on type
     */
    oneway void apduReq(in int token, in SapApduType type, in byte[] command);

    /**
     * CONNECT_REQ from SAP 1.1 spec 5.1.1
     *
     * @param token Id to match req-resp. Resp must include same token.
     * @param maxMsgSize MaxMsgSize to be used for SIM Access Profile connection
     */
    oneway void connectReq(in int token, in int maxMsgSize);

    /**
     * DISCONNECT_REQ from SAP 1.1 spec 5.1.3
     *
     * @param token Id to match req-resp. Resp must include same token.
     */
    oneway void disconnectReq(in int token);

    /**
     * POWER_SIM_OFF_REQ and POWER_SIM_ON_REQ from SAP 1.1 spec 5.1.10 + 5.1.12
     *
     * @param token Id to match req-resp. Resp must include same token.
     * @param state true for on, false for off
     */
    oneway void powerReq(in int token, in boolean state);

    /**
     * RESET_SIM_REQ from SAP 1.1 spec 5.1.14
     *
     * @param token Id to match req-resp. Resp must include same token.
     */
    oneway void resetSimReq(in int token);

    /**
     * Set callback that has response and unsolicited indication functions
     *
     * @param sapCallback Object containing response and unosolicited indication callbacks
     */
    void setCallback(in ISapCallback sapCallback);

    /**
     * SET_TRANSPORT_PROTOCOL_REQ from SAP 1.1 spec 5.1.20
     *
     * @param token Id to match req-resp. Resp must include same token.
     * @param transferProtocol Transport Protocol
     */
    oneway void setTransferProtocolReq(in int token, in SapTransferProtocol transferProtocol);

    /**
     * TRANSFER_ATR_REQ from SAP 1.1 spec 5.1.8
     *
     * @param token Id to match req-resp. Resp must include same token.
     */
    oneway void transferAtrReq(in int token);

    /**
     * TRANSFER_CARD_READER_STATUS_REQ from SAP 1.1 spec 5.1.17
     *
     * @param token Id to match req-resp. Resp must include same token.
     */
    oneway void transferCardReaderStatusReq(in int token);
}
