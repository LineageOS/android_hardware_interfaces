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

package android.hardware.radio.network;
/* @hide */
@VintfStability
interface IRadioNetwork {
  oneway void getAllowedNetworkTypesBitmap(in int serial);
  oneway void getAvailableBandModes(in int serial);
  oneway void getAvailableNetworks(in int serial);
  oneway void getBarringInfo(in int serial);
  oneway void getCdmaRoamingPreference(in int serial);
  oneway void getCellInfoList(in int serial);
  oneway void getDataRegistrationState(in int serial);
  /**
   * @deprecated Deprecated starting from Android U.
   */
  oneway void getImsRegistrationState(in int serial);
  oneway void getNetworkSelectionMode(in int serial);
  oneway void getOperator(in int serial);
  oneway void getSignalStrength(in int serial);
  oneway void getSystemSelectionChannels(in int serial);
  oneway void getVoiceRadioTechnology(in int serial);
  oneway void getVoiceRegistrationState(in int serial);
  oneway void isNrDualConnectivityEnabled(in int serial);
  oneway void responseAcknowledgement();
  oneway void setAllowedNetworkTypesBitmap(in int serial, in int networkTypeBitmap);
  oneway void setBandMode(in int serial, in android.hardware.radio.network.RadioBandMode mode);
  oneway void setBarringPassword(in int serial, in String facility, in String oldPassword, in String newPassword);
  oneway void setCdmaRoamingPreference(in int serial, in android.hardware.radio.network.CdmaRoamingType type);
  oneway void setCellInfoListRate(in int serial, in int rate);
  oneway void setIndicationFilter(in int serial, in int indicationFilter);
  oneway void setLinkCapacityReportingCriteria(in int serial, in int hysteresisMs, in int hysteresisDlKbps, in int hysteresisUlKbps, in int[] thresholdsDownlinkKbps, in int[] thresholdsUplinkKbps, in android.hardware.radio.AccessNetwork accessNetwork);
  oneway void setLocationUpdates(in int serial, in boolean enable);
  oneway void setNetworkSelectionModeAutomatic(in int serial);
  oneway void setNetworkSelectionModeManual(in int serial, in String operatorNumeric, in android.hardware.radio.AccessNetwork ran);
  oneway void setNrDualConnectivityState(in int serial, in android.hardware.radio.network.NrDualConnectivityState nrDualConnectivityState);
  oneway void setResponseFunctions(in android.hardware.radio.network.IRadioNetworkResponse radioNetworkResponse, in android.hardware.radio.network.IRadioNetworkIndication radioNetworkIndication);
  oneway void setSignalStrengthReportingCriteria(in int serial, in android.hardware.radio.network.SignalThresholdInfo[] signalThresholdInfos);
  oneway void setSuppServiceNotifications(in int serial, in boolean enable);
  oneway void setSystemSelectionChannels(in int serial, in boolean specifyChannels, in android.hardware.radio.network.RadioAccessSpecifier[] specifiers);
  oneway void startNetworkScan(in int serial, in android.hardware.radio.network.NetworkScanRequest request);
  oneway void stopNetworkScan(in int serial);
  oneway void supplyNetworkDepersonalization(in int serial, in String netPin);
  oneway void setUsageSetting(in int serial, in android.hardware.radio.network.UsageSetting usageSetting);
  oneway void getUsageSetting(in int serial);
  oneway void setEmergencyMode(int serial, in android.hardware.radio.network.EmergencyMode emcModeType);
  oneway void triggerEmergencyNetworkScan(int serial, in android.hardware.radio.network.EmergencyNetworkScanTrigger request);
  oneway void cancelEmergencyNetworkScan(int serial, boolean resetScan);
  oneway void exitEmergencyMode(in int serial);
  oneway void setNullCipherAndIntegrityEnabled(in int serial, in boolean enabled);
  oneway void isNullCipherAndIntegrityEnabled(in int serial);
  oneway void isN1ModeEnabled(in int serial);
  oneway void setN1ModeEnabled(in int serial, boolean enable);
  oneway void isCellularIdentifierTransparencyEnabled(in int serial);
  oneway void setCellularIdentifierTransparencyEnabled(in int serial, in boolean enabled);
  oneway void setSecurityAlgorithmsUpdatedEnabled(in int serial, boolean enable);
  oneway void isSecurityAlgorithmsUpdatedEnabled(in int serial);
}
