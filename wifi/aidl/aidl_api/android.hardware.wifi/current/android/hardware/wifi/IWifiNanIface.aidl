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
interface IWifiNanIface {
  String getName();
  void configRequest(in char cmdId, in android.hardware.wifi.NanConfigRequest msg1, in android.hardware.wifi.NanConfigRequestSupplemental msg2);
  void createDataInterfaceRequest(in char cmdId, in String ifaceName);
  void deleteDataInterfaceRequest(in char cmdId, in String ifaceName);
  void disableRequest(in char cmdId);
  void enableRequest(in char cmdId, in android.hardware.wifi.NanEnableRequest msg1, in android.hardware.wifi.NanConfigRequestSupplemental msg2);
  void getCapabilitiesRequest(in char cmdId);
  void initiateDataPathRequest(in char cmdId, in android.hardware.wifi.NanInitiateDataPathRequest msg);
  void registerEventCallback(in android.hardware.wifi.IWifiNanIfaceEventCallback callback);
  void respondToDataPathIndicationRequest(in char cmdId, in android.hardware.wifi.NanRespondToDataPathIndicationRequest msg);
  void startPublishRequest(in char cmdId, in android.hardware.wifi.NanPublishRequest msg);
  void startSubscribeRequest(in char cmdId, in android.hardware.wifi.NanSubscribeRequest msg);
  void stopPublishRequest(in char cmdId, in byte sessionId);
  void stopSubscribeRequest(in char cmdId, in byte sessionId);
  void terminateDataPathRequest(in char cmdId, in int ndpInstanceId);
  void suspendRequest(in char cmdId, in byte sessionId);
  void resumeRequest(in char cmdId, in byte sessionId);
  void transmitFollowupRequest(in char cmdId, in android.hardware.wifi.NanTransmitFollowupRequest msg);
  void initiatePairingRequest(in char cmdId, in android.hardware.wifi.NanPairingRequest msg);
  void respondToPairingIndicationRequest(in char cmdId, in android.hardware.wifi.NanRespondToPairingIndicationRequest msg);
  void initiateBootstrappingRequest(in char cmdId, in android.hardware.wifi.NanBootstrappingRequest msg);
  void respondToBootstrappingIndicationRequest(in char cmdId, in android.hardware.wifi.NanBootstrappingResponse msg);
  void terminatePairingRequest(in char cmdId, in int pairingInstanceId);
  const int MIN_DATA_PATH_CONFIG_PASSPHRASE_LENGTH = 8;
  const int MAX_DATA_PATH_CONFIG_PASSPHRASE_LENGTH = 63;
}
