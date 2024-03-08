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
import android.hardware.contexthub.HostEndpointInfo;
import android.hardware.contexthub.IContextHubCallback;
import android.hardware.contexthub.MessageDeliveryStatus;
import android.hardware.contexthub.NanSessionStateUpdate;
import android.hardware.contexthub.NanoappBinary;
import android.hardware.contexthub.NanoappInfo;
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
     * Puts the context hub in and out of test mode. Test mode is a clean state
     * where tests can be executed in the same environment. If enable is true,
     * this will enable test mode by unloading all nanoapps. If enable is false,
     * this will disable test mode and reverse the actions of enabling test mode
     * by loading all preloaded nanoapps. This puts CHRE in a normal state.
     *
     * This should only be used for a test environment, either through a
     * @TestApi or development tools. This should not be used in a production
     * environment.
     *
     * @param enable If true, put the context hub in test mode. If false, disable
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
}
