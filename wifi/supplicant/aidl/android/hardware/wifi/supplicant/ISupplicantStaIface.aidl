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

import android.hardware.wifi.supplicant.AnqpInfoId;
import android.hardware.wifi.supplicant.BtCoexistenceMode;
import android.hardware.wifi.supplicant.ConnectionCapabilities;
import android.hardware.wifi.supplicant.DppAkm;
import android.hardware.wifi.supplicant.DppCurve;
import android.hardware.wifi.supplicant.DppNetRole;
import android.hardware.wifi.supplicant.DppResponderBootstrapInfo;
import android.hardware.wifi.supplicant.Hs20AnqpSubtypes;
import android.hardware.wifi.supplicant.ISupplicantStaIfaceCallback;
import android.hardware.wifi.supplicant.ISupplicantStaNetwork;
import android.hardware.wifi.supplicant.IfaceType;
import android.hardware.wifi.supplicant.KeyMgmtMask;
import android.hardware.wifi.supplicant.MloLinksInfo;
import android.hardware.wifi.supplicant.MscsParams;
import android.hardware.wifi.supplicant.QosPolicyScsData;
import android.hardware.wifi.supplicant.QosPolicyScsRequestStatus;
import android.hardware.wifi.supplicant.QosPolicyStatus;
import android.hardware.wifi.supplicant.RxFilterType;
import android.hardware.wifi.supplicant.SignalPollResult;
import android.hardware.wifi.supplicant.WpaDriverCapabilitiesMask;
import android.hardware.wifi.supplicant.WpsConfigMethods;

/**
 * Interface exposed by the supplicant for each station mode network
 * interface (e.g wlan0) it controls.
 */
@VintfStability
interface ISupplicantStaIface {
    /**
     * Add a DPP peer URI. URI is acquired externally, e.g. by scanning a QR code
     *
     * @param uri Peer's DPP URI.
     * @return ID for the URI
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    int addDppPeerUri(in String uri);

    /**
     * External programs can request supplicant to not start offchannel
     * operations during other tasks that may need exclusive control of the
     * radio.
     *
     * This method can be used to reserve a slot for radio access. If freq is
     * specified, other radio work items on the same channel can be completed in
     * parallel. Otherwise, all other radio work items are blocked during
     * execution. Timeout must be set to |ExtRadioWorkDefaults.TIMEOUT_IN_SECS|
     * seconds by default to avoid blocking supplicant operations on the iface
     * for excessive time. If a longer (or shorter) safety timeout is needed,
     * that may be specified with the optional timeout parameter. This command
     * returns an identifier for the radio work item.
     *
     * Once the radio work item has been started,
     * |ISupplicant.onExtRadioWorkStart| callback is indicated that the external
     * processing can start.
     *
     * @param name Name for the radio work being added.
     * @param freqInMhz Frequency to specify. Set to 0 for all channels.
     * @param timeoutInSec Timeout to specify. Set to 0 for default timeout.
     * @return Identifier for this radio work.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    int addExtRadioWork(in String name, in int freqInMhz, in int timeoutInSec);

    /**
     * Add a new network to the interface.
     *
     * @return AIDL interface object representing the new network if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    @PropagateAllowBlocking ISupplicantStaNetwork addNetwork();

    /**
     * Send driver command to add the specified RX filter.
     *
     * @param type Type of filter.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void addRxFilter(in RxFilterType type);

    /**
     * Cancel any ongoing WPS operations.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void cancelWps();

    /**
     * Disconnect from the current active network.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void disconnect();

    /**
     * Enable/Disable auto reconnect to networks.
     * Use this to prevent wpa_supplicant from trying to connect to networks
     * on its own.
     *
     * @param enable true to enable, false to disable.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void enableAutoReconnect(in boolean enable);

    /**
     * Add fast initial link setup (IEEE 802.11ai FILS) HLP packets.
     * Use this to add higher layer protocol (HLP) packet in FILS (Re)Association Request frame
     * (Eg: DHCP discover packet).
     *
     * @param dst_mac MAC address of the destination
     * @param pkt The contents of the HLP packet starting from ethertype
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void filsHlpAddRequest(in byte[] dst_mac, in byte[] pkt);

    /**
     * Flush fast initial link setup (IEEE 802.11ai FILS) HLP packets.
     * Use this to flush all the higher layer protocol (HLP) packets added in
     * wpa_supplicant to send in FILS (Re)Association Request frame
     * (Eg: DHCP discover packet).
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void filsHlpFlushRequest();

    /**
     * Generates DPP bootstrap information: Bootstrap ID, DPP URI and listen
     * channel for responder mode.
     *
     * @param macAddress MAC address of the interface for the DPP operation.
     * @param deviceInfo Device specific information.
     *        As per DPP Specification V1.0 section 5.2,
     *        allowed Range of ASCII characters in deviceInfo - %x20-7E
     *        semicolon is not allowed.
     * @param curve Elliptic curve cryptography type used to generate DPP
     *        public/private key pair.
     * @return Bootstrap info.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     *         |SupplicantStatusCode.FAILURE_UNSUPPORTED|
     */
    DppResponderBootstrapInfo generateDppBootstrapInfoForResponder(
            in byte[] macAddress, in String deviceInfo, in DppCurve curve);

    /**
     * To Onboard / Configure self with DPP credentials.
     *
     * This is used to generate DppConnectionKeys for self. Thus a configurator
     * can use the credentials to connect to an AP which it has configured for
     * DPP AKM. This should be called before initiating first DPP connection
     * on Configurator side. This API generates onDppSuccessConfigReceived()
     * callback event asynchronously with DppConnectionKeys.
     *
     * @param ssid Network SSID configured profile
     * @param privEcKey Private EC keys for this profile which was used to
     *        configure other enrollee in network.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     *         |SupplicantStatusCode.FAILURE_UNSUPPORTED|
     */
    void generateSelfDppConfiguration(in String ssid, in byte[] privEcKey);

    /**
     * Get Connection capabilities
     *
     * @return Connection capabilities.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    ConnectionCapabilities getConnectionCapabilities();

    /**
     * Get Connection MLO links Info
     *
     * @return Connection MLO Links Info.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    MloLinksInfo getConnectionMloLinksInfo();

    /**
     * Get Key management capabilities of the device
     *
     * @return Bitmap of key management mask.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    KeyMgmtMask getKeyMgmtCapabilities();

    /**
     * Send driver command to get MAC address of the device.
     *
     * @return MAC address of the device.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    byte[] getMacAddress();

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
     * Use |ISupplicantStaNetwork.getId()| on the corresponding network AIDL
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
    @PropagateAllowBlocking ISupplicantStaNetwork getNetwork(in int id);

    /**
     * Retrieves the type of the network interface.
     *
     * @return Type of the network interface, e.g., STA.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    IfaceType getType();

    /**
     * Get wpa driver capabilities.
     *
     * @return Bitmap of wpa driver features.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    WpaDriverCapabilitiesMask getWpaDriverCapabilities();

    /**
     * Initiate ANQP (for IEEE 802.11u Interworking/Hotspot 2.0) queries with the
     * specified access point.
     * The ANQP data fetched must be returned in the
     * |ISupplicantStaIfaceCallback.onAnqpQueryDone| callback.
     *
     * @param macAddress MAC address of the access point.
     * @param infoElements List of information elements to query for.
     * @param subtypes List of HS20 subtypes to query for.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void initiateAnqpQuery(
            in byte[] macAddress, in AnqpInfoId[] infoElements, in Hs20AnqpSubtypes[] subTypes);

    /**
     * Initiate the Hotspot 2.0 icon query with the specified accesss point.
     * The icon data fetched must be returned in the
     * |ISupplicantStaIfaceCallback.onHs20IconQueryDone| callback.
     *
     * @deprecated No longer in use.
     *
     * @param macAddress MAC address of the access point.
     * @param fileName Name of the file to request from the access point.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void initiateHs20IconQuery(in byte[] macAddress, in String fileName);

    /**
     * Initiate TDLS discover with the provided peer MAC address.
     *
     * @param macAddress MAC address of the peer.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void initiateTdlsDiscover(in byte[] macAddress);

    /**
     * Initiate TDLS setup with the provided peer MAC address.
     *
     * @param macAddress MAC address of the peer.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void initiateTdlsSetup(in byte[] macAddress);

    /**
     * Initiate TDLS teardown with the provided peer MAC address.
     *
     * @param macAddress MAC address of the peer.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void initiateTdlsTeardown(in byte[] macAddress);

    /**
     * Initiate Venue URL ANQP (for IEEE 802.11u Interworking/Hotspot 2.0) query with the
     * specified access point. This specific query can be used only post connection, once security
     * is established and PMF is enabled, to avoid spoofing preassociation ANQP responses.
     * The ANQP data fetched must be returned in the
     * |ISupplicantStaIfaceCallback.onAnqpQueryDone| callback.
     *
     * @param macAddress MAC address of the access point.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void initiateVenueUrlAnqpQuery(in byte[] macAddress);

    /**
     * Retrieve a list of all the network Id's controlled by the supplicant.
     *
     * The corresponding |ISupplicantStaNetwork| object for any network can be
     * retrieved using |getNetwork| method.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     * @return List of all network Id's controlled by the supplicant.
     */
    int[] listNetworks();

    /**
     * Reconnect to the currently active network, even if we are already
     * connected.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void reassociate();

    /**
     * Reconnect to the currently active network, if we are currently
     * disconnected.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|,
     *         |SupplicantStatusCode.FAILURE_IFACE_NOT_DISCONNECTED|
     */
    void reconnect();

    /**
     * Register for callbacks from this interface.
     *
     * These callbacks are invoked for events that are specific to this interface.
     * Registration of multiple callback objects is supported. These objects must
     * be automatically deleted when the corresponding client process is dead or
     * if this interface is removed.
     *
     * @param callback An instance of the |ISupplicantStaIfaceCallback| AIDL
     *        interface object.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void registerCallback(in ISupplicantStaIfaceCallback callback);

    /**
     * Enable/disable QoS policy feature.
     * @param enable true to enable, false to disable.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setQosPolicyFeatureEnabled(in boolean enable);

    /**
     * Send a DSCP policy response to the AP. If a DSCP request is ongoing,
     * sends a solicited (uses the ongoing DSCP request as dialog token) DSCP
     * response. Otherwise, sends an unsolicited DSCP response.
     *
     * @param qosPolicyRequestId Dialog token to identify the request.
     * @param morePolicies Flag to indicate more QoS policies can be accommodated.
     * @param qosPolicyStatusList QoS policy status info for each QoS policy id.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_UNSUPPORTED|
     */
    void sendQosPolicyResponse(in int qosPolicyRequestId, in boolean morePolicies,
            in QosPolicyStatus[] qosPolicyStatusList);

    /**
     * Indicate removal of all active QoS policies configured by the AP.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_UNSUPPORTED|
     */
    void removeAllQosPolicies();

    /**
     * Remove a DPP peer URI.
     *
     * @param id The ID of the URI, as returned by |addDppPeerUri|.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void removeDppUri(in int id);

    /**
     * Indicates to supplicant that the external radio work has completed.
     * This allows other radio works to be performed. If this method is not
     * invoked (e.g., due to the external program terminating), supplicant
     * must time out the radio work item on the iface and send
     * |ISupplicantStaIfaceCallback.onExtRadioWorkTimeout| event to indicate
     * that this has happened.
     *
     * This method may also be used to cancel items that have been scheduled
     * via |addExtRadioWork|, but have not yet been started (notified via
     * |ISupplicantStaIfaceCallback.onExtRadioWorkStart|).
     *
     * @param id Identifier generated for the radio work addition
     *         (using |addExtRadioWork|).
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void removeExtRadioWork(in int id);

    /**
     * Remove a network from the interface.
     *
     * Use |ISupplicantStaNetwork.getId()| on the corresponding network AIDL
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
     * Send driver command to remove the specified RX filter.
     *
     * @param type Type of filter.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void removeRxFilter(in RxFilterType type);

    /**
     * Send driver command to set Bluetooth coexistence mode.
     *
     * @param mode Mode of Bluetooth coexistence.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setBtCoexistenceMode(in BtCoexistenceMode mode);

    /**
     * Send driver command to set Bluetooth coexistence scan mode.
     * When this mode is on, some of the low-level scan parameters
     * used by the driver are changed to reduce interference
     * with A2DP streaming.
     *
     * @param enable true to enable, false to disable.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setBtCoexistenceScanModeEnabled(in boolean enable);

    /**
     * Send driver command to set country code.
     *
     * @param code 2 byte country code (as defined in ISO 3166) to set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setCountryCode(in byte[] code);

    /**
     * Use external processing for SIM/USIM operations
     *
     * @param useExternalSim true to use external, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setExternalSim(in boolean useExternalSim);

    /**
     * Set Wi-Fi Alliance Agile Multiband (MBO) cellular data status.
     *
     * @param available true means cellular data available, false otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setMboCellularDataStatus(in boolean available);

    /**
     * Turn on/off power save mode for the interface.
     *
     * @param enable Indicate if power save is to be turned on/off.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_IFACE_DISABLED|
     */
    void setPowerSave(in boolean enable);

    /**
     * Send driver command to set suspend optimizations for power save.
     *
     * @param enable true to enable, false to disable.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void setSuspendModeEnabled(in boolean enable);

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
     * @parm name Name to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsDeviceName(in String name);

    /**
     * Set the device type for WPS operations.
     *
     * @parm type Type of device. Refer to section B.1 of Wifi P2P
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
     * @parm manufacturer Manufacture to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsManufacturer(in String manufacturer);

    /**
     * Set the model name for WPS operations.
     * Model of the device (up to |WPS_MODEL_NAME_MAX_LEN| ASCII characters).
     *
     * @parm modelName Model name to be set.
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
     * @parm modelNumber Model number to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsModelNumber(in String modelNumber);

    /**
     * Set the serial number for WPS operations.
     * Serial number of the device (up to |WPS_SERIAL_NUMBER_MAX_LEN| characters)
     *
     * @parm serialNumber Serial number to be set.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setWpsSerialNumber(in String serialNumber);

    /**
     * Start DPP in Configurator-Initiator mode.
     *
     * @param peerBootstrapId Peer device's URI ID.
     * @param ownBootstrapId Local device's URI ID (0 for none, optional).
     * @param ssid Network SSID to send to peer (SAE/PSK/DPP mode).
     * @param password Network password to send to peer (SAE/PSK mode).
     * @param psk Network PSK to send to peer (PSK mode only). Either password or psk should be set.
     * @param netRole Role to configure the peer, |DppNetRole.DPP_NET_ROLE_STA| or
     *        |DppNetRole.DPP_NET_ROLE_AP|.
     * @param securityAkm Security AKM to use (See DppAkm).
     * @param privEcKey Private EC keys for this profile which was used to
     *        configure other enrollee in network. This param is valid only for DPP AKM.
     *        This param is set to Null by configurator to indicate first DPP-AKM based
     *        configuration to an Enrollee. non-Null value indicates configurator had
     *        previously configured an enrollee.
     * @return Return the Private EC key when securityAkm is DPP and privEcKey was Null.
     *         Otherwise return Null.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    byte[] startDppConfiguratorInitiator(in int peerBootstrapId, in int ownBootstrapId,
            in String ssid, in String password, in String psk, in DppNetRole netRole,
            in DppAkm securityAkm, in byte[] privEcKey);

    /**
     * Start DPP in Enrollee-Initiator mode.
     *
     * @param peerBootstrapId Peer device's URI ID.
     * @param ownBootstrapId Local device's URI ID (0 for none, optional).
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void startDppEnrolleeInitiator(in int peerBootstrapId, in int ownBootstrapId);

    /**
     * Start DPP in Enrollee-Responder mode.
     * Framework must first call |generateDppBootstrapInfoForResponder| to generate
     * the bootstrapping information: Bootstrap ID, DPP URI and the listen channel.
     * Then call this API with derived listen channel to start listening for
     * authentication request from Peer initiator.
     *
     * @param listenChannel DPP listen channel.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     *         |SupplicantStatusCode.FAILURE_UNSUPPORTED|
     */
    void startDppEnrolleeResponder(in int listenChannel);

    /**
     * Send driver command to start RX filter.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void startRxFilter();

    /**
     * Initiate WPS Push Button setup.
     * The PBC operation requires that a button is also pressed at the
     * AP/Registrar at about the same time (2 minute window).
     *
     * @param bssid BSSID of the AP. Use zero'ed bssid to indicate wildcard.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void startWpsPbc(in byte[] bssid);

    /**
     * Initiate WPS Pin Display setup.
     *
     * @param bssid BSSID of the AP. Use zero'ed bssid to indicate wildcard.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     * @return 8 digit pin generated.
     */
    String startWpsPinDisplay(in byte[] bssid);

    /**
     * Initiate WPS Pin Keypad setup.
     *
     * @param pin 8 digit pin to be used.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void startWpsPinKeypad(in String pin);

    /**
     * Initiate WPS setup in registrar role to learn the current AP configuration.
     *
     * @param bssid BSSID of the AP.
     * @param pin Pin of the AP.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void startWpsRegistrar(in byte[] bssid, in String pin);

    /**
     * Stop DPP Initiator operation.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_NETWORK_INVALID|
     */
    void stopDppInitiator();

    /**
     * Stop DPP Responder operation - Remove the bootstrap code and stop listening.
     *
     * @param ownBootstrapId Local device's URI ID obtained through
     *        |generateDppBootstrapInfoForResponder| call.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     *         |SupplicantStatusCode.FAILURE_UNSUPPORTED|
     */
    void stopDppResponder(in int ownBootstrapId);

    /**
     * Send driver command to stop RX filter.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_INVALID|
     */
    void stopRxFilter();

    /**
     * This method returns the signal poll results. Results will be for each
     * link in case of Multiple Link Operation (MLO).
     *
     * @return Signal Poll Results per link.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     *         |SupplicantStatusCode.FAILURE_UNSUPPORTED|
     */
    SignalPollResult[] getSignalPollResults();

    /**
     * Maximum number of policies that can be included in a QoS SCS add/remove request.
     */
    const int MAX_POLICIES_PER_QOS_SCS_REQUEST = 16;

    /**
     * Send a set of QoS SCS policy add requests to the AP.
     *
     * This is a request to the AP (if it supports the feature) to apply the QoS policies
     * on traffic in the downlink.
     *
     * Synchronous response will indicate which policies were sent to the AP, and which
     * were rejected immediately by supplicant. Caller will also receive an asynchronous
     * response in |ISupplicantStaIfaceCallback.onQosPolicyResponseForScs| indicating
     * the response from the AP for each policy that was sent.
     *
     * @param  qosPolicyScsData QoS policies info provided by STA.
     * @return QosPolicyScsRequestStatus[] synchronously corresponding to all
     *         the scs policies. Size of the result array will be the same as
     *         the size of the input array.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN| if the number of policies in the
     *          request is greater than |MAX_POLICIES_PER_QOS_SCS_REQUEST|
     *
     *         |SupplicantStatusCode.FAILURE_UNSUPPORTED| if the AP does not support
     *          the feature.
     *
     *         |SupplicantStatusCode.FAILURE_ONGOING_REQUEST| if a request is currently
     *          being processed. Supplicant will only handle one request at a time.
     */
    QosPolicyScsRequestStatus[] addQosPolicyRequestForScs(in QosPolicyScsData[] qosPolicyData);

    /**
     * Request the removal of specific QoS policies for SCS configured by the STA.
     *
     * Synchronous response will indicate which policies were sent to the AP, and which
     * were rejected immediately by supplicant. Caller will also receive an asynchronous
     * response in |ISupplicantStaIfaceCallback.onQosPolicyResponseForScs| indicating
     * the response from the AP for each policy that was sent.
     *
     * @param  scsPolicyIds policy id's to be removed.
     * @return QosPolicyScsRequestStatus[] synchronously corresponding to all
     *         the scs policies.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN| if the number of policies in the
     *          request is greater than |MAX_POLICIES_PER_QOS_SCS_REQUEST|
     *
     *         |SupplicantStatusCode.FAILURE_UNSUPPORTED| if the AP does not support
     *          the feature.
     *
     *         |SupplicantStatusCode.FAILURE_ONGOING_REQUEST| if a request is currently
     *          being processed. Supplicant will only handle one request at a time.
     */
    QosPolicyScsRequestStatus[] removeQosPolicyForScs(in byte[] scsPolicyIds);

    /**
     * Enable Mirrored Stream Classification Service (MSCS) and configure using
     * the provided configuration values.
     *
     * If MSCS has already been enabled/configured, this will overwrite the
     * existing configuration.
     *
     * @param params |MscsParams| object containing the configuration.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID| if the configuration is invalid.
     *         |SupplicantStatusCode.FAILURE_UNKNOWN| if the configuration could not be set.
     */
    void configureMscs(in MscsParams params);

    /**
     * Disable Mirrored Stream Classification Service (MSCS).
     *
     * If MSCS is enabled/configured, this will send a remove request to the AP.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void disableMscs();
}
