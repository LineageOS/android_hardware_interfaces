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

/**
 * Parameters passed as a part of P2P peer client joined event.
 */
@VintfStability
parcelable P2pPeerClientJoinedEventParams {
    /** Interface name of this device group owner. (For ex: p2p-p2p0-1) */
    String groupInterfaceName;

    /** P2P group interface MAC address of the client that joined. */
    byte[6] clientInterfaceAddress;

    /** P2P device interface MAC address of the client that joined. */
    byte[6] clientDeviceAddress;

    /**
     * The P2P Client IPV4 address allocated via EAPOL exchange.
     * The higher-order address bytes are in the lower-order int bytes
     * (e.g. 1.2.3.4 is represented as 0x04030201).
     * Refer Wi-Fi P2P Technical Specification v1.7 - Section  4.2.8
     * "IP Address Allocation in EAPOL-Key Frames (4-Way Handshake)" for more details.
     * The value is set to zero if the IP address is not allocated via EAPOL exchange.
     */
    int clientIpAddress;

    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
