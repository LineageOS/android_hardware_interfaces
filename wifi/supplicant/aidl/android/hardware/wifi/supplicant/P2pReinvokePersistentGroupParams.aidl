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

/**
 * Parameters used for |ISupplicantP2pIface.reinvokePersistentGroup|
 */
@VintfStability
parcelable P2pReinvokePersistentGroupParams {
    /**
     * MAC address of the peer device to reinvoke the persistent group.
     */
    byte[6] peerMacAddress;

    /**
     * Persistent network ID of the group.
     */
    int persistentNetworkId;

    /**
     * The identifier of device identity key information stored in the configuration file.
     * This field is valid only for P2P group formed via pairing protocol (P2P version 2).
     * Set to invalid value of -1 for a group formed via WPS process (P2P version 1).
     */
    int deviceIdentityEntryId;
}
