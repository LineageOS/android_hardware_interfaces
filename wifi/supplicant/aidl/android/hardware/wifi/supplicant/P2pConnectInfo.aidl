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
     * MAC address of the device to connect to.
     */
    byte[6] peerAddress;

    /**
     * Provisioning method to use.
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
}
