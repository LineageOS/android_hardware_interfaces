/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.wifi;
@VintfStability
interface IWifiStaIfaceEventCallback {
  oneway void onBackgroundFullScanResult(in int cmdId, in int bucketsScanned, in android.hardware.wifi.StaScanResult result);
  oneway void onBackgroundScanFailure(in int cmdId);
  oneway void onBackgroundScanResults(in int cmdId, in android.hardware.wifi.StaScanData[] scanDatas);
  oneway void onRssiThresholdBreached(in int cmdId, in byte[6] currBssid, in int currRssi);
  oneway void onTwtFailure(in int cmdId, in android.hardware.wifi.IWifiStaIfaceEventCallback.TwtErrorCode error);
  oneway void onTwtSessionCreate(in int cmdId, in android.hardware.wifi.TwtSession twtSession);
  oneway void onTwtSessionUpdate(in int cmdId, in android.hardware.wifi.TwtSession twtSession);
  oneway void onTwtSessionTeardown(in int cmdId, in int twtSessionId, in android.hardware.wifi.IWifiStaIfaceEventCallback.TwtTeardownReasonCode reasonCode);
  oneway void onTwtSessionStats(in int cmdId, in int twtSessionId, in android.hardware.wifi.TwtSessionStats twtSessionStats);
  oneway void onTwtSessionSuspend(in int cmdId, in int twtSessionId);
  oneway void onTwtSessionResume(in int cmdId, in int twtSessionId);
  @Backing(type="byte") @VintfStability
  enum TwtErrorCode {
    FAILURE_UNKNOWN,
    ALREADY_RESUMED,
    ALREADY_SUSPENDED,
    INVALID_PARAMS,
    MAX_SESSION_REACHED,
    NOT_AVAILABLE,
    NOT_SUPPORTED,
    PEER_NOT_SUPPORTED,
    PEER_REJECTED,
    TIMEOUT,
  }
  @Backing(type="byte") @VintfStability
  enum TwtTeardownReasonCode {
    UNKNOWN,
    LOCALLY_REQUESTED,
    INTERNALLY_INITIATED,
    PEER_INITIATED,
  }
}
