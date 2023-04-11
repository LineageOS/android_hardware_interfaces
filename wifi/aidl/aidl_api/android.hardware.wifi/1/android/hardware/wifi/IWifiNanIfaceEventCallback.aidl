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
interface IWifiNanIfaceEventCallback {
  oneway void eventClusterEvent(in android.hardware.wifi.NanClusterEventInd event);
  oneway void eventDataPathConfirm(in android.hardware.wifi.NanDataPathConfirmInd event);
  oneway void eventDataPathRequest(in android.hardware.wifi.NanDataPathRequestInd event);
  oneway void eventDataPathScheduleUpdate(in android.hardware.wifi.NanDataPathScheduleUpdateInd event);
  oneway void eventDataPathTerminated(in int ndpInstanceId);
  oneway void eventDisabled(in android.hardware.wifi.NanStatus status);
  oneway void eventFollowupReceived(in android.hardware.wifi.NanFollowupReceivedInd event);
  oneway void eventMatch(in android.hardware.wifi.NanMatchInd event);
  oneway void eventMatchExpired(in byte discoverySessionId, in int peerId);
  oneway void eventPublishTerminated(in byte sessionId, in android.hardware.wifi.NanStatus status);
  oneway void eventSubscribeTerminated(in byte sessionId, in android.hardware.wifi.NanStatus status);
  oneway void eventTransmitFollowup(in char id, in android.hardware.wifi.NanStatus status);
  oneway void eventSuspensionModeChanged(in android.hardware.wifi.NanSuspensionModeChangeInd event);
  oneway void notifyCapabilitiesResponse(in char id, in android.hardware.wifi.NanStatus status, in android.hardware.wifi.NanCapabilities capabilities);
  oneway void notifyConfigResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyCreateDataInterfaceResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyDeleteDataInterfaceResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyDisableResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyEnableResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyInitiateDataPathResponse(in char id, in android.hardware.wifi.NanStatus status, in int ndpInstanceId);
  oneway void notifyRespondToDataPathIndicationResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyStartPublishResponse(in char id, in android.hardware.wifi.NanStatus status, in byte sessionId);
  oneway void notifyStartSubscribeResponse(in char id, in android.hardware.wifi.NanStatus status, in byte sessionId);
  oneway void notifyStopPublishResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyStopSubscribeResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyTerminateDataPathResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifySuspendResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyResumeResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyTransmitFollowupResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void eventPairingRequest(in android.hardware.wifi.NanPairingRequestInd event);
  oneway void eventPairingConfirm(in android.hardware.wifi.NanPairingConfirmInd event);
  oneway void notifyInitiatePairingResponse(in char id, in android.hardware.wifi.NanStatus status, in int pairingInstanceId);
  oneway void notifyRespondToPairingIndicationResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void eventBootstrappingRequest(in android.hardware.wifi.NanBootstrappingRequestInd event);
  oneway void eventBootstrappingConfirm(in android.hardware.wifi.NanBootstrappingConfirmInd event);
  oneway void notifyInitiateBootstrappingResponse(in char id, in android.hardware.wifi.NanStatus status, in int bootstrappingInstanceId);
  oneway void notifyRespondToBootstrappingIndicationResponse(in char id, in android.hardware.wifi.NanStatus status);
  oneway void notifyTerminatePairingResponse(in char id, in android.hardware.wifi.NanStatus status);
}
