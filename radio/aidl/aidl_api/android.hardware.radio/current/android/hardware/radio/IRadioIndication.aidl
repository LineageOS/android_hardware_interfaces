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
interface IRadioIndication {
  oneway void barringInfoChanged(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.CellIdentity cellIdentity, in android.hardware.radio.BarringInfo[] barringInfos);
  oneway void callRing(in android.hardware.radio.RadioIndicationType type, in boolean isGsm, in android.hardware.radio.CdmaSignalInfoRecord record);
  oneway void callStateChanged(in android.hardware.radio.RadioIndicationType type);
  oneway void carrierInfoForImsiEncryption(in android.hardware.radio.RadioIndicationType info);
  oneway void cdmaCallWaiting(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.CdmaCallWaiting callWaitingRecord);
  oneway void cdmaInfoRec(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.CdmaInformationRecords records);
  oneway void cdmaNewSms(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.CdmaSmsMessage msg);
  oneway void cdmaOtaProvisionStatus(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.CdmaOtaProvisionStatus status);
  oneway void cdmaPrlChanged(in android.hardware.radio.RadioIndicationType type, in int version);
  oneway void cdmaRuimSmsStorageFull(in android.hardware.radio.RadioIndicationType type);
  oneway void cdmaSubscriptionSourceChanged(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.CdmaSubscriptionSource cdmaSource);
  oneway void cellInfoList(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.CellInfo[] records);
  oneway void currentEmergencyNumberList(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.EmergencyNumber[] emergencyNumberList);
  oneway void currentLinkCapacityEstimate(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.LinkCapacityEstimate lce);
  oneway void currentPhysicalChannelConfigs(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.PhysicalChannelConfig[] configs);
  oneway void currentSignalStrength(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.SignalStrength signalStrength);
  oneway void dataCallListChanged(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.SetupDataCallResult[] dcList);
  oneway void enterEmergencyCallbackMode(in android.hardware.radio.RadioIndicationType type);
  oneway void exitEmergencyCallbackMode(in android.hardware.radio.RadioIndicationType type);
  oneway void hardwareConfigChanged(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.HardwareConfig[] configs);
  oneway void imsNetworkStateChanged(in android.hardware.radio.RadioIndicationType type);
  oneway void indicateRingbackTone(in android.hardware.radio.RadioIndicationType type, in boolean start);
  oneway void keepaliveStatus(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.KeepaliveStatus status);
  oneway void lceData(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.LceDataInfo lce);
  oneway void modemReset(in android.hardware.radio.RadioIndicationType type, in String reason);
  oneway void networkScanResult(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.NetworkScanResult result);
  oneway void networkStateChanged(in android.hardware.radio.RadioIndicationType type);
  oneway void newBroadcastSms(in android.hardware.radio.RadioIndicationType type, in byte[] data);
  oneway void newSms(in android.hardware.radio.RadioIndicationType type, in byte[] pdu);
  oneway void newSmsOnSim(in android.hardware.radio.RadioIndicationType type, in int recordNumber);
  oneway void newSmsStatusReport(in android.hardware.radio.RadioIndicationType type, in byte[] pdu);
  oneway void nitzTimeReceived(in android.hardware.radio.RadioIndicationType type, in String nitzTime, in long receivedTime);
  oneway void onSupplementaryServiceIndication(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.StkCcUnsolSsResult ss);
  oneway void onUssd(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.UssdModeType modeType, in String msg);
  oneway void pcoData(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.PcoDataInfo pco);
  oneway void radioCapabilityIndication(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.RadioCapability rc);
  oneway void radioStateChanged(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.RadioState radioState);
  oneway void registrationFailed(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.CellIdentity cellIdentity, in String chosenPlmn, in android.hardware.radio.Domain domain, in int causeCode, in int additionalCauseCode);
  oneway void resendIncallMute(in android.hardware.radio.RadioIndicationType type);
  oneway void restrictedStateChanged(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.PhoneRestrictedState state);
  oneway void rilConnected(in android.hardware.radio.RadioIndicationType type);
  oneway void simPhonebookChanged(in android.hardware.radio.RadioIndicationType type);
  oneway void simPhonebookRecordsReceived(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.PbReceivedStatus status, in android.hardware.radio.PhonebookRecordInfo[] records);
  oneway void simRefresh(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.SimRefreshResult refreshResult);
  oneway void simSmsStorageFull(in android.hardware.radio.RadioIndicationType type);
  oneway void simStatusChanged(in android.hardware.radio.RadioIndicationType type);
  oneway void srvccStateNotify(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.SrvccState state);
  oneway void stkCallControlAlphaNotify(in android.hardware.radio.RadioIndicationType type, in String alpha);
  oneway void stkCallSetup(in android.hardware.radio.RadioIndicationType type, in long timeout);
  oneway void stkEventNotify(in android.hardware.radio.RadioIndicationType type, in String cmd);
  oneway void stkProactiveCommand(in android.hardware.radio.RadioIndicationType type, in String cmd);
  oneway void stkSessionEnd(in android.hardware.radio.RadioIndicationType type);
  oneway void subscriptionStatusChanged(in android.hardware.radio.RadioIndicationType type, in boolean activate);
  oneway void suppSvcNotify(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.SuppSvcNotification suppSvc);
  oneway void uiccApplicationsEnablementChanged(in android.hardware.radio.RadioIndicationType type, in boolean enabled);
  oneway void unthrottleApn(in android.hardware.radio.RadioIndicationType type, in String apn);
  oneway void voiceRadioTechChanged(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.RadioTechnology rat);
}
