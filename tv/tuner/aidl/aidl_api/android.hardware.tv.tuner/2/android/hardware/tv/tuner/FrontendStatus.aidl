/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;
/* @hide */
@VintfStability
union FrontendStatus {
  boolean isDemodLocked;
  int snr;
  int ber;
  int per;
  int preBer;
  int signalQuality;
  int signalStrength;
  int symbolRate;
  android.hardware.tv.tuner.FrontendInnerFec innerFec;
  android.hardware.tv.tuner.FrontendModulationStatus modulationStatus;
  android.hardware.tv.tuner.FrontendSpectralInversion inversion;
  android.hardware.tv.tuner.LnbVoltage lnbVoltage;
  int plpId;
  boolean isEWBS;
  int agc;
  boolean isLnaOn;
  boolean[] isLayerError;
  int mer;
  long freqOffset;
  android.hardware.tv.tuner.FrontendDvbtHierarchy hierarchy;
  boolean isRfLocked;
  android.hardware.tv.tuner.FrontendStatusAtsc3PlpInfo[] plpInfo;
  android.hardware.tv.tuner.FrontendModulation[] modulations;
  int[] bers;
  android.hardware.tv.tuner.FrontendInnerFec[] codeRates;
  android.hardware.tv.tuner.FrontendBandwidth bandwidth;
  android.hardware.tv.tuner.FrontendGuardInterval interval;
  android.hardware.tv.tuner.FrontendTransmissionMode transmissionMode;
  int uec;
  int systemId;
  android.hardware.tv.tuner.FrontendInterleaveMode[] interleaving;
  int[] isdbtSegment;
  int[] tsDataRate;
  android.hardware.tv.tuner.FrontendRollOff rollOff;
  boolean isMiso;
  boolean isLinear;
  boolean isShortFrames;
  android.hardware.tv.tuner.FrontendIsdbtMode isdbtMode;
  android.hardware.tv.tuner.FrontendIsdbtPartialReceptionFlag partialReceptionFlag;
  int[] streamIdList;
  int[] dvbtCellIds;
  android.hardware.tv.tuner.FrontendScanAtsc3PlpInfo[] allPlpInfo;
  String iptvContentUrl = "";
  long iptvPacketsReceived;
  long iptvPacketsLost;
  int iptvWorstJitterMs;
  int iptvAverageJitterMs;
}
