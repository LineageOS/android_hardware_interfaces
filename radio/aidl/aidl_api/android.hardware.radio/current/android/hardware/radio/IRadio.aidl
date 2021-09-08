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
interface IRadio {
  oneway void acceptCall(in int serial);
  oneway void acknowledgeIncomingGsmSmsWithPdu(in int serial, in boolean success, in String ackPdu);
  oneway void acknowledgeLastIncomingCdmaSms(in int serial, in android.hardware.radio.CdmaSmsAck smsAck);
  oneway void acknowledgeLastIncomingGsmSms(in int serial, in boolean success, in android.hardware.radio.SmsAcknowledgeFailCause cause);
  oneway void allocatePduSessionId(in int serial);
  oneway void areUiccApplicationsEnabled(in int serial);
  oneway void cancelHandover(in int serial, in int callId);
  oneway void cancelPendingUssd(in int serial);
  oneway void changeIccPin2ForApp(in int serial, in String oldPin2, in String newPin2, in String aid);
  oneway void changeIccPinForApp(in int serial, in String oldPin, in String newPin, in String aid);
  oneway void conference(in int serial);
  oneway void deactivateDataCall(in int serial, in int cid, in android.hardware.radio.DataRequestReason reason);
  oneway void deleteSmsOnRuim(in int serial, in int index);
  oneway void deleteSmsOnSim(in int serial, in int index);
  oneway void dial(in int serial, in android.hardware.radio.Dial dialInfo);
  oneway void emergencyDial(in int serial, in android.hardware.radio.Dial dialInfo, in android.hardware.radio.EmergencyServiceCategory categories, in String[] urns, in android.hardware.radio.EmergencyCallRouting routing, in boolean hasKnownUserIntentEmergency, in boolean isTesting);
  oneway void enableModem(in int serial, in boolean on);
  oneway void enableUiccApplications(in int serial, in boolean enable);
  oneway void exitEmergencyCallbackMode(in int serial);
  oneway void explicitCallTransfer(in int serial);
  oneway void getAllowedCarriers(in int serial);
  oneway void getAllowedNetworkTypesBitmap(in int serial);
  oneway void getAvailableBandModes(in int serial);
  oneway void getAvailableNetworks(in int serial);
  oneway void getBarringInfo(in int serial);
  oneway void getBasebandVersion(in int serial);
  oneway void getCDMASubscription(in int serial);
  oneway void getCallForwardStatus(in int serial, in android.hardware.radio.CallForwardInfo callInfo);
  oneway void getCallWaiting(in int serial, in int serviceClass);
  oneway void getCdmaBroadcastConfig(in int serial);
  oneway void getCdmaRoamingPreference(in int serial);
  oneway void getCdmaSubscriptionSource(in int serial);
  oneway void getCellInfoList(in int serial);
  oneway void getClip(in int serial);
  oneway void getClir(in int serial);
  oneway void getCurrentCalls(in int serial);
  oneway void getDataCallList(in int serial);
  oneway void getDataRegistrationState(in int serial);
  oneway void getDeviceIdentity(in int serial);
  oneway void getFacilityLockForApp(in int serial, in String facility, in String password, in int serviceClass, in String appId);
  oneway void getGsmBroadcastConfig(in int serial);
  oneway void getHardwareConfig(in int serial);
  oneway void getIccCardStatus(in int serial);
  oneway void getImsRegistrationState(in int serial);
  oneway void getImsiForApp(in int serial, in String aid);
  oneway void getLastCallFailCause(in int serial);
  oneway void getModemActivityInfo(in int serial);
  oneway void getModemStackStatus(in int serial);
  oneway void getMute(in int serial);
  oneway void getNeighboringCids(in int serial);
  oneway void getNetworkSelectionMode(in int serial);
  oneway void getOperator(in int serial);
  oneway void getPreferredNetworkType(in int serial);
  oneway void getPreferredNetworkTypeBitmap(in int serial);
  oneway void getPreferredVoicePrivacy(in int serial);
  oneway void getRadioCapability(in int serial);
  oneway void getSignalStrength(in int serial);
  oneway void getSimPhonebookCapacity(in int serial);
  oneway void getSimPhonebookRecords(in int serial);
  oneway void getSlicingConfig(in int serial);
  oneway void getSmscAddress(in int serial);
  oneway void getSystemSelectionChannels(in int serial);
  oneway void getTTYMode(in int serial);
  oneway void getVoiceRadioTechnology(in int serial);
  oneway void getVoiceRegistrationState(in int serial);
  oneway void handleStkCallSetupRequestFromSim(in int serial, in boolean accept);
  oneway void hangup(in int serial, in int gsmIndex);
  oneway void hangupForegroundResumeBackground(in int serial);
  oneway void hangupWaitingOrBackground(in int serial);
  oneway void iccCloseLogicalChannel(in int serial, in int channelId);
  oneway void iccIOForApp(in int serial, in android.hardware.radio.IccIo iccIo);
  oneway void iccOpenLogicalChannel(in int serial, in String aid, in int p2);
  oneway void iccTransmitApduBasicChannel(in int serial, in android.hardware.radio.SimApdu message);
  oneway void iccTransmitApduLogicalChannel(in int serial, in android.hardware.radio.SimApdu message);
  oneway void isNrDualConnectivityEnabled(in int serial);
  oneway void nvReadItem(in int serial, in android.hardware.radio.NvItem itemId);
  oneway void nvResetConfig(in int serial, in android.hardware.radio.ResetNvType resetType);
  oneway void nvWriteCdmaPrl(in int serial, in byte[] prl);
  oneway void nvWriteItem(in int serial, in android.hardware.radio.NvWriteItem item);
  oneway void rejectCall(in int serial);
  oneway void releasePduSessionId(in int serial, in int id);
  oneway void reportSmsMemoryStatus(in int serial, in boolean available);
  oneway void reportStkServiceIsRunning(in int serial);
  oneway void requestIccSimAuthentication(in int serial, in int authContext, in String authData, in String aid);
  oneway void requestIsimAuthentication(in int serial, in String challenge);
  oneway void requestShutdown(in int serial);
  oneway void responseAcknowledgement();
  oneway void sendBurstDtmf(in int serial, in String dtmf, in int on, in int off);
  oneway void sendCDMAFeatureCode(in int serial, in String featureCode);
  oneway void sendCdmaSms(in int serial, in android.hardware.radio.CdmaSmsMessage sms);
  oneway void sendCdmaSmsExpectMore(in int serial, in android.hardware.radio.CdmaSmsMessage sms);
  oneway void sendDeviceState(in int serial, in android.hardware.radio.DeviceStateType deviceStateType, in boolean state);
  oneway void sendDtmf(in int serial, in String s);
  oneway void sendEnvelope(in int serial, in String command);
  oneway void sendEnvelopeWithStatus(in int serial, in String contents);
  oneway void sendImsSms(in int serial, in android.hardware.radio.ImsSmsMessage message);
  oneway void sendSms(in int serial, in android.hardware.radio.GsmSmsMessage message);
  oneway void sendSmsExpectMore(in int serial, in android.hardware.radio.GsmSmsMessage message);
  oneway void sendTerminalResponseToSim(in int serial, in String commandResponse);
  oneway void sendUssd(in int serial, in String ussd);
  oneway void separateConnection(in int serial, in int gsmIndex);
  oneway void setAllowedCarriers(in int serial, in android.hardware.radio.CarrierRestrictions carriers, in android.hardware.radio.SimLockMultiSimPolicy multiSimPolicy);
  oneway void setAllowedNetworkTypesBitmap(in int serial, in android.hardware.radio.RadioAccessFamily networkTypeBitmap);
  oneway void setBandMode(in int serial, in android.hardware.radio.RadioBandMode mode);
  oneway void setBarringPassword(in int serial, in String facility, in String oldPassword, in String newPassword);
  oneway void setCallForward(in int serial, in android.hardware.radio.CallForwardInfo callInfo);
  oneway void setCallWaiting(in int serial, in boolean enable, in int serviceClass);
  oneway void setCarrierInfoForImsiEncryption(in int serial, in android.hardware.radio.ImsiEncryptionInfo imsiEncryptionInfo);
  oneway void setCdmaBroadcastActivation(in int serial, in boolean activate);
  oneway void setCdmaBroadcastConfig(in int serial, in android.hardware.radio.CdmaBroadcastSmsConfigInfo[] configInfo);
  oneway void setCdmaRoamingPreference(in int serial, in android.hardware.radio.CdmaRoamingType type);
  oneway void setCdmaSubscriptionSource(in int serial, in android.hardware.radio.CdmaSubscriptionSource cdmaSub);
  oneway void setCellInfoListRate(in int serial, in int rate);
  oneway void setClir(in int serial, in int status);
  oneway void setDataAllowed(in int serial, in boolean allow);
  oneway void setDataProfile(in int serial, in android.hardware.radio.DataProfileInfo[] profiles);
  oneway void setDataThrottling(in int serial, in android.hardware.radio.DataThrottlingAction dataThrottlingAction, in long completionDurationMillis);
  oneway void setFacilityLockForApp(in int serial, in String facility, in boolean lockState, in String password, in int serviceClass, in String appId);
  oneway void setGsmBroadcastActivation(in int serial, in boolean activate);
  oneway void setGsmBroadcastConfig(in int serial, in android.hardware.radio.GsmBroadcastSmsConfigInfo[] configInfo);
  oneway void setIndicationFilter(in int serial, in android.hardware.radio.IndicationFilter indicationFilter);
  oneway void setInitialAttachApn(in int serial, in android.hardware.radio.DataProfileInfo dataProfileInfo);
  oneway void setLinkCapacityReportingCriteria(in int serial, in int hysteresisMs, in int hysteresisDlKbps, in int hysteresisUlKbps, in int[] thresholdsDownlinkKbps, in int[] thresholdsUplinkKbps, in android.hardware.radio.AccessNetwork accessNetwork);
  oneway void setLocationUpdates(in int serial, in boolean enable);
  oneway void setMute(in int serial, in boolean enable);
  oneway void setNetworkSelectionModeAutomatic(in int serial);
  oneway void setNetworkSelectionModeManual(in int serial, in String operatorNumeric, in android.hardware.radio.RadioAccessNetworks ran);
  oneway void setNrDualConnectivityState(in int serial, in android.hardware.radio.NrDualConnectivityState nrDualConnectivityState);
  oneway void setPreferredNetworkType(in int serial, in android.hardware.radio.PreferredNetworkType nwType);
  oneway void setPreferredNetworkTypeBitmap(in int serial, in android.hardware.radio.RadioAccessFamily networkTypeBitmap);
  oneway void setPreferredVoicePrivacy(in int serial, in boolean enable);
  oneway void setRadioCapability(in int serial, in android.hardware.radio.RadioCapability rc);
  oneway void setRadioPower(in int serial, in boolean powerOn, in boolean forEmergencyCall, in boolean preferredForEmergencyCall);
  oneway void setResponseFunctions(in android.hardware.radio.IRadioResponse radioResponse, in android.hardware.radio.IRadioIndication radioIndication);
  oneway void setSignalStrengthReportingCriteria(in int serial, in android.hardware.radio.SignalThresholdInfo signalThresholdInfo, in android.hardware.radio.AccessNetwork accessNetwork);
  oneway void setSimCardPower(in int serial, in android.hardware.radio.CardPowerState powerUp);
  oneway void setSmscAddress(in int serial, in String smsc);
  oneway void setSuppServiceNotifications(in int serial, in boolean enable);
  oneway void setSystemSelectionChannels(in int serial, in boolean specifyChannels, in android.hardware.radio.RadioAccessSpecifier[] specifiers);
  oneway void setTTYMode(in int serial, in android.hardware.radio.TtyMode mode);
  oneway void setUiccSubscription(in int serial, in android.hardware.radio.SelectUiccSub uiccSub);
  oneway void setupDataCall(in int serial, in android.hardware.radio.AccessNetwork accessNetwork, in android.hardware.radio.DataProfileInfo dataProfileInfo, in boolean roamingAllowed, in android.hardware.radio.DataRequestReason reason, in android.hardware.radio.LinkAddress[] addresses, in String[] dnses, in int pduSessionId, in @nullable android.hardware.radio.SliceInfo sliceInfo, in @nullable android.hardware.radio.TrafficDescriptor trafficDescriptor, in boolean matchAllRuleAllowed);
  oneway void startDtmf(in int serial, in String s);
  oneway void startHandover(in int serial, in int callId);
  oneway void startKeepalive(in int serial, in android.hardware.radio.KeepaliveRequest keepalive);
  oneway void startNetworkScan(in int serial, in android.hardware.radio.NetworkScanRequest request);
  oneway void stopDtmf(in int serial);
  oneway void stopKeepalive(in int serial, in int sessionHandle);
  oneway void stopNetworkScan(in int serial);
  oneway void supplyIccPin2ForApp(in int serial, in String pin2, in String aid);
  oneway void supplyIccPinForApp(in int serial, in String pin, in String aid);
  oneway void supplyIccPuk2ForApp(in int serial, in String puk2, in String pin2, in String aid);
  oneway void supplyIccPukForApp(in int serial, in String puk, in String pin, in String aid);
  oneway void supplyNetworkDepersonalization(in int serial, in String netPin);
  oneway void supplySimDepersonalization(in int serial, in android.hardware.radio.PersoSubstate persoType, in String controlKey);
  oneway void switchWaitingOrHoldingAndActive(in int serial);
  oneway void updateSimPhonebookRecords(in int serial, in android.hardware.radio.PhonebookRecordInfo recordInfo);
  oneway void writeSmsToRuim(in int serial, in android.hardware.radio.CdmaSmsWriteArgs cdmaSms);
  oneway void writeSmsToSim(in int serial, in android.hardware.radio.SmsWriteArgs smsWriteArgs);
}
