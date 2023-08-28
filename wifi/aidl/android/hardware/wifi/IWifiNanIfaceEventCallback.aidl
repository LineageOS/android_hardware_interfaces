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

package android.hardware.wifi;

import android.hardware.wifi.NanBootstrappingConfirmInd;
import android.hardware.wifi.NanBootstrappingRequestInd;
import android.hardware.wifi.NanCapabilities;
import android.hardware.wifi.NanClusterEventInd;
import android.hardware.wifi.NanDataPathConfirmInd;
import android.hardware.wifi.NanDataPathRequestInd;
import android.hardware.wifi.NanDataPathScheduleUpdateInd;
import android.hardware.wifi.NanFollowupReceivedInd;
import android.hardware.wifi.NanMatchInd;
import android.hardware.wifi.NanPairingConfirmInd;
import android.hardware.wifi.NanPairingRequestInd;
import android.hardware.wifi.NanStatus;
import android.hardware.wifi.NanSuspensionModeChangeInd;
/**
 * NAN Response and Asynchronous Event Callbacks.
 *
 * References to "NAN Spec" are to the Wi-Fi Alliance "Wi-Fi Neighbor Awareness
 * Networking (NAN) Technical Specification".
 */
@VintfStability
oneway interface IWifiNanIfaceEventCallback {
    /**
     * Callback indicating that a cluster event has been received.
     *
     * @param event NanClusterEventInd containing event details.
     */
    void eventClusterEvent(in NanClusterEventInd event);

    /**
     * Callback indicating that a data-path (NDP) setup has been completed.
     * Received by both Initiator and Responder.
     *
     * @param event NanDataPathConfirmInd containing event details.
     */
    void eventDataPathConfirm(in NanDataPathConfirmInd event);

    /**
     * Callback indicating that a data-path (NDP) setup has been requested by
     * an Initiator peer (received by the intended Responder).
     *
     * @param event NanDataPathRequestInd containing event details.
     */
    void eventDataPathRequest(in NanDataPathRequestInd event);

    /**
     * Callback indicating that a data-path (NDP) schedule has been updated
     * (e.g. channels have been changed).
     *
     * @param event NanDataPathScheduleUpdateInd containing event details.
     */
    void eventDataPathScheduleUpdate(in NanDataPathScheduleUpdateInd event);

    /**
     * Callback indicating that a list of data-paths (NDP) have been terminated.
     * Received by both Initiator and Responder.
     *
     * @param ndpInstanceId Data-path ID of the terminated data-path.
     */
    void eventDataPathTerminated(in int ndpInstanceId);

    /**
     * Callback indicating that a NAN has been disabled.
     *
     * @param status NanStatus describing the reason for the disable event.
     *               Possible status codes are:
     *               |NanStatusCode.SUCCESS|
     *               |NanStatusCode.UNSUPPORTED_CONCURRENCY_NAN_DISABLED|
     */
    void eventDisabled(in NanStatus status);

    /**
     * Callback indicating that a followup message has been received from a peer.
     *
     * @param event NanFollowupReceivedInd containing event details.
     */
    void eventFollowupReceived(in NanFollowupReceivedInd event);

    /**
     * Callback indicating that a match has occurred: i.e. a service has been
     * discovered.
     *
     * @param event NanMatchInd containing event details.
     */
    void eventMatch(in NanMatchInd event);

    /**
     * Callback indicating that a previously discovered match (service) has expired.
     *
     * @param discoverySessionId Discovery session ID of the expired match.
     * @param peerId Peer ID of the expired match.
     */
    void eventMatchExpired(in byte discoverySessionId, in int peerId);

    /**
     * Callback indicating that an active publish session has terminated.
     *
     * @param sessionId Discovery session ID of the terminated session.
     * @param status NanStatus describing the reason for the session termination.
     *               Possible status codes are:
     *               |NanStatusCode.SUCCESS|
     */
    void eventPublishTerminated(in byte sessionId, in NanStatus status);

    /**
     * Callback indicating that an active subscribe session has terminated.
     *
     * @param sessionId Discovery session ID of the terminated session.
     * @param status NanStatus describing the reason for the session termination.
     *               Possible status codes are:
     *               |NanStatusCode.SUCCESS|
     */
    void eventSubscribeTerminated(in byte sessionId, in NanStatus status);

    /**
     * Callback providing status on a completed followup message transmit operation.
     *
     * @param id Command ID corresponding to the original |transmitFollowupRequest| request.
     * @param status NanStatus of the operation. Possible status codes are:
     *               |NanStatusCode.SUCCESS|
     *               |NanStatusCode.NO_OTA_ACK|
     *               |NanStatusCode.PROTOCOL_FAILURE|
     */
    void eventTransmitFollowup(in char id, in NanStatus status);

    /**
     * Callback indicating that device suspension mode status change
     *
     * @param event NanSuspensionModeChangeInd containing event details.
     */
    void eventSuspensionModeChanged(in NanSuspensionModeChangeInd event);

    /**
     * Callback invoked in response to a capability request
     * |IWifiNanIface.getCapabilitiesRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     * @param capabilities Capability data.
     */
    void notifyCapabilitiesResponse(
            in char id, in NanStatus status, in NanCapabilities capabilities);

    /**
     * Callback invoked in response to a config request |IWifiNanIface.configRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     */
    void notifyConfigResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to a create data interface request
     * |IWifiNanIface.createDataInterfaceRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     */
    void notifyCreateDataInterfaceResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to a delete data interface request
     * |IWifiNanIface.deleteDataInterfaceRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     */
    void notifyDeleteDataInterfaceResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to a disable request |IWifiNanIface.disableRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     */
    void notifyDisableResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to an enable request |IWifiNanIface.enableRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.ALREADY_ENABLED|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.NAN_NOT_ALLOWED|
     */
    void notifyEnableResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to an initiate data path request
     * |IWifiNanIface.initiateDataPathRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.INVALID_PEER_ID|
     * @param ndpInstanceId ID of the new data path being negotiated (on successful status).
     */
    void notifyInitiateDataPathResponse(in char id, in NanStatus status, in int ndpInstanceId);

    /**
     * Callback invoked in response to a respond to data path indication request
     * |IWifiNanIface.respondToDataPathIndicationRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.INVALID_NDP_ID|
     */
    void notifyRespondToDataPathIndicationResponse(in char id, in NanStatus status);

    /**
     * Callback invoked to notify the status of the start publish request
     * |IWifiNanIface.startPublishRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.NO_RESOURCES_AVAILABLE|
     *        |NanStatusCode.INVALID_SESSION_ID|
     * @param sessionId ID of the new publish session (if successfully created).
     */
    void notifyStartPublishResponse(in char id, in NanStatus status, in byte sessionId);

    /**
     * Callback invoked to notify the status of the start subscribe request
     * |IWifiNanIface.startSubscribeRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.NO_RESOURCES_AVAILABLE|
     *        |NanStatusCode.INVALID_SESSION_ID|
     * @param sessionId ID of the new subscribe session (if successfully created).
     */
    void notifyStartSubscribeResponse(in char id, in NanStatus status, in byte sessionId);

    /**
     * Callback invoked in response to a stop publish request
     * |IWifiNanIface.stopPublishRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_SESSION_ID|
     *        |NanStatusCode.INTERNAL_FAILURE|
     */
    void notifyStopPublishResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to a stop subscribe request
     * |IWifiNanIface.stopSubscribeRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_SESSION_ID|
     *        |NanStatusCode.INTERNAL_FAILURE|
     */
    void notifyStopSubscribeResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to a terminate data path request
     * |IWifiNanIface.terminateDataPathRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.INVALID_NDP_ID|
     */
    void notifyTerminateDataPathResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to a suspension request
     * |IWifiNanIface.suspendRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_SESSION_ID|
     *        |NanStatusCode.INVALID_STATE|
     *        |NanStatusCode.INTERNAL_FAILURE|
     */
    void notifySuspendResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to a resume request
     * |IWifiNanIface.resumeRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_SESSION_ID|
     *        |NanStatusCode.INVALID_STATE|
     *        |NanStatusCode.INTERNAL_FAILURE|
     */
    void notifyResumeResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to a transmit followup request
     * |IWifiNanIface.transmitFollowupRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.INVALID_SESSION_ID|
     *        |NanStatusCode.INVALID_PEER_ID|
     *        |NanStatusCode.FOLLOWUP_TX_QUEUE_FULL|
     */
    void notifyTransmitFollowupResponse(in char id, in NanStatus status);

    /**
     * Callback indicating that a NAN pairing setup/verification has been requested by
     * an Initiator peer (received by the intended Responder).
     *
     * @param event NanPairingRequestInd containing event details.
     */
    void eventPairingRequest(in NanPairingRequestInd event);

    /**
     * Callback indicating that a NAN pairing setup/verification has been completed.
     * Received by both Initiator and Responder.
     *
     * @param event NanPairingConfirmInd containing event details.
     */
    void eventPairingConfirm(in NanPairingConfirmInd event);

    /**
     * Callback invoked in response to an initiate NAN pairing request
     * |IWifiNanIface.initiatePairingRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.INVALID_PEER_ID|
     * @param pairingInstanceId ID of the new pairing being negotiated (on successful status).
     */
    void notifyInitiatePairingResponse(in char id, in NanStatus status, in int pairingInstanceId);

    /**
     * Callback invoked in response to a respond to NAN pairing indication request
     * |IWifiNanIface.respondToPairingIndicationRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.INVALID_NDP_ID|
     */
    void notifyRespondToPairingIndicationResponse(in char id, in NanStatus status);

    /**
     * Callback indicating that a NAN bootstrapping setup has been requested by
     * an Initiator peer (received by the intended Responder).
     *
     * @param event NanBootstrappingRequestInd containing event details.
     */
    void eventBootstrappingRequest(in NanBootstrappingRequestInd event);

    /**
     * Callback indicating that a NAN bootstrapping setuphas been completed.
     * Received by Initiator.
     *
     * @param event NanBootstrappingConfirmInd containing event details.
     */
    void eventBootstrappingConfirm(in NanBootstrappingConfirmInd event);

    /**
     * Callback invoked in response to an initiate NAN pairing bootstrapping request
     * |IWifiNanIface.initiateBootstrappingRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.INVALID_PEER_ID|
     * @param bootstrappingInstanceId ID of the new pairing being negotiated (on successful status).
     */
    void notifyInitiateBootstrappingResponse(
            in char id, in NanStatus status, in int bootstrappingInstanceId);

    /**
     * Callback invoked in response to a respond to pairing bootstrapping indication request
     * |IWifiNanIface.respondToBootstrappingIndicationRequest|.
     *
     * @param id Command ID corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.INVALID_NDP_ID|
     */
    void notifyRespondToBootstrappingIndicationResponse(in char id, in NanStatus status);

    /**
     * Callback invoked in response to a terminate pairing request
     * |IWifiNanIface.terminatePairingRequest|.
     *
     * @param id Command Id corresponding to the original request.
     * @param status NanStatus of the operation. Possible status codes are:
     *        |NanStatusCode.SUCCESS|
     *        |NanStatusCode.INVALID_ARGS|
     *        |NanStatusCode.INTERNAL_FAILURE|
     *        |NanStatusCode.PROTOCOL_FAILURE|
     *        |NanStatusCode.INVALID_PAIRING_ID|
     */
    void notifyTerminatePairingResponse(in char id, in NanStatus status);
}
