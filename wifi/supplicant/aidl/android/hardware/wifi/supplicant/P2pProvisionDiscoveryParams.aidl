/*
 * Copyright (C) 2024 The Android Open Source Project
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

import android.hardware.wifi.supplicant.WpsProvisionMethod;

/**
 * Parameters used for |ISupplicantP2pIfaceCallback.provisionDiscoveryWithParams|
 */
@VintfStability
parcelable P2pProvisionDiscoveryParams {
    /**
     * MAC address of the peer device to send the provision discovery request.
     */
    byte[6] peerMacAddress;

    /**
     * Wi-Fi Protected Setup provisioning method. If using Wi-Fi Protected Setup,
     * then must be set to a non-|WpsProvisionMethod.NONE| provisioning method,
     * otherwise set to |WpsProvisionMethod.NONE|.
     */
    WpsProvisionMethod provisionMethod;

    /**
     * Wi-Fi Direct pairing bootstrapping method. If using P2P pairing protocol,
     * then must be set one of the |P2pPairingBootstrappingMethodMask|, otherwise
     * set to zero.
     */
    int pairingBootstrappingMethod;
}
