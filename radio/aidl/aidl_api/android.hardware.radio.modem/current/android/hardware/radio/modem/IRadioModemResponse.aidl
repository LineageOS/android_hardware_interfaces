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

package android.hardware.radio.modem;
/* @hide */
@VintfStability
interface IRadioModemResponse {
  oneway void acknowledgeRequest(in int serial);
  oneway void enableModemResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void getBasebandVersionResponse(in android.hardware.radio.RadioResponseInfo info, in String version);
  /**
   * @deprecated use getImeiResponse(RadioResponseInfo responseInfo, ImeiInfo imeiInfo)
   */
  oneway void getDeviceIdentityResponse(in android.hardware.radio.RadioResponseInfo info, in String imei, in String imeisv, in String esn, in String meid);
  oneway void getHardwareConfigResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.modem.HardwareConfig[] config);
  oneway void getModemActivityInfoResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.modem.ActivityStatsInfo activityInfo);
  oneway void getModemStackStatusResponse(in android.hardware.radio.RadioResponseInfo info, in boolean isEnabled);
  oneway void getRadioCapabilityResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.modem.RadioCapability rc);
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  oneway void nvReadItemResponse(in android.hardware.radio.RadioResponseInfo info, in String result);
  oneway void nvResetConfigResponse(in android.hardware.radio.RadioResponseInfo info);
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  oneway void nvWriteCdmaPrlResponse(in android.hardware.radio.RadioResponseInfo info);
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  oneway void nvWriteItemResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void requestShutdownResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void sendDeviceStateResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void setRadioCapabilityResponse(in android.hardware.radio.RadioResponseInfo info, in android.hardware.radio.modem.RadioCapability rc);
  oneway void setRadioPowerResponse(in android.hardware.radio.RadioResponseInfo info);
  oneway void getImeiResponse(in android.hardware.radio.RadioResponseInfo responseInfo, in @nullable android.hardware.radio.modem.ImeiInfo imeiInfo);
}
