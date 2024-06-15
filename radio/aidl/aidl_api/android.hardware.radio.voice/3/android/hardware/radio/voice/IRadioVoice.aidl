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

package android.hardware.radio.voice;
/* @hide */
@VintfStability
interface IRadioVoice {
  oneway void acceptCall(in int serial);
  oneway void cancelPendingUssd(in int serial);
  oneway void conference(in int serial);
  oneway void dial(in int serial, in android.hardware.radio.voice.Dial dialInfo);
  oneway void emergencyDial(in int serial, in android.hardware.radio.voice.Dial dialInfo, in int categories, in String[] urns, in android.hardware.radio.voice.EmergencyCallRouting routing, in boolean hasKnownUserIntentEmergency, in boolean isTesting);
  oneway void exitEmergencyCallbackMode(in int serial);
  oneway void explicitCallTransfer(in int serial);
  oneway void getCallForwardStatus(in int serial, in android.hardware.radio.voice.CallForwardInfo callInfo);
  oneway void getCallWaiting(in int serial, in int serviceClass);
  oneway void getClip(in int serial);
  oneway void getClir(in int serial);
  oneway void getCurrentCalls(in int serial);
  oneway void getLastCallFailCause(in int serial);
  oneway void getMute(in int serial);
  oneway void getPreferredVoicePrivacy(in int serial);
  oneway void getTtyMode(in int serial);
  oneway void handleStkCallSetupRequestFromSim(in int serial, in boolean accept);
  oneway void hangup(in int serial, in int gsmIndex);
  oneway void hangupForegroundResumeBackground(in int serial);
  oneway void hangupWaitingOrBackground(in int serial);
  oneway void isVoNrEnabled(in int serial);
  oneway void rejectCall(in int serial);
  oneway void responseAcknowledgement();
  oneway void sendBurstDtmf(in int serial, in String dtmf, in int on, in int off);
  oneway void sendCdmaFeatureCode(in int serial, in String featureCode);
  oneway void sendDtmf(in int serial, in String s);
  oneway void sendUssd(in int serial, in String ussd);
  oneway void separateConnection(in int serial, in int gsmIndex);
  oneway void setCallForward(in int serial, in android.hardware.radio.voice.CallForwardInfo callInfo);
  oneway void setCallWaiting(in int serial, in boolean enable, in int serviceClass);
  oneway void setClir(in int serial, in int status);
  oneway void setMute(in int serial, in boolean enable);
  oneway void setPreferredVoicePrivacy(in int serial, in boolean enable);
  oneway void setResponseFunctions(in android.hardware.radio.voice.IRadioVoiceResponse radioVoiceResponse, in android.hardware.radio.voice.IRadioVoiceIndication radioVoiceIndication);
  oneway void setTtyMode(in int serial, in android.hardware.radio.voice.TtyMode mode);
  oneway void setVoNrEnabled(in int serial, in boolean enable);
  oneway void startDtmf(in int serial, in String s);
  oneway void stopDtmf(in int serial);
  oneway void switchWaitingOrHoldingAndActive(in int serial);
}
