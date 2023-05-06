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
interface IRadioVoiceIndication {
  oneway void callRing(in android.hardware.radio.RadioIndicationType type, in boolean isGsm, in android.hardware.radio.voice.CdmaSignalInfoRecord record);
  oneway void callStateChanged(in android.hardware.radio.RadioIndicationType type);
  oneway void cdmaCallWaiting(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.voice.CdmaCallWaiting callWaitingRecord);
  oneway void cdmaInfoRec(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.voice.CdmaInformationRecord[] records);
  oneway void cdmaOtaProvisionStatus(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.voice.CdmaOtaProvisionStatus status);
  oneway void currentEmergencyNumberList(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.voice.EmergencyNumber[] emergencyNumberList);
  oneway void enterEmergencyCallbackMode(in android.hardware.radio.RadioIndicationType type);
  oneway void exitEmergencyCallbackMode(in android.hardware.radio.RadioIndicationType type);
  oneway void indicateRingbackTone(in android.hardware.radio.RadioIndicationType type, in boolean start);
  oneway void onSupplementaryServiceIndication(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.voice.StkCcUnsolSsResult ss);
  oneway void onUssd(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.voice.UssdModeType modeType, in String msg);
  oneway void resendIncallMute(in android.hardware.radio.RadioIndicationType type);
  oneway void srvccStateNotify(in android.hardware.radio.RadioIndicationType type, in android.hardware.radio.voice.SrvccState state);
  oneway void stkCallControlAlphaNotify(in android.hardware.radio.RadioIndicationType type, in String alpha);
  oneway void stkCallSetup(in android.hardware.radio.RadioIndicationType type, in long timeout);
}
