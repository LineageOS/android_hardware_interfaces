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
 * Parameters passed as a part of P2P peer client disconnected event.
 */
@VintfStability
parcelable P2pPeerClientDisconnectedEventParams {
    /** Interface name of this device group owner. (For ex: p2p-p2p0-1) */
    String groupInterfaceName;

    /** P2P group interface MAC address of the client that disconnected. */
    byte[6] clientInterfaceAddress;

    /** P2P device interface MAC address of the client that disconnected. */
    byte[6] clientDeviceAddress;

    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
