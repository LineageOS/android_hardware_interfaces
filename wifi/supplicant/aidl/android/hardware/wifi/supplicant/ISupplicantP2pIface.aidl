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

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.supplicant.FreqRange;
import android.hardware.wifi.supplicant.ISupplicantP2pIfaceCallback;
import android.hardware.wifi.supplicant.ISupplicantP2pNetwork;
import android.hardware.wifi.supplicant.IfaceType;
import android.hardware.wifi.supplicant.MiracastMode;
import android.hardware.wifi.supplicant.P2pConnectInfo;
import android.hardware.wifi.supplicant.P2pDiscoveryInfo;
import android.hardware.wifi.supplicant.P2pFrameTypeMask;
import android.hardware.wifi.supplicant.P2pGroupCapabilityMask;
import android.hardware.wifi.supplicant.WpsConfigMethods;
import android.hardware.wifi.supplicant.WpsProvisionMethod;

/**
 * Interface exposed by the supplicant for each P2P mode network
 * interface (e.g p2p0) it controls.
 */
@VintfStability
interface ISupplicantP2pIface {
    /**
     * This command can be used to add a bonjour service.
     *
     * @param query Hex dump of the query data.
     * @param return Hex dump of the response data.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void addBonjourService(in byte[] query, in byte[] response);

    /**
     * Set up a P2P group owner manually (i.e., without group owner
     * negotiation with a specific peer). This is also known as autonomous
     * group owner. Optional |persistentNetworkId| may be used to specify
     * restart of a persistent group.
     *
     * @param persistent Used to request a persistent group to be formed.
     * @param persistentNetworkId Used to specify the restart of a persistent
     *        group. Set to UINT32_MAX for a non-persistent group.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void addGroup(in boolean persistent, in int persistentNetworkId);

    /**
     * Set up a P2P group owner or join a group as a group client
     * with the specified configuration.
     *
     * If joinExistingGroup is false, this device sets up a P2P group owner manually (i.e.,
     * without group owner negotiation with a specific peer) with the specified SSID,
     * passphrase, persistent mode, and frequency/band.
     *
     * If joinExistingGroup is true, this device acts as a group client and joins the group
     * whose network name and group owner's MAC address matches the specified SSID
     * and peer address without WPS process. If peerAddress is 00:00:00:00:00:00, the first found
     * group whose network name matches the specified SSID is joined.
     *
     * @param ssid The SSID of this group.
     * @param pskPassphrase The passphrase of this group.
     * @param persistent Used to request a persistent group to be formed,
     *        only applied for the group owner.
     * @param freq The required frequency or band for this group.
     *        only applied for the group owner.
     *        The following values are supported:
     *        0: automatic channel selection,
     *        2: for 2.4GHz channels
     *        5: for 5GHz channels
     *        specific frequency, i.e., 2412, 5500, etc.
     *        If an invalid band or unsupported frequency are specified, it fails.
     * @param peerAddress the group owner's MAC address, only applied for the group client.
     *        If the MAC is "00:00:00:00:00:00", the device must try to find a peer
     *        whose network name matches the specified SSID.
     * @param joinExistingGroup if true, join a group as a group client; otherwise,
     *        create a group as a group owner.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void addGroupWithConfig(in byte[] ssid, in String pskPassphrase, in boolean persistent,
            in int freq, in byte[] peerAddress, in boolean joinExistingGroup);

    /**
     * Add a new network to the interface.
     *
     * @return AIDL interface object representing the new network if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    @PropagateAllowBlocking ISupplicantP2pNetwork addNetwork();

    /**
     * This command can be used to add a UPNP service.
     *
     * @param version Version to be used.
     * @package serviceName Service name to be used.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void addUpnpService(in int version, in String serviceName);

    /**
     * Cancel an ongoing P2P group formation and joining-a-group related
     * operation. This operation unauthorizes the specific peer device (if any
     * had been authorized to start group formation), stops P2P find (if in
     * progress), stops pending operations for join-a-group, and removes the
     * P2P group interface (if one was used) that is in the WPS provisioning
     * step. If the WPS provisioning step has been completed, the group is not
     * terminated.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NOT_STARTED|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void cancelConnect();

    /**
     * Cancel a previous service discovery request.
     *
     * @param identifier Identifier for the request to cancel.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NOT_STARTED|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void cancelServiceDiscovery(in long identifier);

    /**
     * Cancel any ongoing WPS operations.
     *
     * @param groupIfName Group interface name to use.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void cancelWps(in String groupIfName);

    /**
     * Configure Extended Listen Timing.
     *
     * If enabled, listen state must be entered every |intervalInMillis| for at
     * least |periodInMillis|. Both values have acceptable range of 1-65535
     * (with interval obviously having to be larger than or equal to duration).
     * If the P2P module is not idle at the time the Extended Listen Timing
     * timeout occurs, the Listen State operation must be skipped.
     *
     * @param periodInMillis Period in milliseconds.
     * @param intervalInMillis Interval in milliseconds.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void configureExtListen(in int periodInMillis, in int intervalInMillis);

    /**
     * Start P2P group formation with a discovered P2P peer. This includes
     * optional group owner negotiation, group interface setup, provisioning,
     * and establishing data connection.
     * <p>
     * @deprecated This method is deprecated from AIDL v3, newer HALs should use
     * connectWithParams.
     *
     * @param peerAddress MAC address of the device to connect to.
     * @param provisionMethod Provisioning method to use.
     * @param preSelectedPin Pin to be used, if |provisionMethod| uses one of the
     *        preselected |PIN*| methods.
     * @param joinExistingGroup Indicates that this is a command to join an
     *        existing group as a client. It skips the group owner negotiation
     *        part. This must send a Provision Discovery Request message to the
     *        target group owner before associating for WPS provisioning.
     * @param persistent Used to request a persistent group to be formed.
     * @param goIntent Used to override the default Intent for this group owner
     *        negotiation (Values from 1-15). Refer to section 4.1.6 in
     *        Wi-Fi Peer-to-Peer (P2P) Technical Specification Version 1.7.
     * @return Pin generated, if |provisionMethod| uses one of the
     *         generated |PIN*| methods.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    String connect(in byte[] peerAddress, in WpsProvisionMethod provisionMethod,
            in String preSelectedPin, in boolean joinExistingGroup, in boolean persistent,
            in int goIntent);

    /**
     * Creates a NFC handover request message.
     *
     * @return Bytes representing the handover request as specified in
     *         section 3.1.1 of NFC Connection Handover 1.2 Technical
     *         Specification.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    byte[] createNfcHandoverRequestMessage();

    /**
     * Creates a NFC handover select message.
     *
     * @return Bytes representing the handover select as specified in
     *         section 3.1.2 of NFC Connection Handover 1.2 Technical
     *         Specification.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    byte[] createNfcHandoverSelectMessage();

    /**
     * Enable/Disable Wifi Display.
     *
     * @param enable true to enable, false to disable.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void enableWfd(in boolean enable);

    /**
     * Initiate a P2P service discovery with an optional timeout.
     * <p>
     * @deprecated This method is deprecated from AIDL v3, newer HALs should use
     * findWithParams.
     *
     * @param timeoutInSec Max time to be spent is performing discovery.
     *        Set to 0 to indefinitely continue discovery until an explicit
     *        |stopFind| is sent.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void find(in int timeoutInSec);

    /**
     * Flush P2P peer table and state.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void flush();

    /**
     * This command can be used to flush all services from the
     * device.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void flushServices();

    /**
     * Gets the MAC address of the device.
     *
     * @return MAC address of the device.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    byte[] getDeviceAddress();

    /**
     * Get whether EDMG(802.11ay) is enabled for this network.
     *
     * @return true if set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    boolean getEdmg();

    /**
     * Gets the capability of the group which the device is a
     * member of.
     *
     * @param peerAddress MAC address of the peer.
     * @return Combination of |P2pGroupCapabilityMask| values.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    P2pGroupCapabilityMask getGroupCapability(in byte[] peerAddress);

    /**
     * Retrieves the name of the network interface.
     *
     * @return Name of the network interface, e.g., wlan0
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    String getName();

    /**
     * Gets an AIDL interface object for the network corresponding to the
     * network id.
     *
     * Use |ISupplicantP2pNetwork.getId()| on the corresponding network AIDL
     * interface object to retrieve the ID.
     *
     * @param id Network ID allocated to the corresponding network.
     * @return AIDL interface object representing the new network if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_UNKNOWN|
     */
    @PropagateAllowBlocking ISupplicantP2pNetwork getNetwork(in int id);

    /**
     * Gets the operational SSID of the device.
     *
     * @param peerAddress MAC address of the peer.
     * @return SSID of the device
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    byte[] getSsid(in byte[] peerAddress);

    /**
     * Retrieves the type of the network interface.
     *
     * @return Type of the network interface, e.g., STA.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    IfaceType getType();

    /**
     * Invite a device to a persistent group.
     * If the peer device is the group owner of the persistent group, the peer
     * parameter is not needed. Otherwise it is used to specify which
     * device to invite. |goDeviceAddress| parameter may be used to override
     * the group owner device address for Invitation Request should it not be
     * known for some reason (this should not be needed in most cases).
     *
     * @param groupIfName Group interface name to use.
     * @param goDeviceAddress MAC address of the group owner device.
     * @param peerAddress MAC address of the device to invite.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void invite(in String groupIfName, in byte[] goDeviceAddress, in byte[] peerAddress);

    /**
     * Retrieve a list of all the network Id's controlled by the supplicant.
     *
     * The corresponding |ISupplicantP2pNetwork| object for any network can be
     * retrieved using the |getNetwork| method.
     *
     * @return List of all network Id's controlled by the supplicant.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    int[] listNetworks();

    /**
     * Send P2P provision discovery request to the specified peer. The
     * parameters for this command are the P2P device address of the peer and the
     * desired configuration method.
     *
     * @param peerAddress MAC address of the device to send discovery.
     * @method provisionMethod Provisioning method to use.
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void provisionDiscovery(in byte[] peerAddress, in WpsProvisionMethod provisionMethod);

    /**
     * Register for callbacks from this interface.
     *
     * These callbacks are invoked for events that are specific to this interface.
     * Registration of multiple callback objects is supported. These objects must
     * be automatically deleted when the corresponding client process is dead or
     * if this interface is removed.
     *
     * @param callback An instance of the |ISupplicantP2pIfaceCallback| AIDL
     *        interface object.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void registerCallback(in ISupplicantP2pIfaceCallback callback);

    /**
     * Reinvoke a device from a persistent group.
     *
     * @param persistentNetworkId Used to specify the persistent group.
     * @param peerAddress MAC address of the device to reinvoke.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void reinvoke(in int persistentNetworkId, in byte[] peerAddress);

    /**
     * Reject connection attempt from a peer (specified with a device
     * address). This is a mechanism to reject a pending group owner negotiation
     * with a peer and request to automatically block any further connection or
     * discovery of the peer.
     *
     * @param peerAddress MAC address of the device to reject.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void reject(in byte[] peerAddress);

    /**
     * This command can be used to remove a bonjour service.
     *
     * @param query Hex dump of the query data.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NOT_STARTED|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void removeBonjourService(in byte[] query);

    /**
     * Terminate a P2P group. If a new virtual network interface was used for
     * the group, it must also be removed. The network interface name of the
     * group interface is used as a parameter for this command.
     *
     * @param groupIfName Group interface name to use.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void removeGroup(in String groupIfName);

    /**
     * Remove a network from the interface.
     *
     * Use |ISupplicantP2pNetwork.getId()| on the corresponding network AIDL
     * interface object to retrieve the ID.
     *
     * @param id Network ID allocated to the corresponding network.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_UNKNOWN|
     */
    void removeNetwork(in int id);

    /**
     * This command can be used to remove a UPNP service.
     *
     * @param version Version to be used.
     * @package serviceName Service name to be used.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NOT_STARTED|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void removeUpnpService(in int version, in String serviceName);

    /**
     * Report the initiation of the NFC handover select.
     *
     * @param select Bytes representing the handover select as specified in
     *        section 3.1.2 of NFC Connection Handover 1.2 Technical
     *        Specification.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void reportNfcHandoverInitiation(in byte[] select);

    /**
     * Report the response of the NFC handover request.
     *
     * @param request Bytes representing the handover request as specified in
     *        section 3.1.1 of NFC Connection Handover 1.2 Technical
     *        Specification.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void reportNfcHandoverResponse(in byte[] request);

    /**
     * Schedule a P2P service discovery request. The parameters for this command
     * are the device address of the peer device (or 00:00:00:00:00:00 for
     * wildcard query that is sent to every discovered P2P peer that supports
     * service discovery) and P2P Service Query TLV(s) as hexdump.
     *
     * @param peerAddress MAC address of the device to discover.
     * @param query Hex dump of the query data.
     * @return Identifier for the request. Can be used to cancel the
     *         request.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    long requestServiceDiscovery(in byte[] peerAddress, in byte[] query);

    /**
     * Persist the current configuration to disk.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void saveConfig();

    /**
     * Set P2P disallowed frequency ranges.
     *
     * Specify ranges of frequencies that are disallowed for any P2P operations.
     *
     * @param ranges List of ranges which needs to be disallowed.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setDisallowedFrequencies(in FreqRange[] ranges);

    /**
     * Set whether to enable EDMG(802.11ay). Only allowed if hw mode is |HOSTAPD_MODE_IEEE80211AD|
     *
     * @param enable true to set, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void setEdmg(in boolean enable);

    /**
     * Set the Maximum idle time in seconds for P2P groups.
     * This value controls how long a P2P group is maintained after there
     * is no other members in the group. As a group owner, this means no
     * associated stations in the group. As a P2P client, this means no
     * group owner seen in scan results.
     *
     * @param groupIfName Group interface name to use.
     * @param timeoutInSec Timeout value in seconds.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setGroupIdle(in String groupIfName, in int timeoutInSec);

    /**
     * Set P2P Listen channel.
     *
     * When specifying a social channel on the 2.4 GHz band (1/6/11) there is no
     * need to specify the operating class since it defaults to 81. When
     * specifying a social channel on the 60 GHz band (2), specify the 60 GHz
     * operating class (180).
     *
     * @param channel Wifi channel. eg, 1, 6, 11.
     * @param operatingClass Operating Class indicates the channel set of the AP
     *        indicated by this BSSID
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setListenChannel(in int channel, in int operatingClass);

    /**
     * Set MAC randomization enabled/disabled.
     *
     * @param enable true to enable, false to disable.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setMacRandomization(in boolean enable);

    /**
     * Send driver command to set Miracast mode.
     *
     * @param mode Mode of Miracast.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setMiracastMode(in MiracastMode mode);

    /**
     * Turn on/off power save mode for the interface.
     *
     * @param groupIfName Group interface name to use.
     * @param enable Indicate if power save is to be turned on/off.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void setPowerSave(in String groupIfName, in boolean enable);

    /**
     * Set the postfix to be used for P2P SSID's.
     *
     * @param postfix String to be appended to SSID.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setSsidPostfix(in byte[] postfix);

    /**
     * Set Wifi Display device info.
     *
     * @param info WFD device info as described in section 5.1.2 of WFD technical
     *        specification v1.0.0.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setWfdDeviceInfo(in byte[] info);

    /**
     * Set Wifi Display R2 device info.
     *
     * @param info WFD R2 device info as described in section 5.1.12 of WFD technical
     *        specification v2.1.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setWfdR2DeviceInfo(in byte[] info);

    /**
     * Remove the client with the MAC address from the group.
     *
     * @param peerAddress Mac address of the client.
     * @param isLegacyClient Indicate if client is a legacy client or not.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void removeClient(in byte[/*6*/] peerAddress, in boolean isLegacyClient);

    /**
     * Set the list of supported config methods for WPS operations.
     *
     * @param configMethods Mask of WPS configuration methods supported by the
     *        device.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsConfigMethods(in WpsConfigMethods configMethods);

    /**
     * Set the device name for WPS operations.
     * User-friendly description of device (up to |WPS_DEVICE_NAME_MAX_LEN|
     * octets encoded in UTF-8).
     *
     * @param name Name to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsDeviceName(in String name);

    /**
     * Set the device type for WPS operations.
     *
     * @param type Type of device. Refer to section B.1 of Wifi P2P
     *       Technical specification v1.2.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsDeviceType(in byte[] type);

    /**
     * Set the manufacturer for WPS operations.
     * The manufacturer of the device (up to |WPS_MANUFACTURER_MAX_LEN| ASCII
     * characters).
     *
     * @param manufacturer Manufacture to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsManufacturer(in String manufacturer);

    /**
     * Set the model name for WPS operations.
     * Model of the device (up to |WPS_MODEL_NAME_MAX_LEN| ASCII characters).
     *
     * @param modelName Model name to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsModelName(in String modelName);

    /**
     * Set the model number for WPS operations.
     * Additional device description (up to |WPS_MODEL_NUMBER_MAX_LEN| ASCII
     * characters).
     *
     * @param modelNumber Model number to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsModelNumber(in String modelNumber);

    /**
     * Set the serial number for WPS operations.
     * Serial number of the device (up to |WPS_SERIAL_NUMBER_MAX_LEN| characters)
     *
     * @param serialNumber Serial number to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsSerialNumber(in String serialNumber);

    /**
     * Initiate WPS Push Button setup.
     * The PBC operation requires that a button is also pressed at the
     * AP/Registrar at about the same time (2 minute window).
     *
     * @param groupIfName Group interface name to use.
     * @param bssid BSSID of the AP. Use zero'ed bssid to indicate wildcard.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void startWpsPbc(in String groupIfName, in byte[] bssid);

    /**
     * Initiate WPS Pin Display setup.
     *
     * @param groupIfName Group interface name to use.
     * @param bssid BSSID of the AP. Use zero'ed bssid to indicate wildcard.
     * @return 8 digit pin generated.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    String startWpsPinDisplay(in String groupIfName, in byte[] bssid);

    /**
     * Initiate WPS Pin Keypad setup.
     *
     * @param groupIfName Group interface name to use.
     * @param pin 8 digit pin to be used.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void startWpsPinKeypad(in String groupIfName, in String pin);

    /**
     * Stop an ongoing P2P service discovery.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void stopFind();

    /**
     * Initiate a P2P device discovery only on social channels.
     * <p>
     * @deprecated This method is deprecated from AIDL v3, newer HALs should use
     * findWithParams.
     *
     * @param timeoutInSec The maximum amount of time that should be spent in performing device
     *         discovery.
     *        Set to 0 to indefinitely continue discovery until an explicit
     *        |stopFind| is sent.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void findOnSocialChannels(in int timeoutInSec);

    /**
     * Initiate a P2P device discovery on a specific frequency.
     * <p>
     * @deprecated This method is deprecated from AIDL v3, newer HALs should use
     * findWithParams.
     *
     * @param freqInHz the frequency to be scanned.
     * @param timeoutInSec Max time to be spent is performing discovery.
     *        Set to 0 to indefinitely continue discovery until an explicit
     *        |stopFind| is sent.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void findOnSpecificFrequency(in int freqInHz, in int timeoutInSec);

    /**
     * Set vendor-specific information elements to P2P frames.
     *
     * @param frameTypeMask The bit mask of P2P frame type represented by
     *         P2pFrameTypeMask.
     * @param vendorElemBytes Vendor-specific information element bytes. The format of an
     *         information element is EID (1 byte) + Length (1 Byte) + Payload which is
     *         defined in Section 9.4.4 TLV encodings of 802.11-2016 IEEE Standard for
     *         Information technology. The length indicates the size of the payload.
     *         Multiple information elements may be appended within the byte array.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setVendorElements(in P2pFrameTypeMask frameTypeMask, in byte[] vendorElemBytes);

    /**
     * Configure the IP addresses in supplicant for P2P GO to provide the IP address to
     * client in EAPOL handshake. Refer Wi-Fi P2P Technical Specification v1.7 - Section  4.2.8
     * "IP Address Allocation in EAPOL-Key Frames (4-Way Handshake)" for more details.
     * The IP addresses are IPV4 addresses and higher-order address bytes are in the lower-order
     * int bytes (e.g. 1.2.3.4 is represented as 0x04030201)
     *
     * @param ipAddressGo The P2P Group Owner IP address.
     * @param ipAddressMask The P2P Group owner subnet mask.
     * @param ipAddressStart The starting address in the IP address pool.
     * @param ipAddressEnd The ending address in the IP address pool.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void configureEapolIpAddressAllocationParams(
            in int ipAddressGo, in int ipAddressMask, in int ipAddressStart, in int ipAddressEnd);

    /**
     * Start P2P group formation with a discovered P2P peer. This includes
     * optional group owner negotiation, group interface setup, provisioning,
     * and establishing data connection.
     *
     * @param connectInfo Parameters associated with this connection request.
     * @return Pin generated, if |provisionMethod| uses one of the
     *         generated |PIN*| methods.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    String connectWithParams(in P2pConnectInfo connectInfo);

    /**
     * Initiate a P2P service discovery with an optional timeout.
     *
     * @param discoveryInfo Parameters associated with this discovery request.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void findWithParams(in P2pDiscoveryInfo discoveryInfo);
}
