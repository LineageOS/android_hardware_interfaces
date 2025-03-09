/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.supplicant.WpsProvisionMethod;

/**
 * Request parameters used for |ISupplicantP2pIface.connectWithParams|
 */
@VintfStability
parcelable P2pConnectInfo {
    /**
     * MAC address of the peer device to connect to or to authorize a connect
     * request.
     */
    byte[6] peerAddress;

    /**
     * Wi-Fi Protected Setup provisioning method. If using Wi-Fi Protected Setup,
     * then must be set to a non-|WpsProvisionMethod.NONE| provisioning method,
     * otherwise set to |WpsProvisionMethod.NONE|.
     */
    WpsProvisionMethod provisionMethod;

    /**
     * Pin to be used, if |provisionMethod| uses one of the
     * preselected |PIN*| methods.
     */
    String preSelectedPin;

    /**
     * Indicates that this is a command to join an existing group as a client.
     * This means that the group owner negotiation step can be skipped.
     * This must send a Provision Discovery Request message to the
     * target group owner before associating for WPS provisioning.
     */
    boolean joinExistingGroup;

    /**
     * Used to request a persistent group to be formed.
     */
    boolean persistent;

    /**
     * Used to override the default Intent for this group owner
     * negotiation (Values from 1-15). Refer to section 4.1.6 in
     * Wi-Fi Peer-to-Peer (P2P) Technical Specification Version 1.7.
     */
    int goIntent;

    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;

    /**
     * Wi-Fi Direct pairing bootstrapping method. If using P2P pairing protocol,
     * then must be set one of the |P2pPairingBootstrappingMethodMask|, otherwise
     * set to zero.
     */
    int pairingBootstrappingMethod;

    /**
     * Password for pairing setup, if |bootstrappingMethod| uses one of the
     * |P2pPairingBootstrappingMethodMask| methods other than
     * P2pPairingBootstrappingMethodMask.BOOTSTRAPPING_OPPORTUNISTIC,
     * null otherwise.
     */
    @nullable String password;

    /**
     * Channel frequency in MHz to start group formation,
     * join an existing group owner or authorize a connection request.
     */
    int frequencyMHz;

    /**
     * Used to authorize a connection request from the Peer device.
     * The MAC address of the peer device is set in peerAddress.
     */
    boolean authorizeConnectionFromPeer;

    /**
     * Used to check if the authorize connection request is on an existing Group Owner
     * interface to allow a peer device to connect. This field is set to null if the request
     * is to form a group or join an existing group.
     */
    @nullable String groupInterfaceName;
}
