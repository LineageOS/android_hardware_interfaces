/*
 * Copyright 2024 The Android Open Source Project
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

package android.hardware.bluetooth.socket;

/**
 * RFCOMM channel information
 */
@VintfStability
parcelable RfcommChannelInfo {
    /**
     * L2cap local channel ID for RFCOMM.
     */
    int localCid;

    /**
     * L2cap remote channel ID for RFCOMM.
     */
    int remoteCid;

    /**
     * Local Maximum Transmission Unit Size in bytes that the local L2CAP layer can receive.
     */
    int localMtu;

    /**
     * Remote Maximum Transmission Unit Size in bytes that the remote L2CAP layer can receive.
     */
    int remoteMtu;

    /**
     * Protocol initial credits at Rx path.
     *
     * The host stack will always set the initial credits to 0 when configuring the RFCOMM
     * channel, and this value will always be zero. It means offload stack should send initial
     * credits to peer device when IBluetoothSocket.opened() is successful.
     */
    int initialRxCredits;

    /**
     * Protocol initial credits at Tx path.
     */
    int initialTxCredits;

    /**
     * Data Link Connection Identifier (DLCI).
     */
    int dlci;

    /**
     * Maximum frame size negotiated during DLCI establishment.
     */
    int maxFrameSize;

    /**
     * Flag of whether the Android stack initiated the RFCOMM multiplexer control channel.
     *
     * This flag determines the value of the Command/Response (C/R) bit of RFCOMM frames.
     */
    boolean muxInitiator;
}
