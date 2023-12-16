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
interface ISupplicantStaIface {
  int addDppPeerUri(in String uri);
  int addExtRadioWork(in String name, in int freqInMhz, in int timeoutInSec);
  @PropagateAllowBlocking android.hardware.wifi.supplicant.ISupplicantStaNetwork addNetwork();
  void addRxFilter(in android.hardware.wifi.supplicant.RxFilterType type);
  void cancelWps();
  void disconnect();
  void enableAutoReconnect(in boolean enable);
  void filsHlpAddRequest(in byte[] dst_mac, in byte[] pkt);
  void filsHlpFlushRequest();
  android.hardware.wifi.supplicant.DppResponderBootstrapInfo generateDppBootstrapInfoForResponder(in byte[] macAddress, in String deviceInfo, in android.hardware.wifi.supplicant.DppCurve curve);
  void generateSelfDppConfiguration(in String ssid, in byte[] privEcKey);
  android.hardware.wifi.supplicant.ConnectionCapabilities getConnectionCapabilities();
  android.hardware.wifi.supplicant.MloLinksInfo getConnectionMloLinksInfo();
  android.hardware.wifi.supplicant.KeyMgmtMask getKeyMgmtCapabilities();
  byte[] getMacAddress();
  String getName();
  @PropagateAllowBlocking android.hardware.wifi.supplicant.ISupplicantStaNetwork getNetwork(in int id);
  android.hardware.wifi.supplicant.IfaceType getType();
  android.hardware.wifi.supplicant.WpaDriverCapabilitiesMask getWpaDriverCapabilities();
  void initiateAnqpQuery(in byte[] macAddress, in android.hardware.wifi.supplicant.AnqpInfoId[] infoElements, in android.hardware.wifi.supplicant.Hs20AnqpSubtypes[] subTypes);
  /**
   * @deprecated No longer in use.
   */
  void initiateHs20IconQuery(in byte[] macAddress, in String fileName);
  void initiateTdlsDiscover(in byte[] macAddress);
  void initiateTdlsSetup(in byte[] macAddress);
  void initiateTdlsTeardown(in byte[] macAddress);
  void initiateVenueUrlAnqpQuery(in byte[] macAddress);
  int[] listNetworks();
  void reassociate();
  void reconnect();
  void registerCallback(in android.hardware.wifi.supplicant.ISupplicantStaIfaceCallback callback);
  void setQosPolicyFeatureEnabled(in boolean enable);
  void sendQosPolicyResponse(in int qosPolicyRequestId, in boolean morePolicies, in android.hardware.wifi.supplicant.QosPolicyStatus[] qosPolicyStatusList);
  void removeAllQosPolicies();
  void removeDppUri(in int id);
  void removeExtRadioWork(in int id);
  void removeNetwork(in int id);
  void removeRxFilter(in android.hardware.wifi.supplicant.RxFilterType type);
  void setBtCoexistenceMode(in android.hardware.wifi.supplicant.BtCoexistenceMode mode);
  void setBtCoexistenceScanModeEnabled(in boolean enable);
  void setCountryCode(in byte[] code);
  void setExternalSim(in boolean useExternalSim);
  void setMboCellularDataStatus(in boolean available);
  void setPowerSave(in boolean enable);
  void setSuspendModeEnabled(in boolean enable);
  void setWpsConfigMethods(in android.hardware.wifi.supplicant.WpsConfigMethods configMethods);
  void setWpsDeviceName(in String name);
  void setWpsDeviceType(in byte[] type);
  void setWpsManufacturer(in String manufacturer);
  void setWpsModelName(in String modelName);
  void setWpsModelNumber(in String modelNumber);
  void setWpsSerialNumber(in String serialNumber);
  byte[] startDppConfiguratorInitiator(in int peerBootstrapId, in int ownBootstrapId, in String ssid, in String password, in String psk, in android.hardware.wifi.supplicant.DppNetRole netRole, in android.hardware.wifi.supplicant.DppAkm securityAkm, in byte[] privEcKey);
  void startDppEnrolleeInitiator(in int peerBootstrapId, in int ownBootstrapId);
  void startDppEnrolleeResponder(in int listenChannel);
  void startRxFilter();
  void startWpsPbc(in byte[] bssid);
  String startWpsPinDisplay(in byte[] bssid);
  void startWpsPinKeypad(in String pin);
  void startWpsRegistrar(in byte[] bssid, in String pin);
  void stopDppInitiator();
  void stopDppResponder(in int ownBootstrapId);
  void stopRxFilter();
  android.hardware.wifi.supplicant.SignalPollResult[] getSignalPollResults();
  android.hardware.wifi.supplicant.QosPolicyScsRequestStatus[] addQosPolicyRequestForScs(in android.hardware.wifi.supplicant.QosPolicyScsData[] qosPolicyData);
  android.hardware.wifi.supplicant.QosPolicyScsRequestStatus[] removeQosPolicyForScs(in byte[] scsPolicyIds);
  void configureMscs(in android.hardware.wifi.supplicant.MscsParams params);
  void disableMscs();
  const int MAX_POLICIES_PER_QOS_SCS_REQUEST = 16;
}
