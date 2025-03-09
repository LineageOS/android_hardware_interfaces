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

package android.hardware.contexthub;

import android.hardware.contexthub.ContextHubInfo;
import android.hardware.contexthub.ContextHubMessage;
import android.hardware.contexthub.EndpointId;
import android.hardware.contexthub.EndpointInfo;
import android.hardware.contexthub.HostEndpointInfo;
import android.hardware.contexthub.HubInfo;
import android.hardware.contexthub.IContextHubCallback;
import android.hardware.contexthub.IEndpointCallback;
import android.hardware.contexthub.Message;
import android.hardware.contexthub.MessageDeliveryStatus;
import android.hardware.contexthub.NanSessionStateUpdate;
import android.hardware.contexthub.NanoappBinary;
import android.hardware.contexthub.NanoappInfo;
import android.hardware.contexthub.Reason;
import android.hardware.contexthub.Service;
import android.hardware.contexthub.Setting;

@VintfStability
interface IContextHub {
    /**
     * Enumerates all available Context Hubs.
     *
     * @return A list of ContextHubInfo describing all Context Hubs.
     */
    List<ContextHubInfo> getContextHubs();

    /**
     * Loads a nanoapp, and invokes the nanoapp's initialization "start()" entrypoint.
     *
     * The return value of this method only indicates that the request has been accepted.
     * If true is returned, the Context Hub must handle an asynchronous result using the
     * the handleTransactionResult() callback.
     *
     * Depending on the implementation, nanoapp loaded via this API may or may
     * not persist across reboots of the hub. If they do persist, the
     * implementation must initially place nanoapp in the disabled state upon a
     * reboot, and not start them until a call is made to enableNanoapp(). In
     * this case, the app must also be unloaded upon a factory reset of the
     * device.
     *
     * Loading a nanoapp must not take more than 30 seconds.
     *
     * @param contextHubId The identifier of the Context Hub
     * @param appBinary The nanoapp binary with header
     * @param transactionId The transaction ID associated with this request
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid.
     *         EX_UNSUPPORTED_OPERATION if this functionality is unsupported.
     *         EX_SERVICE_SPECIFIC on error
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void loadNanoapp(in int contextHubId, in NanoappBinary appBinary, in int transactionId);

    /**
     * Invokes the nanoapp's deinitialization "end()" entrypoint, and unloads the nanoapp.
     *
     * The return value of this method only indicates that the request has been accepted.
     * If true is returned, the Context Hub must handle an asynchronous result using the
     * the handleTransactionResult() callback.
     *
     * Unloading a nanoapp must not take more than 5 seconds.
     *
     * @param contextHubId The identifier of the Context Hub
     * @param appId The unique ID of the nanoapp
     * @param transactionId The transaction ID associated with this request
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid.
     *         EX_UNSUPPORTED_OPERATION if this functionality is unsupported.
     *         EX_SERVICE_SPECIFIC on error
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void unloadNanoapp(in int contextHubId, in long appId, in int transactionId);

    /**
     * Disables a nanoapp by invoking the nanoapp's "end()" entrypoint, but does not unload the
     * nanoapp.
     *
     * The return value of this method only indicates that the request has been accepted.
     * If true is returned, the Context Hub must handle an asynchronous result using the
     * the handleTransactionResult() callback.
     *
     * Disabling a nanoapp must not take more than 5 seconds.
     *
     * @param contextHubId The identifier of the Context Hub
     * @param appId The unique ID of the nanoapp
     * @param transactionId The transaction ID associated with this request
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid.
     *         EX_UNSUPPORTED_OPERATION if this functionality is unsupported.
     *         EX_SERVICE_SPECIFIC on error
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void disableNanoapp(in int contextHubId, in long appId, in int transactionId);

    /**
     * Enables a nanoapp by invoking the nanoapp's initialization "start()" entrypoint.
     *
     * The return value of this method only indicates that the request has been accepted.
     * If true is returned, the Context Hub must handle an asynchronous result using the
     * the handleTransactionResult() callback.
     *
     * Enabling a nanoapp must not take more than 5 seconds.
     *
     * @param contextHubId The identifier of the Context Hub
     * @param appId appIdentifier returned by the HAL
     * @param message   message to be sent
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid.
     *         EX_UNSUPPORTED_OPERATION if this functionality is unsupported.
     *         EX_SERVICE_SPECIFIC on error
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void enableNanoapp(in int contextHubId, in long appId, in int transactionId);

    /**
     * Notification sent by the framework to indicate that the user has changed a setting.
     *
     * @param setting User setting that has been modified
     * @param enabled true if the setting has been enabled, false otherwise
     */
    void onSettingChanged(in Setting setting, in boolean enabled);

    /**
     * Queries for a list of loaded nanoapps on a Context Hub.
     *
     * If this method succeeds, the result of the query must be delivered through the
     * handleNanoappInfo() callback.
     *
     * @param contextHubId The identifier of the Context Hub
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid.
     *         EX_UNSUPPORTED_OPERATION if this functionality is unsupported.
     *         EX_SERVICE_SPECIFIC on error
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void queryNanoapps(in int contextHubId);

    /**
     * Register a callback for the HAL implementation to send asynchronous messages to the service
     * from a Context hub. Each HAL client can only have one callback for each Context Hub ID.
     *
     * A call to this function when a callback has already been registered must override the
     * previous registration.
     *
     * @param contextHubId The identifier of the Context Hub
     * @param callback an implementation of the IContextHubCallbacks
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid.
     *         EX_SERVICE_SPECIFIC on error
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void registerCallback(in int contextHubId, in IContextHubCallback cb);

    /**
     * Sends a message targeted to a nanoapp to the Context Hub.
     *
     * @param contextHubId The identifier of the Context Hub
     * @param message The message to be sent
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid.
     *         EX_SERVICE_SPECIFIC on error
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void sendMessageToHub(in int contextHubId, in ContextHubMessage message);

    /**
     * Invoked when a host endpoint has connected with the ContextHubService.
     *
     * The host associated with this invocation may initiate a communication channel with
     * the Context Hub using sendMessageToHub.
     *
     * @param hostEndpointInfo Metadata associated with this host endpoint.
     */
    void onHostEndpointConnected(in HostEndpointInfo hostEndpointInfo);

    /**
     * Invoked when a host endpoint has disconnected from the framework. This could be as a result
     * of an explicit connection closure, or unexpected restarts.
     *
     * Note that hostEndpointId is the same as the value in HostEndpointInfo. When this function is
     * called, the HAL is expected to clean up any resources attached to the messaging channel
     * associated with this host endpoint ID.
     *
     * @param hostEndPointId The ID of the host that has disconnected. Any invalid values for this
     *                       parameter should be ignored (no-op).
     */
    void onHostEndpointDisconnected(char hostEndpointId);

    /**
     * Provides the list of preloaded nanoapp IDs on the system. The output of this API must
     * not change.
     *
     * @param contextHubId The identifier of the Context Hub.
     *
     * @return The list of preloaded nanoapp IDs.
     */
    long[] getPreloadedNanoappIds(in int contextHubId);

    /**
     * Invoked when the state of the NAN session requested through handleNanSessionRequest()
     * changes. This function may be invoked without a corresponding handleNanSessionRequest to
     * indicate if a NAN session was terminated without a request due to resource limitations.
     *
     * If the state becomes disabled without an explicit request from the HAL, the HAL MUST
     * explicitly invoke handleNanSessionRequest() at a later point in time to attempt to
     * re-enable NAN.
     *
     * @param update Information about the latest NAN session state.
     */
    void onNanSessionStateChanged(in NanSessionStateUpdate update);

    /**
     * Puts the Context Hub in and out of test mode. Test mode is a clean state
     * where tests can be executed in the same environment. If enable is true,
     * this will enable test mode by unloading all nanoapps. If enable is false,
     * this will disable test mode and reverse the actions of enabling test mode
     * by loading all preloaded nanoapps. This puts CHRE in a normal state.
     *
     * This should only be used for a test environment, either through a
     * @TestApi or development tools. This should not be used in a production
     * environment.
     *
     * @param enable If true, put the Context Hub in test mode. If false, disable
     *               test mode.
     */
    void setTestMode(in boolean enable);

    /**
     * Sends a message delivery status to the Context Hub in response to receiving a
     * ContextHubMessage with isReliable=true. Each reliable message should have a
     * messageDeliveryStatus response. This method sends the message delivery status
     * back to the Context Hub.
     *
     * @param contextHubId The identifier of the Context Hub.
     * @param messageDeliveryStatus The status to be sent.
     *
     * @throws EX_UNSUPPORTED_OPERATION if ContextHubInfo.supportsReliableMessages is false for
     * this hub.
     */
    void sendMessageDeliveryStatusToHub(
            in int contextHubId, in MessageDeliveryStatus messageDeliveryStatus);

    /**
     * Error codes that are used as service specific errors with the AIDL return
     * value EX_SERVICE_SPECIFIC.
     */
    const int EX_CONTEXT_HUB_UNSPECIFIED = -1;

    /** Lists all the hubs, including the Context Hub and generic hubs. */
    List<HubInfo> getHubs();

    /** Lists all the endpoints, including the Context Hub nanoapps and generic endpoints. */
    List<EndpointInfo> getEndpoints();

    /**
     * Publishes an endpoint from the calling side (e.g. Android). Endpoints must be registered
     * prior to starting a session.
     */
    void registerEndpoint(in EndpointInfo endpoint);

    /**
     * Teardown an endpoint from the calling side (e.g. Android). This endpoint must have already
     * been published via registerEndpoint().
     */
    void unregisterEndpoint(in EndpointInfo endpoint);

    /**
     * Attaches a callback interface to receive events targeted at endpoints registered by the
     * caller.
     */
    void registerEndpointCallback(in IEndpointCallback callback);

    /**
     * Request a range of session IDs for the caller to use when initiating sessions. This may be
     * called more than once, but typical usage is to request a large enough range to accommodate
     * the maximum expected number of concurrent sessions, but not overly large as to limit other
     * clients.
     *
     * @param size The number of sessionId reserved for host-initiated sessions. This number should
     *         be less than or equal to 1024.
     *
     * @return An array with two elements representing the smallest and largest possible session id
     *         available for host.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the size is invalid.
     * @throws EX_SERVICE_SPECIFIC if the id range requested cannot be allocated.
     */
    int[] requestSessionIdRange(int size);

    /**
     * Request to open a session for communication between an endpoint previously registered by the
     * caller and a target endpoint found in getEndpoints(), optionally scoped to a service
     * published by the target endpoint.
     *
     * Upon returning from this function, the session is in pending state, and the final result will
     * be given by an asynchronous call to onEndpointSessionOpenComplete() on success, or
     * onCloseEndpointSession() on failure.
     *
     * @param sessionId Caller-allocated session identifier, which must be unique across all active
     *         sessions, and must fall in a range allocated via requestSessionIdRange().
     * @param destination The EndpointId representing the destination side of the session.
     * @param initiator The EndpointId representing the initiating side of the session, which
     *         must've already been published through registerEndpoint().
     * @param serviceDescriptor Descriptor for the service specification for scoping this session
     *         (nullable). Null indicates a fully custom marshalling scheme. The value should match
     *         a published descriptor for both destination and initiator.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid, or the combination of the
     *         arguments is invalid.
     * @throws EX_SERVICE_SPECIFIC on other errors
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void openEndpointSession(int sessionId, in EndpointId destination, in EndpointId initiator,
            in @nullable String serviceDescriptor);

    /**
     * Send a message from one endpoint to another on the (currently open) session.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         openEndpointSession() or onEndpointSessionOpenRequest().
     * @param msg The Message object representing a message to endpoint from the endpoint on host.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid, or the combination of the
     *         arguments is invalid.
     * @throws EX_SERVICE_SPECIFIC on other errors
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void sendMessageToEndpoint(int sessionId, in Message msg);

    /**
     * Sends a message delivery status to the endpoint in response to receiving a Message with flag
     * FLAG_REQUIRES_DELIVERY_STATUS. Each message with the flag should have a MessageDeliveryStatus
     * response. This method sends the message delivery status back to the remote endpoint for a
     * session.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         openEndpointSession() or onEndpointSessionOpenRequest().
     * @param msgStatus The MessageDeliveryStatus object representing the delivery status for a
     *         specific message (identified by the sequenceNumber) within the session.
     *
     * @throws EX_UNSUPPORTED_OPERATION if ContextHubInfo.supportsReliableMessages is false for
     *          the hub involved in this session.
     */
    void sendMessageDeliveryStatusToEndpoint(int sessionId, in MessageDeliveryStatus msgStatus);

    /**
     * Closes a session previously opened by openEndpointSession() or requested via
     * onEndpointSessionOpenRequest(). Processing of session closure must be ordered/synchronized
     * with message delivery, such that if this session was open, any messages previously passed to
     * sendMessageToEndpoint() that are still in-flight must still be delivered before the session
     * is closed. Any in-flight messages to the endpoint that requested to close the session will
     * not be delivered.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         openEndpointSession() or onEndpointSessionOpenRequest().
     * @param reason The reason for this close endpoint session request.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid, or the combination of the
     *         arguments is invalid.
     * @throws EX_SERVICE_SPECIFIC on other errors
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void closeEndpointSession(int sessionId, in Reason reason);

    /**
     * Notifies the HAL that the session requested by onEndpointSessionOpenRequest is ready to use.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         onEndpointSessionOpenRequest(). This id is assigned by the HAL.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid, or the combination of the
     *         arguments is invalid.
     * @throws EX_SERVICE_SPECIFIC on other errors
     *         - EX_CONTEXT_HUB_UNSPECIFIED if the request failed for other reasons.
     */
    void endpointSessionOpenComplete(int sessionId);
}
