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
interface IRadioModem {
  oneway void enableModem(in int serial, in boolean on);
  oneway void getBasebandVersion(in int serial);
  /**
   * @deprecated use getImei(int serial)
   */
  oneway void getDeviceIdentity(in int serial);
  oneway void getHardwareConfig(in int serial);
  oneway void getModemActivityInfo(in int serial);
  oneway void getModemStackStatus(in int serial);
  oneway void getRadioCapability(in int serial);
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  oneway void nvReadItem(in int serial, in android.hardware.radio.modem.NvItem itemId);
  oneway void nvResetConfig(in int serial, in android.hardware.radio.modem.ResetNvType resetType);
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  oneway void nvWriteCdmaPrl(in int serial, in byte[] prl);
  /**
   * @deprecated NV APIs are deprecated starting from Android U.
   */
  oneway void nvWriteItem(in int serial, in android.hardware.radio.modem.NvWriteItem item);
  oneway void requestShutdown(in int serial);
  oneway void responseAcknowledgement();
  oneway void sendDeviceState(in int serial, in android.hardware.radio.modem.DeviceStateType deviceStateType, in boolean state);
  oneway void setRadioCapability(in int serial, in android.hardware.radio.modem.RadioCapability rc);
  oneway void setRadioPower(in int serial, in boolean powerOn, in boolean forEmergencyCall, in boolean preferredForEmergencyCall);
  oneway void setResponseFunctions(in android.hardware.radio.modem.IRadioModemResponse radioModemResponse, in android.hardware.radio.modem.IRadioModemIndication radioModemIndication);
  oneway void getImei(in int serial);
}
