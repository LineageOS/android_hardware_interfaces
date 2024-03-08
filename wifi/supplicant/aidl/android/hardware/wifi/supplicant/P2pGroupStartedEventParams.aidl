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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.supplicant.P2pClientEapolIpAddressInfo;

/**
 * Parameters passed as part of Wifi P2P group start event.
 */
@VintfStability
parcelable P2pGroupStartedEventParams {
    /** Interface name of the group (For ex: p2p-p2p0-1). */
    String groupInterfaceName;

    /** Whether this device is owner of the group. */
    boolean isGroupOwner;

    /** SSID of the group. */
    byte[] ssid;

    /** Frequency in MHz on which this group is created. */
    int frequencyMHz;

    /** PSK used to secure the group. */
    byte[] psk;

    /** PSK passphrase used to secure the group. */
    String passphrase;

    /** Whether this group is persisted or not. */
    boolean isPersistent;

    /** MAC Address of the owner of this group. */
    byte[6] goDeviceAddress;

    /** MAC Address of the P2P interface of the owner of this group. */
    byte[6] goInterfaceAddress;

    /**
     * Flag to indicate that the P2P Client IP address is allocated via EAPOL exchange.
     */
    boolean isP2pClientEapolIpAddressInfoPresent;

    /**
     * The P2P Client IP Address allocated by the P2P Group Owner in EAPOL
     * key exchange.
     * Refer Wi-Fi P2P Technical Specification v1.7 - Section  4.2.8
     * "IP Address Allocation in EAPOL-Key Frames (4-Way Handshake)" for more details.
     * The value is undefined if isP2pClientEapolIpAddressInfoPresent is false.
     */
    P2pClientEapolIpAddressInfo p2pClientIpInfo;

    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
