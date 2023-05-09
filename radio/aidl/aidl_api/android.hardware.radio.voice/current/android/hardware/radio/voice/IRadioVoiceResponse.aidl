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
interface IRadioVoiceResponse {
  oneway void acceptCallResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void acknowledgeRequest(in int serial);
  oneway void cancelPendingUssdResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void conferenceResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void dialResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void emergencyDialResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void exitEmergencyCallbackModeResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void explicitCallTransferResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void getCallForwardStatusResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.voice.CallForwardInfo[] callForwardInfos);
  oneway void getCallWaitingResponse(in android.hardware.radio.RadioResponseInfo info, in boolean enable, in int serviceClass);
  oneway void getClipResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.voice.ClipStatus status);
  oneway void getClirResponse(in android.hardware.radio.RadioResponseInfo info, in int n, in int m);
  oneway void getCurrentCallsResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.voice.Call[] calls);
  oneway void getLastCallFailCauseResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.voice.LastCallFailCauseInfo failCauseinfo);
  oneway void getMuteResponse(in android.hardware.radio.RadioResponseInfo info, in boolean enable);
  oneway void getPreferredVoicePrivacyResponse(in android.hardware.radio.RadioResponseInfo info, in boolean enable);
  oneway void getTtyModeResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.voice.TtyMode mode);
  oneway void handleStkCallSetupRequestFromSimResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void hangupConnectionResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void hangupForegroundResumeBackgroundResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void hangupWaitingOrBackgroundResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void isVoNrEnabledResponse(in android.hardware.radio.RadioResponseInfo info, in boolean enable);
  oneway void rejectCallResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendBurstDtmfResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendCdmaFeatureCodeResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendDtmfResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendUssdResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void separateConnectionResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCallForwardResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setCallWaitingResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setClirResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setMuteResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setPreferredVoicePrivacyResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setTtyModeResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setVoNrEnabledResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void startDtmfResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void stopDtmfResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void switchWaitingOrHoldingAndActiveResponse(in android.hardware.radio.RadioResponseInfo info);
}
