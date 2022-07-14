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

import android.hardware.wifi.IWifiRttControllerEventCallback;
import android.hardware.wifi.IWifiStaIface;
import android.hardware.wifi.MacAddress;
import android.hardware.wifi.RttCapabilities;
import android.hardware.wifi.RttConfig;
import android.hardware.wifi.RttLciInformation;
import android.hardware.wifi.RttLcrInformation;
import android.hardware.wifi.RttResponder;
import android.hardware.wifi.WifiChannelInfo;

/**
 * Interface used to perform RTT (Round trip time) operations.
 */
@VintfStability
interface IWifiRttController {
    /**
     * Disable RTT responder mode.
     *
     * @param cmdId Command Id corresponding to the original request.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void disableResponder(in int cmdId);

    /**
     * Enable RTT responder mode.
     *
     * @param cmdId Command Id to use for this invocation.
     * @parm channelHint Hint for the channel information where RTT responder
     *       must be enabled on.
     * @param maxDurationInSeconds Timeout of responder mode.
     * @param info Instance of |RttResponderInfo|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void enableResponder(in int cmdId, in WifiChannelInfo channelHint, in int maxDurationInSeconds,
            in RttResponder info);

    /**
     * Get the iface on which the RTT operations must be performed.
     *
     * @return AIDL interface object representing the iface if bound
     *         to a specific iface, null otherwise
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|
     */
    IWifiStaIface getBoundIface();

    /**
     * RTT capabilities of the device.
     *
     * @return Instance of |RttCapabilities|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    RttCapabilities getCapabilities();

    /**
     * Get RTT responder information (e.g. WiFi channel) to enable responder on.
     *
     * @return Instance of |RttResponderInfo|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    RttResponder getResponderInfo();

    /**
     * API to cancel RTT measurements.
     *
     * @param cmdId Command Id corresponding to the original request.
     * @param addrs Vector of addresses for which to cancel.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void rangeCancel(in int cmdId, in MacAddress[] addrs);

    /**
     * API to request RTT measurement.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param rttConfigs Vector of |RttConfig| parameters.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void rangeRequest(in int cmdId, in RttConfig[] rttConfigs);

    /**
     * Requests notifications of significant events on this RTT controller.
     * Multiple calls to this must register multiple callbacks, each of which
     * must receive all events.
     *
     * @param callback An instance of the |IWifiRttControllerEventCallback| AIDL
     *        interface object.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|
     */
    void registerEventCallback(in IWifiRttControllerEventCallback callback);

    /**
     * API to configure the LCI (Location civic information).
     * Used in RTT Responder mode only.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param lci Instance of |RttLciInformation|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void setLci(in int cmdId, in RttLciInformation lci);

    /**
     * API to configure the LCR (Location civic records).
     * Used in RTT Responder mode only.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param lcr Instance of |RttLcrInformation|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_RTT_CONTROLLER_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_NOT_AVAILABLE|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void setLcr(in int cmdId, in RttLcrInformation lcr);
}
