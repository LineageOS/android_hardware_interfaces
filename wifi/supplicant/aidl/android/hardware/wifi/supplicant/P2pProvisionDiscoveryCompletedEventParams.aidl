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
import android.hardware.wifi.supplicant.P2pProvDiscStatusCode;

/**
 * Parameters passed as a part of P2P provision discovery frame notification.
 */
@VintfStability
parcelable P2pProvisionDiscoveryCompletedEventParams {
    /**
     * P2P device interface MAC address of the device who sent the request or responded to our
     * request.
     */
    byte[6] p2pDeviceAddress;
    /** True if this is a request, false if this is a response. */
    boolean isRequest;
    /** Status of the provision discovery */
    P2pProvDiscStatusCode status;
    /**
     * Wi-Fi Protected Setup provisioning method. If using Wi-Fi Protected Setup,
     * then must be set to a non-|WpsProvisionMethod.NONE| provisioning method,
     * otherwise set to |WpsProvisionMethod.NONE|.
     */
    int configMethods;
    /** 8-digit pin generated */
    String generatedPin;
    /**
     * Interface name of this device group owner. (For ex: p2p-p2p0-1)
     * This field is filled only when the provision discovery request is received
     * with P2P Group ID attribute. i.e., when the peer device is joining this
     * device operating P2P group.
     * Refer to WFA Wi-Fi_Direct_Specification_v1.9 section 3.2.1 for more details.
     */
    String groupInterfaceName;
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
}
