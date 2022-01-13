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

package android.hardware.wifi.hostapd;

import android.hardware.wifi.hostapd.EncryptionType;

/**
 * Parameters to use for setting up the access point network.
 */
@VintfStability
parcelable NetworkParams {
    /**
     * SSID to set for the network
     */
    byte[] ssid;
    /**
     * Whether the network needs to be hidden or not.
     */
    boolean isHidden;
    /**
     * Key management mask for the replace encryptionType.
     */
    EncryptionType encryptionType;
    /**
     * Passphrase for WPA3_SAE network, WPA3_SAE_TRANSITION and WPA2_PSK.
     */
    String passphrase;
    /**
     * Enable the interworking service and set access network type to
     * CHARGEABLE_PUBLIC_NETWORK when set to true.
     */
    boolean isMetered;
    /**
     * Additional vendor specific elements for Beacon and Probe Response frames
     * This parameter can be used to add additional vendor specific element(s) into
     * the end of the Beacon and Probe Response frames. The format for these
     * element(s) is a binary dump of the raw information elements (id+len+payload for
     * one or more elements). Example: byte[]{ 221, 4, 17, 34, 51, 1 }
     */
    byte[] vendorElements;
}
