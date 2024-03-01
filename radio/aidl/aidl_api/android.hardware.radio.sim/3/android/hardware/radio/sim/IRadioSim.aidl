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

package android.hardware.radio.sim;
/* @hide */
@VintfStability
interface IRadioSim {
  oneway void areUiccApplicationsEnabled(in int serial);
  oneway void changeIccPin2ForApp(in int serial, in String oldPin2, in String newPin2, in String aid);
  oneway void changeIccPinForApp(in int serial, in String oldPin, in String newPin, in String aid);
  oneway void enableUiccApplications(in int serial, in boolean enable);
  oneway void getAllowedCarriers(in int serial);
  oneway void getCdmaSubscription(in int serial);
  oneway void getCdmaSubscriptionSource(in int serial);
  oneway void getFacilityLockForApp(in int serial, in String facility, in String password, in int serviceClass, in String appId);
  oneway void getIccCardStatus(in int serial);
  oneway void getImsiForApp(in int serial, in String aid);
  oneway void getSimPhonebookCapacity(in int serial);
  oneway void getSimPhonebookRecords(in int serial);
  /**
   * @deprecated use iccCloseLogicalChannelWithSessionInfo instead.
   */
  oneway void iccCloseLogicalChannel(in int serial, in int channelId);
  oneway void iccIoForApp(in int serial, in android.hardware.radio.sim.IccIo iccIo);
  oneway void iccOpenLogicalChannel(in int serial, in String aid, in int p2);
  oneway void iccTransmitApduBasicChannel(in int serial, in android.hardware.radio.sim.SimApdu message);
  oneway void iccTransmitApduLogicalChannel(in int serial, in android.hardware.radio.sim.SimApdu message);
  oneway void reportStkServiceIsRunning(in int serial);
  oneway void requestIccSimAuthentication(in int serial, in int authContext, in String authData, in String aid);
  oneway void responseAcknowledgement();
  oneway void sendEnvelope(in int serial, in String contents);
  oneway void sendEnvelopeWithStatus(in int serial, in String contents);
  oneway void sendTerminalResponseToSim(in int serial, in String contents);
  oneway void setAllowedCarriers(in int serial, in android.hardware.radio.sim.CarrierRestrictions carriers, in android.hardware.radio.sim.SimLockMultiSimPolicy multiSimPolicy);
  oneway void setCarrierInfoForImsiEncryption(in int serial, in android.hardware.radio.sim.ImsiEncryptionInfo imsiEncryptionInfo);
  oneway void setCdmaSubscriptionSource(in int serial, in android.hardware.radio.sim.CdmaSubscriptionSource cdmaSub);
  oneway void setFacilityLockForApp(in int serial, in String facility, in boolean lockState, in String password, in int serviceClass, in String appId);
  oneway void setResponseFunctions(in android.hardware.radio.sim.IRadioSimResponse radioSimResponse, in android.hardware.radio.sim.IRadioSimIndication radioSimIndication);
  oneway void setSimCardPower(in int serial, in android.hardware.radio.sim.CardPowerState powerUp);
  oneway void setUiccSubscription(in int serial, in android.hardware.radio.sim.SelectUiccSub uiccSub);
  oneway void supplyIccPin2ForApp(in int serial, in String pin2, in String aid);
  oneway void supplyIccPinForApp(in int serial, in String pin, in String aid);
  oneway void supplyIccPuk2ForApp(in int serial, in String puk2, in String pin2, in String aid);
  oneway void supplyIccPukForApp(in int serial, in String puk, in String pin, in String aid);
  oneway void supplySimDepersonalization(in int serial, in android.hardware.radio.sim.PersoSubstate persoType, in String controlKey);
  oneway void updateSimPhonebookRecords(in int serial, in android.hardware.radio.sim.PhonebookRecordInfo recordInfo);
  oneway void iccCloseLogicalChannelWithSessionInfo(in int serial, in android.hardware.radio.sim.SessionInfo sessionInfo);
}
