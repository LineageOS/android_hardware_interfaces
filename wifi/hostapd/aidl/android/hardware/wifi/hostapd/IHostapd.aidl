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

import android.hardware.wifi.hostapd.BandMask;
import android.hardware.wifi.hostapd.ChannelParams;
import android.hardware.wifi.hostapd.DebugLevel;
import android.hardware.wifi.hostapd.HwModeParams;
import android.hardware.wifi.hostapd.IHostapdCallback;
import android.hardware.wifi.hostapd.Ieee80211ReasonCode;
import android.hardware.wifi.hostapd.IfaceParams;
import android.hardware.wifi.hostapd.NetworkParams;

/**
 * Top-level interface for managing SoftAPs.
 */
@VintfStability
interface IHostapd {
    /**
     * Adds a new access point for hostapd to control.
     *
     * This should trigger the setup of an access point with the specified
     * interface and network params.
     *
     * @param ifaceParams AccessPoint Params for the access point.
     * @param nwParams Network Params for the access point.
     * @throws ServiceSpecificException with one of the following values:
     *         |HostapdStatusCode.FAILURE_ARGS_INVALID|,
     *         |HostapdStatusCode.FAILURE_UNKNOWN|,
     *         |HostapdStatusCode.FAILURE_IFACE_EXISTS|
     */
    void addAccessPoint(in IfaceParams ifaceParams, in NetworkParams nwParams);

    /**
     * Force one of the hotspot clients to disconnect.
     *
     * @param ifaceName Name of the interface.
     * @param clientAddress MAC Address of the hotspot client.
     * @param reasonCode One of disconnect reason code defined by 802.11.
     * @throws ServiceSpecificException with one of the following values:
     *         |HostapdStatusCode.FAILURE_IFACE_UNKNOWN|,
     *         |HostapdStatusCode.FAILURE_CLIENT_UNKNOWN|
     */
    void forceClientDisconnect(
            in String ifaceName, in byte[] clientAddress, in Ieee80211ReasonCode reasonCode);

    /**
     * Register for callbacks from the hostapd service.
     *
     * These callbacks are invoked for global events that are not specific
     * to any interface or network. Registration of multiple callback
     * objects is supported. These objects must be deleted when the corresponding
     * client process is dead.
     *
     * @param callback An instance of the |IHostapdCallback| AIDL interface
     *     object.
     * @throws ServiceSpecificException with one of the following values:
     *     |HostapdStatusCode.FAILURE_UNKNOWN|
     */
    void registerCallback(in IHostapdCallback callback);

    /**
     * Removes an existing access point from hostapd.
     *
     * This must bring down the access point previously set up on the
     * interface.
     *
     * @param ifaceName Name of the interface.
     * @throws ServiceSpecificException with one of the following values:
     *         |HostapdStatusCode.FAILURE_UNKNOWN|,
     *         |HostapdStatusCode.FAILURE_IFACE_UNKNOWN|
     */
    void removeAccessPoint(in String ifaceName);

    /**
     * Set debug parameters for the hostapd.
     *
     * @param level Debug logging level for the hostapd.
     *        (one of |DebugLevel| values).
     * @throws ServiceSpecificException with one of the following values:
     *         |HostapdStatusCode.FAILURE_UNKNOWN|
     */
    void setDebugParams(in DebugLevel level);

    /**
     * Terminate the service.
     * This must de-register the service and clear all states. If this HAL
     * supports the lazy HAL protocol, then this may trigger daemon to exit and
     * wait to be restarted.
     */
    oneway void terminate();

    /**
     * Removes an existing link from multiple link device which the current AP resides on.
     *
     * The multiple link device is an access point which was added by |IHostapd.addAccessPoint| with
     * |IfaceParams.usesMlo| set to true.
     *
     * @param ifaceName Name of the interface which was added by |IHostapd.addAccessPoint|
     * @param linkIdentity the identity of the link which associated to the multiple link device
     * that the current AP resides on. The link identity should be one of the identities provided in
     * |IfaceParams.instanceIdentities| when this iface was created.
     * @throws ServiceSpecificException with one of the following values:
     *         |HostapdStatusCode.FAILURE_UNKNOWN|,
     *         |HostapdStatusCode.FAILURE_ARGS_INVALID| when the linkIdentity mis-matches one of the
     *             identities provided in |IfaceParams.instanceIdentities|.
     *         |HostapdStatusCode.FAILURE_IFACE_UNKNOWN| when current existing AP isn't using mlo.
     */
    void removeLinkFromMultipleLinkBridgedApIface(in String ifaceName, in String linkIdentity);
}
