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

package android.hardware.wifi;

import android.hardware.wifi.NanIdentityResolutionAttribute;
import android.hardware.wifi.NanPairingRequestType;
import android.hardware.wifi.common.OuiKeyedData;

/**
 * NAN pairing request indication message structure.
 * Event indication received by an intended Responder when a
 * pairing request is initiated by an Initiator. See Wi-Fi Aware R4.0 section 7.6.1.3
 */
@VintfStability
parcelable NanPairingRequestInd {
    /**
     * Discovery session (publish or subscribe) ID of a previously created discovery session. The
     * pairing request is received in the context of this discovery session.
     * NAN Spec: Service Descriptor Attribute (SDA) / Instance ID
     */
    byte discoverySessionId;
    /**
     * A unique ID of the peer. Can be subsequently used in |IWifiNanIface.transmitFollowupRequest|
     * or to set up a data-path.
     */
    int peerId;
    /**
     * MAC address of the Initiator peer. This is the MAC address of the peer's
     * management/discovery NAN interface.
     */
    byte[6] peerDiscMacAddr;
    /**
     * ID of the NAN pairing Used to identify the pairing in further negotiation/APIs.
     */
    int pairingInstanceId;
    /**
     * Indicate the pairing session is of setup or verification
     */
    NanPairingRequestType requestType;
    /**
     * Whether should cache the negotiated NIK/NPK for future verification
     */
    boolean enablePairingCache;
    /**
     * The NIRA from peer for NAN pairing verification
     */
    NanIdentityResolutionAttribute peerNira;
    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
