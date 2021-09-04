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

package android.hardware.radio;
@VintfStability
interface IRadioResponse {
  oneway void acceptCallResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void acknowledgeIncomingGsmSmsWithPduResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void acknowledgeLastIncomingCdmaSmsResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void acknowledgeLastIncomingGsmSmsResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void acknowledgeRequest(in int serial);
  oneway void allocatePduSessionIdResponse(in android.hardware.radio.RadioResponseInfo info, in int id);
  oneway void areUiccApplicationsEnabledResponse(in android.hardware.radio.RadioResponseInfo info, in boolean enabled);
  oneway void cancelHandoverResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void cancelPendingUssdResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void changeIccPin2ForAppResponse(in android.hardware.radio.RadioResponseInfo info, in int remainingRetries);
  oneway void changeIccPinForAppResponse(in android.hardware.radio.RadioResponseInfo info, in int remainingRetries);
  oneway void conferenceResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void deactivateDataCallResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void deleteSmsOnRuimResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void deleteSmsOnSimResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void dialResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void emergencyDialResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void enableModemResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void enableUiccApplicationsResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void exitEmergencyCallbackModeResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void explicitCallTransferResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void getAllowedCarriersResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.CarrierRestrictionsWithPriority carriers, in android.hardware.radio.SimLockMultiSimPolicy multiSimPolicy);
  oneway void getAllowedNetworkTypesBitmapResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.RadioAccessFamily networkTypeBitmap);
  oneway void getAvailableBandModesResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.RadioBandMode[] bandModes);
  oneway void getAvailableNetworksResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.OperatorInfo[] networkInfos);
  oneway void getBarringInfoResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.CellIdentity cellIdentity, in android.hardware.radio.BarringInfo[] barringInfos);
  oneway void getBasebandVersionResponse(in android.hardware.radio.RadioResponseInfo info, in String version);
  oneway void getCDMASubscriptionResponse(in android.hardware.radio.RadioResponseInfo info, in String mdn, in String hSid, in String hNid, in String min, in String prl);
  oneway void getCallForwardStatusResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.CallForwardInfo[] callForwardInfos);
  oneway void getCallWaitingResponse(in android.hardware.radio.RadioResponseInfo info, in boolean enable, in int serviceClass);
  oneway void getCdmaBroadcastConfigResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.CdmaBroadcastSmsConfigInfo[] configs);
  oneway void getCdmaRoamingPreferenceResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.CdmaRoamingType type);
  oneway void getCdmaSubscriptionSourceResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.CdmaSubscriptionSource source);
  oneway void getCellInfoListResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.CellInfo[] cellInfo);
  oneway void getClipResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.ClipStatus status);
  oneway void getClirResponse(in android.hardware.radio.RadioResponseInfo info, in int n, in int m);
  oneway void getCurrentCallsResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.Call[] calls);
  oneway void getDataCallListResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.SetupDataCallResult[] dcResponse);
  oneway void getDataRegistrationStateResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.RegStateResult dataRegResponse);
  oneway void getDeviceIdentityResponse(in android.hardware.radio.RadioResponseInfo info, in String imei, in String imeisv, in String esn, in String meid);
  oneway void getFacilityLockForAppResponse(in android.hardware.radio.RadioResponseInfo info, in int response);
  oneway void getGsmBroadcastConfigResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.GsmBroadcastSmsConfigInfo[] configs);
  oneway void getHardwareConfigResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.HardwareConfig[] config);
  oneway void getIMSIForAppResponse(in android.hardware.radio.RadioResponseInfo info, in String imsi);
  oneway void getIccCardStatusResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.CardStatus cardStatus);
  oneway void getImsRegistrationStateResponse(in android.hardware.radio.RadioResponseInfo info, in boolean isRegistered, in android.hardware.radio.RadioTechnologyFamily ratFamily);
  oneway void getLastCallFailCauseResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.LastCallFailCauseInfo failCauseinfo);
  oneway void getModemActivityInfoResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.ActivityStatsInfo activityInfo);
  oneway void getModemStackStatusResponse(in android.hardware.radio.RadioResponseInfo info, in boolean isEnabled);
  oneway void getMuteResponse(in android.hardware.radio.RadioResponseInfo info, in boolean enable);
  oneway void getNeighboringCidsResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.NeighboringCell[] cells);
  oneway void getNetworkSelectionModeResponse(in android.hardware.radio.RadioResponseInfo info, in boolean manual);
  oneway void getOperatorResponse(in android.hardware.radio.RadioResponseInfo info, in String longName, in String shortName, in String numeric);
  oneway void getPreferredNetworkTypeBitmapResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.RadioAccessFamily networkTypeBitmap);
  oneway void getPreferredNetworkTypeResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.PreferredNetworkType nwType);
  oneway void getPreferredVoicePrivacyResponse(in android.hardware.radio.RadioResponseInfo info, in boolean enable);
  oneway void getRadioCapabilityResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.RadioCapability rc);
  oneway void getSignalStrengthResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.SignalStrength signalStrength);
  oneway void getSimPhonebookCapacityResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.PhonebookCapacity capacity);
  oneway void getSimPhonebookRecordsResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void getSlicingConfigResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.SlicingConfig slicingConfig);
  oneway void getSmscAddressResponse(in android.hardware.radio.RadioResponseInfo info, in String smsc);
  oneway void getSystemSelectionChannelsResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.RadioAccessSpecifier[] specifiers);
  oneway void getTTYModeResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.TtyMode mode);
  oneway void getVoiceRadioTechnologyResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.RadioTechnology rat);
  oneway void getVoiceRegistrationStateResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.RegStateResult voiceRegResponse);
  oneway void handleStkCallSetupRequestFromSimResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void hangupConnectionResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void hangupForegroundResumeBackgroundResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void hangupWaitingOrBackgroundResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void iccCloseLogicalChannelResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void iccIOForAppResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.IccIoResult iccIo);
  oneway void iccOpenLogicalChannelResponse(in android.hardware.radio.RadioResponseInfo info, in int channelId, in byte[] selectResponse);
  oneway void iccTransmitApduBasicChannelResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.IccIoResult result);
  oneway void iccTransmitApduLogicalChannelResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.IccIoResult result);
  oneway void isNrDualConnectivityEnabledResponse(in android.hardware.radio.RadioResponseInfo info, in boolean isEnabled);
  oneway void nvReadItemResponse(in android.hardware.radio.RadioResponseInfo info, in String result);
  oneway void nvResetConfigResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void nvWriteCdmaPrlResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void nvWriteItemResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void pullLceDataResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.LceDataInfo lceInfo);
  oneway void rejectCallResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void releasePduSessionIdResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void reportSmsMemoryStatusResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void reportStkServiceIsRunningResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void requestIccSimAuthenticationResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.IccIoResult result);
  oneway void requestIsimAuthenticationResponse(in android.hardware.radio.RadioResponseInfo info, in String response);
  oneway void requestShutdownResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendBurstDtmfResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendCDMAFeatureCodeResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendCdmaSmsExpectMoreResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.SendSmsResult sms);
  oneway void sendCdmaSmsResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.SendSmsResult sms);
  oneway void sendDeviceStateResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendDtmfResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendEnvelopeResponse(in android.hardware.radio.RadioResponseInfo info, in String commandResponse);
  oneway void sendEnvelopeWithStatusResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.IccIoResult iccIo);
  oneway void sendImsSmsResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.SendSmsResult sms);
  oneway void sendSMSExpectMoreResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.SendSmsResult sms);
  oneway void sendSmsExpectMoreResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.SendSmsResult sms);
  oneway void sendSmsResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.SendSmsResult sms);
  oneway void sendTerminalResponseToSimResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendUssdResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void separateConnectionResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setAllowedCarriersResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setAllowedNetworkTypesBitmapResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setBandModeResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setBarringPasswordResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCallForwardResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCallWaitingResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCarrierInfoForImsiEncryptionResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCdmaBroadcastActivationResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCdmaBroadcastConfigResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCdmaRoamingPreferenceResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCdmaSubscriptionSourceResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCellInfoListRateResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setClirResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setDataAllowedResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setDataProfileResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setDataThrottlingResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setFacilityLockForAppResponse(in android.hardware.radio.RadioResponseInfo info, in int retry);
  oneway void setGsmBroadcastActivationResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setGsmBroadcastConfigResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setIndicationFilterResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setInitialAttachApnResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setLinkCapacityReportingCriteriaResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setLocationUpdatesResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setMuteResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setNetworkSelectionModeAutomaticResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setNetworkSelectionModeManualResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setNrDualConnectivityStateResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setPreferredNetworkTypeBitmapResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setPreferredNetworkTypeResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setPreferredVoicePrivacyResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setRadioCapabilityResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.RadioCapability rc);
  oneway void setRadioPowerResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setSignalStrengthReportingCriteriaResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setSimCardPowerResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setSmscAddressResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setSuppServiceNotificationsResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setSystemSelectionChannelsResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setTTYModeResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setUiccSubscriptionResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setupDataCallResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.SetupDataCallResult dcResponse);
  oneway void startDtmfResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void startHandoverResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void startKeepaliveResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.KeepaliveStatus status);
  oneway void startLceServiceResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.LceStatusInfo statusInfo);
  oneway void startNetworkScanResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void stopDtmfResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void stopKeepaliveResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void stopLceServiceResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.LceStatusInfo statusInfo);
  oneway void stopNetworkScanResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void supplyIccPin2ForAppResponse(in android.hardware.radio.RadioResponseInfo info, in int remainingRetries);
  oneway void supplyIccPinForAppResponse(in android.hardware.radio.RadioResponseInfo info, in int remainingRetries);
  oneway void supplyIccPuk2ForAppResponse(in android.hardware.radio.RadioResponseInfo info, in int remainingRetries);
  oneway void supplyIccPukForAppResponse(in android.hardware.radio.RadioResponseInfo info, in int remainingRetries);
  oneway void supplyNetworkDepersonalizationResponse(in android.hardware.radio.RadioResponseInfo info, in int remainingRetries);
  oneway void supplySimDepersonalizationResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.PersoSubstate persoType, in int remainingRetries);
  oneway void switchWaitingOrHoldingAndActiveResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void updateSimPhonebookRecordsResponse(in android.hardware.radio.RadioResponseInfo info, in int updatedRecordIndex);
  oneway void writeSmsToRuimResponse(in android.hardware.radio.RadioResponseInfo info, in int index);
  oneway void writeSmsToSimResponse(in android.hardware.radio.RadioResponseInfo info, in int index);
}
