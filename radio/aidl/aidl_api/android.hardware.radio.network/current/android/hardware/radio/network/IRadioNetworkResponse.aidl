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
interface IRadioNetworkResponse {
  oneway void acknowledgeRequest(in int serial);
  oneway void getAllowedNetworkTypesBitmapResponse(in android.hardware.radio.RadioResponseInfo info, in int networkTypeBitmap);
  oneway void getAvailableBandModesResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.RadioBandMode[] bandModes);
  oneway void getAvailableNetworksResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.OperatorInfo[] networkInfos);
  oneway void getBarringInfoResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.CellIdentity cellIdentity, in android.hardware.radio.network.BarringInfo[] barringInfos);
  oneway void getCdmaRoamingPreferenceResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.CdmaRoamingType type);
  oneway void getCellInfoListResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.CellInfo[] cellInfo);
  oneway void getDataRegistrationStateResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.RegStateResult dataRegResponse);
  /**
   * @deprecated Deprecated starting from Android U.
   */
  oneway void getImsRegistrationStateResponse(in android.hardware.radio.RadioResponseInfo info, in boolean isRegistered, in android.hardware.radio.RadioTechnologyFamily ratFamily);
  oneway void getNetworkSelectionModeResponse(in android.hardware.radio.RadioResponseInfo info, in boolean manual);
  oneway void getOperatorResponse(in android.hardware.radio.RadioResponseInfo info, in String longName, in String shortName, in String numeric);
  oneway void getSignalStrengthResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.SignalStrength signalStrength);
  oneway void getSystemSelectionChannelsResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.RadioAccessSpecifier[] specifiers);
  oneway void getVoiceRadioTechnologyResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.RadioTechnology rat);
  oneway void getVoiceRegistrationStateResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.RegStateResult voiceRegResponse);
  oneway void isNrDualConnectivityEnabledResponse(in android.hardware.radio.RadioResponseInfo info, in boolean isEnabled);
  oneway void setAllowedNetworkTypesBitmapResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setBandModeResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setBarringPasswordResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCdmaRoamingPreferenceResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCellInfoListRateResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setIndicationFilterResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setLinkCapacityReportingCriteriaResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setLocationUpdatesResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setNetworkSelectionModeAutomaticResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setNetworkSelectionModeManualResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setNrDualConnectivityStateResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setSignalStrengthReportingCriteriaResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setSuppServiceNotificationsResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setSystemSelectionChannelsResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void startNetworkScanResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void stopNetworkScanResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void supplyNetworkDepersonalizationResponse(in android.hardware.radio.RadioResponseInfo info, in int remainingRetries);
  oneway void setUsageSettingResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void getUsageSettingResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.UsageSetting usageSetting);
  oneway void setEmergencyModeResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.network.EmergencyRegResult regState);
  oneway void triggerEmergencyNetworkScanResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void exitEmergencyModeResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void cancelEmergencyNetworkScanResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setNullCipherAndIntegrityEnabledResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void isNullCipherAndIntegrityEnabledResponse(in android.hardware.radio.RadioResponseInfo info, in boolean isEnabled);
  oneway void isN1ModeEnabledResponse(in android.hardware.radio.RadioResponseInfo info, boolean isEnabled);
  oneway void setN1ModeEnabledResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void isCellularIdentifierTransparencyEnabledResponse(in android.hardware.radio.RadioResponseInfo info, boolean isEnabled);
  oneway void setCellularIdentifierTransparencyEnabledResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setSecurityAlgorithmsUpdatedEnabledResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void isSecurityAlgorithmsUpdatedEnabledResponse(in android.hardware.radio.RadioResponseInfo info, in boolean isEnabled);
}
