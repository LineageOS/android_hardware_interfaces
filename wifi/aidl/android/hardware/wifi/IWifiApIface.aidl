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

package android.hardware.wifi;

import android.hardware.wifi.WifiBand;

/**
 * Represents a network interface in AP mode.
 *
 * This can be obtained through |IWifiChip.getApIface|.
 */
@VintfStability
interface IWifiApIface {
    /**
     * Get the name of this interface.
     *
     * @return Name of this interface.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|
     */
    String getName();

    /**
     * Get the names of the bridged AP instances.
     *
     * @return Vector containing the names of the bridged AP
     *         instances. Note: Returns an empty vector for a non-bridged AP.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    String[] getBridgedInstances();

    /**
     * Gets the factory MAC address of the interface.
     *
     * @return Factory MAC address of the interface.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    byte[6] getFactoryMacAddress();

    /**
     * Set country code for this iface.
     *
     * @param code 2 byte country code (as defined in ISO 3166) to set.
     * @return status Status of the operation.
     *         Possible status codes:
     *         |WifiStatusCode.SUCCESS|,
     *         |WifiStatusCode.FAILURE_UNKNOWN|,
     *         |WifiStatusCode.FAILURE_IFACE_INVALID|
     */
    void setCountryCode(in byte[2] code);

    /**
     * Reset all of the AP interfaces' MAC address to the factory MAC address.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void resetToFactoryMacAddress();

    /**
     * Changes the MAC address of the interface to the given MAC address.
     *
     * @param mac MAC address to change to.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void setMacAddress(in byte[6] mac);
}
