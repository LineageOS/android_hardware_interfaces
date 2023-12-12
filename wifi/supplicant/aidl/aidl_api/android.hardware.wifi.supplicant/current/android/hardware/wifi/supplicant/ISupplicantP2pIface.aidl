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
interface ISupplicantP2pIface {
  void addBonjourService(in byte[] query, in byte[] response);
  void addGroup(in boolean persistent, in int persistentNetworkId);
  void addGroupWithConfig(in byte[] ssid, in String pskPassphrase, in boolean persistent, in int freq, in byte[] peerAddress, in boolean joinExistingGroup);
  @PropagateAllowBlocking android.hardware.wifi.supplicant.ISupplicantP2pNetwork addNetwork();
  void addUpnpService(in int version, in String serviceName);
  void cancelConnect();
  void cancelServiceDiscovery(in long identifier);
  void cancelWps(in String groupIfName);
  void configureExtListen(in int periodInMillis, in int intervalInMillis);
  /**
   * @deprecated This method is deprecated from AIDL v3, newer HALs should use connectWithParams.
   */
  String connect(in byte[] peerAddress, in android.hardware.wifi.supplicant.WpsProvisionMethod provisionMethod, in String preSelectedPin, in boolean joinExistingGroup, in boolean persistent, in int goIntent);
  byte[] createNfcHandoverRequestMessage();
  byte[] createNfcHandoverSelectMessage();
  void enableWfd(in boolean enable);
  /**
   * @deprecated This method is deprecated from AIDL v3, newer HALs should use findWithParams.
   */
  void find(in int timeoutInSec);
  void flush();
  void flushServices();
  byte[] getDeviceAddress();
  boolean getEdmg();
  android.hardware.wifi.supplicant.P2pGroupCapabilityMask getGroupCapability(in byte[] peerAddress);
  String getName();
  @PropagateAllowBlocking android.hardware.wifi.supplicant.ISupplicantP2pNetwork getNetwork(in int id);
  byte[] getSsid(in byte[] peerAddress);
  android.hardware.wifi.supplicant.IfaceType getType();
  void invite(in String groupIfName, in byte[] goDeviceAddress, in byte[] peerAddress);
  int[] listNetworks();
  void provisionDiscovery(in byte[] peerAddress, in android.hardware.wifi.supplicant.WpsProvisionMethod provisionMethod);
  void registerCallback(in android.hardware.wifi.supplicant.ISupplicantP2pIfaceCallback callback);
  void reinvoke(in int persistentNetworkId, in byte[] peerAddress);
  void reject(in byte[] peerAddress);
  void removeBonjourService(in byte[] query);
  void removeGroup(in String groupIfName);
  void removeNetwork(in int id);
  void removeUpnpService(in int version, in String serviceName);
  void reportNfcHandoverInitiation(in byte[] select);
  void reportNfcHandoverResponse(in byte[] request);
  long requestServiceDiscovery(in byte[] peerAddress, in byte[] query);
  void saveConfig();
  void setDisallowedFrequencies(in android.hardware.wifi.supplicant.FreqRange[] ranges);
  void setEdmg(in boolean enable);
  void setGroupIdle(in String groupIfName, in int timeoutInSec);
  void setListenChannel(in int channel, in int operatingClass);
  void setMacRandomization(in boolean enable);
  void setMiracastMode(in android.hardware.wifi.supplicant.MiracastMode mode);
  void setPowerSave(in String groupIfName, in boolean enable);
  void setSsidPostfix(in byte[] postfix);
  void setWfdDeviceInfo(in byte[] info);
  void setWfdR2DeviceInfo(in byte[] info);
  void removeClient(in byte[] peerAddress, in boolean isLegacyClient);
  void setWpsConfigMethods(in android.hardware.wifi.supplicant.WpsConfigMethods configMethods);
  void setWpsDeviceName(in String name);
  void setWpsDeviceType(in byte[] type);
  void setWpsManufacturer(in String manufacturer);
  void setWpsModelName(in String modelName);
  void setWpsModelNumber(in String modelNumber);
  void setWpsSerialNumber(in String serialNumber);
  void startWpsPbc(in String groupIfName, in byte[] bssid);
  String startWpsPinDisplay(in String groupIfName, in byte[] bssid);
  void startWpsPinKeypad(in String groupIfName, in String pin);
  void stopFind();
  /**
   * @deprecated This method is deprecated from AIDL v3, newer HALs should use findWithParams.
   */
  void findOnSocialChannels(in int timeoutInSec);
  /**
   * @deprecated This method is deprecated from AIDL v3, newer HALs should use findWithParams.
   */
  void findOnSpecificFrequency(in int freqInHz, in int timeoutInSec);
  void setVendorElements(in android.hardware.wifi.supplicant.P2pFrameTypeMask frameTypeMask, in byte[] vendorElemBytes);
  void configureEapolIpAddressAllocationParams(in int ipAddressGo, in int ipAddressMask, in int ipAddressStart, in int ipAddressEnd);
  String connectWithParams(in android.hardware.wifi.supplicant.P2pConnectInfo connectInfo);
  void findWithParams(in android.hardware.wifi.supplicant.P2pDiscoveryInfo discoveryInfo);
}
