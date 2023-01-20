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

package android.hardware.radio.satellite;
@VintfStability
interface IRadioSatellite {
  oneway void addAllowedSatelliteContacts(in int serial, in String[] contacts);
  oneway void getCapabilities(in int serial);
  oneway void getMaxCharactersPerTextMessage(in int serial);
  oneway void getPendingMessages(in int serial);
  oneway void getPowerState(in int serial);
  oneway void getSatelliteMode(in int serial);
  oneway void getTimeForNextSatelliteVisibility(in int serial);
  oneway void provisionService(in int serial, in String imei, in String msisdn, in String imsi, in android.hardware.radio.satellite.SatelliteFeature[] features);
  oneway void removeAllowedSatelliteContacts(in int serial, in String[] contacts);
  oneway void responseAcknowledgement();
  oneway void sendMessages(in int serial, in String[] messages, in String destination, in double latitude, in double longitude);
  oneway void setIndicationFilter(in int serial, in int filterBitmask);
  oneway void setPower(in int serial, in boolean on);
  oneway void setResponseFunctions(in android.hardware.radio.satellite.IRadioSatelliteResponse satelliteResponse, in android.hardware.radio.satellite.IRadioSatelliteIndication satelliteIndication);
  oneway void startSendingSatellitePointingInfo(in int serial);
  oneway void stopSendingSatellitePointingInfo(in int serial);
}
