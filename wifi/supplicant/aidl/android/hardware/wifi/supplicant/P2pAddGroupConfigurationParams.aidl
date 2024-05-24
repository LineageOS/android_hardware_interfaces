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

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.supplicant.KeyMgmtMask;

/**
 * Request parameters used for |ISupplicantP2pIface.addGroupWithConfigurationParams|
 */
@VintfStability
parcelable P2pAddGroupConfigurationParams {
    /** The SSID of the group. */
    byte[] ssid;

    /** The passphrase used to secure the group. */
    String passphrase;

    /** Whether this group is persisted. Only applied on the group owner side */
    boolean isPersistent;

    /**
     * The required frequency or band of the group.
     *  Only applied on the group owner side.
     *  The following values are supported:
     *      0: automatic channel selection,
     *      2: for 2.4GHz channels
     *      5: for 5GHz channels
     *      6: for 6GHz channels
     *      specific frequency in MHz, i.e., 2412, 5500, etc.
     *        If an invalid band or unsupported frequency are specified,
     *        |ISupplicantP2pIface.addGroupWithConfigurationParams| fails
     */
    int frequencyMHzOrBand;

    /**
     * The MAC Address of the P2P interface of the Peer GO device.
     *        This field is valid only for the group client side.
     *        If the MAC is "00:00:00:00:00:00", the device must try to find a peer GO device
     *        whose network name matches the specified SSID.
     */
    byte[6] goInterfaceAddress;

    /*
     * True if join a group as a group client; false to create a group as a group owner
     */
    boolean joinExistingGroup;

    /**
     * The authentication Key management mask for the connection. Combination of |KeyMgmtMask|
     * values. The supported authentication key management types are WPA_PSK, SAE and PASN.
     *
     */
    int keyMgmtMask;

    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
