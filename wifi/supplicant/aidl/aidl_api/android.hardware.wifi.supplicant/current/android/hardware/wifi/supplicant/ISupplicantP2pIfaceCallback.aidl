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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.wifi.supplicant;
@VintfStability
interface ISupplicantP2pIfaceCallback {
  /**
   * @deprecated This callback is deprecated from AIDL v2, newer HAL should call onDeviceFoundWithParams.
   */
  oneway void onDeviceFound(in byte[] srcAddress, in byte[] p2pDeviceAddress, in byte[] primaryDeviceType, in String deviceName, in android.hardware.wifi.supplicant.WpsConfigMethods configMethods, in byte deviceCapabilities, in android.hardware.wifi.supplicant.P2pGroupCapabilityMask groupCapabilities, in byte[] wfdDeviceInfo);
  oneway void onDeviceLost(in byte[] p2pDeviceAddress);
  oneway void onFindStopped();
  oneway void onGoNegotiationCompleted(in android.hardware.wifi.supplicant.P2pStatusCode status);
  oneway void onGoNegotiationRequest(in byte[] srcAddress, in android.hardware.wifi.supplicant.WpsDevPasswordId passwordId);
  oneway void onGroupFormationFailure(in String failureReason);
  oneway void onGroupFormationSuccess();
  oneway void onGroupRemoved(in String groupIfname, in boolean isGroupOwner);
  oneway void onGroupStarted(in String groupIfname, in boolean isGroupOwner, in byte[] ssid, in int frequency, in byte[] psk, in String passphrase, in byte[] goDeviceAddress, in boolean isPersistent);
  oneway void onInvitationReceived(in byte[] srcAddress, in byte[] goDeviceAddress, in byte[] bssid, in int persistentNetworkId, in int operatingFrequency);
  oneway void onInvitationResult(in byte[] bssid, in android.hardware.wifi.supplicant.P2pStatusCode status);
  /**
   * @deprecated This callback is deprecated from AIDL v3, newer HAL should call onProvisionDiscoveryCompletedEvent.
   */
  oneway void onProvisionDiscoveryCompleted(in byte[] p2pDeviceAddress, in boolean isRequest, in android.hardware.wifi.supplicant.P2pProvDiscStatusCode status, in android.hardware.wifi.supplicant.WpsConfigMethods configMethods, in String generatedPin);
  oneway void onR2DeviceFound(in byte[] srcAddress, in byte[] p2pDeviceAddress, in byte[] primaryDeviceType, in String deviceName, in android.hardware.wifi.supplicant.WpsConfigMethods configMethods, in byte deviceCapabilities, in android.hardware.wifi.supplicant.P2pGroupCapabilityMask groupCapabilities, in byte[] wfdDeviceInfo, in byte[] wfdR2DeviceInfo);
  oneway void onServiceDiscoveryResponse(in byte[] srcAddress, in char updateIndicator, in byte[] tlvs);
  /**
   * @deprecated This callback is deprecated from AIDL v3, newer HAL should call onPeerClientJoined()
   */
  oneway void onStaAuthorized(in byte[] srcAddress, in byte[] p2pDeviceAddress);
  /**
   * @deprecated This callback is deprecated from AIDL v3, newer HAL should call onPeerClientDisconnected()
   */
  oneway void onStaDeauthorized(in byte[] srcAddress, in byte[] p2pDeviceAddress);
  oneway void onGroupFrequencyChanged(in String groupIfname, in int frequency);
  /**
   * @deprecated This callback is deprecated from AIDL v3, newer HAL should call onDeviceFoundWithParams.
   */
  oneway void onDeviceFoundWithVendorElements(in byte[] srcAddress, in byte[] p2pDeviceAddress, in byte[] primaryDeviceType, in String deviceName, in android.hardware.wifi.supplicant.WpsConfigMethods configMethods, in byte deviceCapabilities, in android.hardware.wifi.supplicant.P2pGroupCapabilityMask groupCapabilities, in byte[] wfdDeviceInfo, in byte[] wfdR2DeviceInfo, in byte[] vendorElemBytes);
  oneway void onGroupStartedWithParams(in android.hardware.wifi.supplicant.P2pGroupStartedEventParams groupStartedEventParams);
  oneway void onPeerClientJoined(in android.hardware.wifi.supplicant.P2pPeerClientJoinedEventParams clientJoinedEventParams);
  oneway void onPeerClientDisconnected(in android.hardware.wifi.supplicant.P2pPeerClientDisconnectedEventParams clientDisconnectedEventParams);
  oneway void onProvisionDiscoveryCompletedEvent(in android.hardware.wifi.supplicant.P2pProvisionDiscoveryCompletedEventParams provisionDiscoveryCompletedEventParams);
  oneway void onDeviceFoundWithParams(in android.hardware.wifi.supplicant.P2pDeviceFoundEventParams deviceFoundEventParams);
}
