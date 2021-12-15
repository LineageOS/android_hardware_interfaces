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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.IfaceType;
import android.hardware.wifi.supplicant.MacAddress;

/**
 * Interface exposed by the supplicant for each P2P mode network
 * configuration it controls.
 */
@VintfStability
interface ISupplicantP2pNetwork {
    /**
     * Get the BSSID set for this network.
     *
     * @return bssid Value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] getBssid();

    /**
     * Get the list of P2P Clients in a persistent group (GO).
     * This is a list of P2P Clients (P2P Device Address) that have joined
     * the persistent group. This is maintained on the GO for persistent
     * group entries (disabled == 2).
     *
     * @return MAC addresses of the clients.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantP2ptusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    MacAddress[] getClientList();

    /**
     * Retrieves the ID allocated to this network by the supplicant.
     *
     * This is not the |SSID| of the network, but an internal identifier for
     * this network used by the supplicant.
     *
     * @return Network ID.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    int getId();

    /**
     * Retrieves the name of the interface this network belongs to.
     *
     * @return Name of the network interface, e.g., wlan0
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    String getInterfaceName();

    /**
     * Getters for the various network params.
     *
     *
     * Get SSID for this network.
     *
     * @return ssid value set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] getSsid();

    /**
     * Retrieves the type of the interface this network belongs to.
     *
     * @return Type of the network interface, e.g., STA.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    IfaceType getType();

    /**
     * Check if the network is currently active one.
     *
     * @return true if current, false otherwise.
     * @throws ServiceSpecificException with one of the following values:,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    boolean isCurrent();

    /**
     * Check if the device is the group owner of the network.
     *
     * @return true if group owner, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    boolean isGroupOwner();

    /**
     * Check if the network is marked persistent.
     *
     * @return true if persistent, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    boolean isPersistent();

    /**
     * Set the list of P2P Clients in a persistent group (GO).
     * This is a list of P2P Clients (P2P Device Address) that have joined
     * the persistent group. This is maintained on the GO for persistent
     * group entries (disabled == 2).
     *
     * @param clients MAC address of the clients.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantP2ptusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setClientList(in MacAddress[] clients);
}
