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

import android.hardware.wifi.IWifiNanIfaceEventCallback;
import android.hardware.wifi.NanBootstrappingRequest;
import android.hardware.wifi.NanBootstrappingResponse;
import android.hardware.wifi.NanConfigRequest;
import android.hardware.wifi.NanConfigRequestSupplemental;
import android.hardware.wifi.NanEnableRequest;
import android.hardware.wifi.NanInitiateDataPathRequest;
import android.hardware.wifi.NanPairingRequest;
import android.hardware.wifi.NanPublishRequest;
import android.hardware.wifi.NanRespondToDataPathIndicationRequest;
import android.hardware.wifi.NanRespondToPairingIndicationRequest;
import android.hardware.wifi.NanSubscribeRequest;
import android.hardware.wifi.NanTransmitFollowupRequest;

/**
 * Interface used to represent a single NAN (Neighbour Aware Network) iface.
 *
 * References to "NAN Spec" are to the Wi-Fi Alliance "Wi-Fi Neighbor Awareness
 * Networking (NAN) Technical Specification".
 */
@VintfStability
interface IWifiNanIface {
    /**
     * Minimum length of Passphrase argument for a data-path configuration.
     */
    const int MIN_DATA_PATH_CONFIG_PASSPHRASE_LENGTH = 8;

    /**
     * Maximum length of Passphrase argument for a data-path configuration.
     */
    const int MAX_DATA_PATH_CONFIG_PASSPHRASE_LENGTH = 63;

    /**
     * Get the name of this iface.
     *
     * @return Name of this iface.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|
     */
    String getName();

    /**
     * Configures an existing NAN functionality (i.e. assumes
     * |IWifiNanIface.enableRequest| already submitted and succeeded).
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyConfigResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg1 Instance of |NanConfigRequest|.
     * @param msg2 Instance of |NanConfigRequestSupplemental|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void configRequest(
            in char cmdId, in NanConfigRequest msg1, in NanConfigRequestSupplemental msg2);

    /**
     * Create a NAN Data Interface.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyCreateDataInterfaceResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param ifaceName The name of the interface, e.g. "aware0".
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void createDataInterfaceRequest(in char cmdId, in String ifaceName);

    /**
     * Delete a NAN Data Interface.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyDeleteDataInterfaceResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param ifaceName The name of the interface, e.g. "aware0".
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void deleteDataInterfaceRequest(in char cmdId, in String ifaceName);

    /**
     * Disable NAN functionality.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyDisableResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void disableRequest(in char cmdId);

    /**
     * Configures and activates NAN clustering (does not start
     * a discovery session or set up data-interfaces or data-paths). Use the
     * |IWifiNanIface.configureRequest| method to change the configuration of an already enabled
     * NAN interface.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyEnableResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg1 Instance of |NanEnableRequest|.
     * @param msg2 Instance of |NanConfigRequestSupplemental|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void enableRequest(
            in char cmdId, in NanEnableRequest msg1, in NanConfigRequestSupplemental msg2);

    /**
     * Get NAN capabilities. Asynchronous response is with
     * |IWifiNanIfaceEventCallback.notifyCapabilitiesResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void getCapabilitiesRequest(in char cmdId);

    /**
     * Initiate a data-path (NDP) setup operation: Initiator.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyInitiateDataPathResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg Instance of |NanInitiateDataPathRequest|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void initiateDataPathRequest(in char cmdId, in NanInitiateDataPathRequest msg);

    /**
     * Requests notifications of significant events on this iface. Multiple calls
     * to this must register multiple callbacks, each of which must receive all
     * events.
     *
     * @param callback An instance of the |IWifiNanIfaceEventCallback| AIDL interface
     *        object.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|
     */
    void registerEventCallback(in IWifiNanIfaceEventCallback callback);

    /**
     * Respond to a received data indication as part of a data-path (NDP) setup operation.
     * An indication is received by the Responder from the Initiator.
     * Asynchronous response is with
     * |IWifiNanIfaceEventCallback.notifyRespondToDataPathIndicationResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg Instance of |NanRespondToDataPathIndicationRequest|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void respondToDataPathIndicationRequest(
            in char cmdId, in NanRespondToDataPathIndicationRequest msg);

    /**
     * Publish request to start advertising a discovery service.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyStartPublishResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg Instance of |NanPublishRequest|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void startPublishRequest(in char cmdId, in NanPublishRequest msg);

    /**
     * Subscribe request to start searching for a discovery service.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyStartSubscribeResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg Instance of |NanSubscribeRequest|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void startSubscribeRequest(in char cmdId, in NanSubscribeRequest msg);

    /**
     * Stop publishing a discovery service.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyStopPublishResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param sessionId ID of the publish discovery session to be stopped.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void stopPublishRequest(in char cmdId, in byte sessionId);

    /**
     * Stop subscribing to a discovery service.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyStopSubscribeResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param sessionId ID of the subscribe discovery session to be stopped.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void stopSubscribeRequest(in char cmdId, in byte sessionId);

    /**
     * Data-path (NDP) termination request. Executed by either Initiator or Responder.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyTerminateDataPathResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param ndpInstanceId Data-path instance ID to be terminated.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void terminateDataPathRequest(in char cmdId, in int ndpInstanceId);

    /**
     * Start the suspension of a discovery service. During the suspend state, the Wi-Fi Aware
     * device must not transmit or receive frames for this session including any active NDPs. If
     * all discovery sessions are suspended then the Wi-Fi Aware device must not transmit or
     * receive any Wi-Fi Aware frames.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifySuspendResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param sessionId ID of the publish/subscribe discovery session to be suspended.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void suspendRequest(in char cmdId, in byte sessionId);

    /**
     * Stop the suspension of a discovery service. Resume cancels an ongoing suspend for this Wi-Fi
     * Aware discovery session and automatically resumes the session and any associated NDPs to the
     * state before they were suspended. The Wi-Fi Aware resume operation should be faster than
     * recreating the corresponding discovery session and NDPs with the same benefit of power.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyResumeResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param sessionId ID of the publish/subscribe discovery session to be resumed.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_NOT_SUPPORTED|,
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void resumeRequest(in char cmdId, in byte sessionId);

    /**
     * NAN transmit follow up message request.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyTransmitFollowupResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg Instance of |NanTransmitFollowupRequest|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void transmitFollowupRequest(in char cmdId, in NanTransmitFollowupRequest msg);

    /**
     * Initiate a NAN pairing operation: Initiator.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyInitiatePairingResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg Instance of |NanPairingRequest|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void initiatePairingRequest(in char cmdId, in NanPairingRequest msg);

    /**
     * Respond to a received request indication of NAN pairing setup operation.
     * An indication is received by the Responder from the Initiator.
     * Asynchronous response is with
     * |IWifiNanIfaceEventCallback.notifyRespondToPairingIndicationResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg Instance of |NanRespondToPairingIndicationRequest|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void respondToPairingIndicationRequest(
            in char cmdId, in NanRespondToPairingIndicationRequest msg);

    /**
     * Initiate a NAN pairing bootstrapping operation: Initiator.
     * Asynchronous response is with
     * |IWifiNanIfaceEventCallback.notifyInitiateBootstrappingResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg Instance of |NanBootstrappingRequest|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void initiateBootstrappingRequest(in char cmdId, in NanBootstrappingRequest msg);

    /**
     * Respond to a received request indication of NAN pairing bootstrapping operation.
     * An indication is received by the Responder from the Initiator.
     * Asynchronous response is with
     * |IWifiNanIfaceEventCallback.notifyRespondToPairingIndicationResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param msg Instance of |notifyRespondToBootstrappingIndicationResponse|.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_INVALID_ARGS|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void respondToBootstrappingIndicationRequest(in char cmdId, in NanBootstrappingResponse msg);

    /**
     * Aware pairing termination request. Executed by either the Initiator or Responder.
     * Asynchronous response is with |IWifiNanIfaceEventCallback.notifyTerminatePairingResponse|.
     *
     * @param cmdId Command Id to use for this invocation.
     * @param pairingInstanceId Pairing instance ID to be terminated.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.ERROR_WIFI_IFACE_INVALID|,
     *         |WifiStatusCode.ERROR_UNKNOWN|
     */
    void terminatePairingRequest(in char cmdId, in int pairingInstanceId);
}
