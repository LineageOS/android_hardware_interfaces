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

import android.hardware.wifi.supplicant.P2pDeviceFoundEventParams;
import android.hardware.wifi.supplicant.P2pGroupCapabilityMask;
import android.hardware.wifi.supplicant.P2pGroupStartedEventParams;
import android.hardware.wifi.supplicant.P2pPeerClientDisconnectedEventParams;
import android.hardware.wifi.supplicant.P2pPeerClientJoinedEventParams;
import android.hardware.wifi.supplicant.P2pProvDiscStatusCode;
import android.hardware.wifi.supplicant.P2pProvisionDiscoveryCompletedEventParams;
import android.hardware.wifi.supplicant.P2pStatusCode;
import android.hardware.wifi.supplicant.WpsConfigMethods;
import android.hardware.wifi.supplicant.WpsDevPasswordId;

/**
 * Callback Interface exposed by the supplicant service
 * for each P2P mode interface (ISupplicantP2pIface).
 *
 * Clients need to host an instance of this AIDL interface object and
 * pass a reference of the object to the supplicant via the
 * corresponding |ISupplicantP2pIface.registerCallback| method.
 */
@VintfStability
oneway interface ISupplicantP2pIfaceCallback {
    /**
     * Used to indicate that a P2P device has been found.
     * <p>
     * @deprecated This callback is deprecated from AIDL v2, newer HAL should call
     * onDeviceFoundWithParams.
     *
     * @param srcAddress MAC address of the device found. This must either
     *        be the P2P device address or the P2P interface address.
     * @param p2pDeviceAddress P2P device address.
     * @param primaryDeviceType Type of device. Refer to section B.1 of Wifi P2P
     *        Technical specification v1.2.
     * @param deviceName Name of the device.
     * @param configMethods Mask of WPS configuration methods supported by the
     *        device.
     * @param deviceCapabilities Refer to section 4.1.4 of Wifi P2P Technical
     *        specification v1.2.
     * @param groupCapabilites Refer to section 4.1.4 of Wifi P2P Technical
     *        specification v1.2.
     * @param wfdDeviceInfo WFD device info as described in section 5.1.2 of WFD
     *        technical specification v1.0.0.
     */
    void onDeviceFound(in byte[] srcAddress, in byte[] p2pDeviceAddress,
            in byte[] primaryDeviceType, in String deviceName, in WpsConfigMethods configMethods,
            in byte deviceCapabilities, in P2pGroupCapabilityMask groupCapabilities,
            in byte[] wfdDeviceInfo);

    /**
     * Used to indicate that a P2P device has been lost.
     *
     * @param p2pDeviceAddress P2P device address.
     */
    void onDeviceLost(in byte[] p2pDeviceAddress);

    /**
     * Used to indicate the termination of P2P find operation.
     */
    void onFindStopped();

    /**
     * Used to indicate the completion of a P2P Group Owner negotiation request.
     *
     * @param status Status of the GO negotiation.
     */
    void onGoNegotiationCompleted(in P2pStatusCode status);

    /**
     * Used to indicate the reception of a P2P Group Owner negotiation request.
     *
     * @param srcAddress MAC address of the device that initiated the GO
     *        negotiation request.
     * @param passwordId Type of password.
     */
    void onGoNegotiationRequest(in byte[] srcAddress, in WpsDevPasswordId passwordId);

    /**
     * Used to indicate a failure to form a P2P group.
     *
     * @param failureReason Failure reason string for debug purposes.
     */
    void onGroupFormationFailure(in String failureReason);

    /**
     * Used to indicate a successful formation of a P2P group.
     */
    void onGroupFormationSuccess();

    /**
     * Used to indicate the removal of a P2P group.
     *
     * @param groupIfName Interface name of the group. (For ex: p2p-p2p0-1)
     * @param isGroupOwner Whether this device is owner of the group.
     */
    void onGroupRemoved(in String groupIfname, in boolean isGroupOwner);

    /**
     * Used to indicate the start of a P2P group.
     *
     * @param groupIfName Interface name of the group. (For ex: p2p-p2p0-1)
     * @param isGroupOwner Whether this device is owner of the group.
     * @param ssid SSID of the group.
     * @param frequency Frequency on which this group is created.
     * @param psk PSK used to secure the group.
     * @param passphrase PSK passphrase used to secure the group.
     * @param goDeviceAddress MAC Address of the owner of this group.
     * @param isPersistent Whether this group is persisted or not.
     */
    void onGroupStarted(in String groupIfname, in boolean isGroupOwner, in byte[] ssid,
            in int frequency, in byte[] psk, in String passphrase, in byte[] goDeviceAddress,
            in boolean isPersistent);

    /**
     * Used to indicate the reception of a P2P invitation.
     *
     * @param srcAddress MAC address of the device that sent the invitation.
     * @param goDeviceAddress MAC Address of the owner of this group.
     * @param bssid Bssid of the group.
     * @param persistentNetworkId Persistent network Id of the group.
     * @param operatingFrequency Frequency on which the invitation was received.
     */
    void onInvitationReceived(in byte[] srcAddress, in byte[] goDeviceAddress, in byte[] bssid,
            in int persistentNetworkId, in int operatingFrequency);

    /**
     * Used to indicate the result of the P2P invitation request.
     *
     * @param bssid Bssid of the group.
     * @param status Status of the invitation.
     */
    void onInvitationResult(in byte[] bssid, in P2pStatusCode status);

    /**
     * Used to indicate the completion of a P2P provision discovery request.
     * <p>
     * @deprecated This callback is deprecated from AIDL v3, newer HAL should call
     * onProvisionDiscoveryCompletedEvent.
     *
     * @param p2pDeviceAddress P2P device address.
     * @param isRequest Whether we received or sent the provision discovery.
     * @param status Status of the provision discovery.
     * @param configMethods Mask of WPS configuration methods supported.
     * @param generatedPin 8 digit pin generated.
     */
    void onProvisionDiscoveryCompleted(in byte[] p2pDeviceAddress, in boolean isRequest,
            in P2pProvDiscStatusCode status, in WpsConfigMethods configMethods,
            in String generatedPin);

    /**
     * Used to indicate that a P2P Wi-Fi Display R2 device has been found. Refer to
     * Wi-Fi Display Technical Specification Version 2.0.
     *
     * @param srcAddress MAC address of the device found. This must either
     *        be the P2P device address for a peer which is not in a group,
     *        or the P2P interface address for a peer which is a Group Owner.
     * @param p2pDeviceAddress P2P device address.
     * @param primaryDeviceType Type of device. Refer to section B.1 of Wifi P2P
     *        Technical specification v1.2.
     * @param deviceName Name of the device.
     * @param configMethods Mask of WPS configuration methods supported by the
     *        device.
     * @param deviceCapabilities Refer to section 4.1.4 of Wifi P2P Technical
     *        specification v1.2.
     * @param groupCapabilites Refer to section 4.1.4 of Wifi P2P Technical
     *        specification v1.2.
     * @param wfdDeviceInfo WFD device info as described in section 5.1.2 of WFD
     *        technical specification v1.0.0.
     * @param wfdR2DeviceInfo WFD R2 device info as described in section 5.1.12 of WFD
     *        technical specification v2.1.
     */
    void onR2DeviceFound(in byte[] srcAddress, in byte[] p2pDeviceAddress,
            in byte[] primaryDeviceType, in String deviceName, in WpsConfigMethods configMethods,
            in byte deviceCapabilities, in P2pGroupCapabilityMask groupCapabilities,
            in byte[] wfdDeviceInfo, in byte[] wfdR2DeviceInfo);

    /**
     * Used to indicate the reception of a P2P service discovery response.
     *
     * @param srcAddress MAC address of the device that sent the service discovery.
     * @param updateIndicator Service update indicator. Refer to section 3.1.3 of
     *        Wifi P2P Technical specification v1.2.
     * @parm tlvs Refer to section 3.1.3.1 of Wifi P2P Technical specification v1.2.
     */
    void onServiceDiscoveryResponse(in byte[] srcAddress, in char updateIndicator, in byte[] tlvs);

    /**
     * Used to indicate when a STA device is connected to this device.
     * <p>
     * @deprecated This callback is deprecated from AIDL v3, newer HAL should call
     * onPeerClientJoined()
     *
     * @param srcAddress MAC address of the device that was authorized.
     * @param p2pDeviceAddress P2P device address.
     */
    void onStaAuthorized(in byte[] srcAddress, in byte[] p2pDeviceAddress);

    /**
     * Used to indicate when a STA device is disconnected from this device.
     * <p>
     * @deprecated This callback is deprecated from AIDL v3, newer HAL should call
     * onPeerClientDisconnected()
     *
     * @param srcAddress MAC address of the device that was deauthorized.
     * @param p2pDeviceAddress P2P device address.
     */
    void onStaDeauthorized(in byte[] srcAddress, in byte[] p2pDeviceAddress);

    /**
     * Used to indicate that operating frequency has changed for this P2P group interface.
     *
     * @param groupIfName Interface name of the group. (For ex: p2p-p2p0-1)
     * @param frequency New operating frequency in MHz.
     */
    void onGroupFrequencyChanged(in String groupIfname, in int frequency);

    /**
     * Used to indicate that a P2P device has been found.
     * <p>
     * @deprecated This callback is deprecated from AIDL v3, newer HAL should call
     * onDeviceFoundWithParams.
     *
     * @param srcAddress MAC address of the device found. This must either
     *        be the P2P device address for a peer which is not in a group,
     *        or the P2P interface address for a peer which is a Group Owner.
     * @param p2pDeviceAddress P2P device address.
     * @param primaryDeviceType Type of device. Refer to section B.1 of Wifi P2P
     *        Technical specification v1.2.
     * @param deviceName Name of the device.
     * @param configMethods Mask of WPS configuration methods supported by the
     *        device.
     * @param deviceCapabilities Refer to section 4.1.4 of Wifi P2P Technical
     *        specification v1.2.
     * @param groupCapabilites Refer to section 4.1.4 of Wifi P2P Technical
     *        specification v1.2.
     * @param wfdDeviceInfo WFD device info as described in section 5.1.2 of WFD
     *        technical specification v1.0.0.
     * @param wfdR2DeviceInfo WFD R2 device info as described in section 5.1.12 of WFD
     *        technical specification v2.1.
     * @param vendorElemBytes Vendor-specific information element bytes. The format of an
     *         information element is EID (1 byte) + Length (1 Byte) + Payload which is
     *         defined in Section 9.4.4 TLV encodings of 802.11-2016 IEEE Standard for
     *         Information technology. The length indicates the size of the payload.
     *         Multiple information elements may be appended within the byte array.
     */
    void onDeviceFoundWithVendorElements(in byte[] srcAddress, in byte[] p2pDeviceAddress,
            in byte[] primaryDeviceType, in String deviceName, in WpsConfigMethods configMethods,
            in byte deviceCapabilities, in P2pGroupCapabilityMask groupCapabilities,
            in byte[] wfdDeviceInfo, in byte[] wfdR2DeviceInfo, in byte[] vendorElemBytes);

    /**
     * Used to indicate the start of a P2P group, with some parameters describing the group.
     *
     * @param groupStartedEventParams Parameters describing the P2P group.
     */
    void onGroupStartedWithParams(in P2pGroupStartedEventParams groupStartedEventParams);

    /**
     * Used to indicate that a P2P client has joined this device group owner.
     *
     * @param clientJoinedEventParams Parameters associated with peer client joined event.
     */
    void onPeerClientJoined(in P2pPeerClientJoinedEventParams clientJoinedEventParams);

    /**
     * Used to indicate that a P2P client has disconnected from this device group owner.
     *
     * @param clientDisconnectedEventParams Parameters associated with peer client disconnected
     *         event.
     */
    void onPeerClientDisconnected(
            in P2pPeerClientDisconnectedEventParams clientDisconnectedEventParams);

    /**
     * Used to indicate the completion of a P2P provision discovery request.
     *
     * @param provisionDiscoveryCompletedEventParams Parameters associated with
     *        P2P provision discovery frame notification.
     */
    void onProvisionDiscoveryCompletedEvent(
            in P2pProvisionDiscoveryCompletedEventParams provisionDiscoveryCompletedEventParams);

    /**
     * Used to indicate that a P2P device has been found.
     *
     * @param deviceFoundEventParams Parameters associated with the device found event.
     */
    void onDeviceFoundWithParams(in P2pDeviceFoundEventParams deviceFoundEventParams);
}
