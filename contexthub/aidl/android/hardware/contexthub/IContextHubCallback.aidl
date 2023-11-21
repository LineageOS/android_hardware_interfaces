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

import android.hardware.contexthub.AsyncEventType;
import android.hardware.contexthub.ContextHubMessage;
import android.hardware.contexthub.MessageDeliveryStatus;
import android.hardware.contexthub.NanSessionRequest;
import android.hardware.contexthub.NanoappInfo;

@VintfStability
interface IContextHubCallback {
    /**
     * This callback is passed by the Contexthub service to the HAL
     * implementation to allow the HAL to send information about the
     * currently loaded and active nanoapps on the hub.
     *
     * @param appInfo vector of HubAppinfo structure for each nanoApp
     *                on the hub that can be enabled, disabled and
     *                unloaded by the service. Any nanoApps that cannot
     *                be controlled by the service must not be reported.
     *                All nanoApps that can be controlled by the service
     *                must be reported.
     */
    void handleNanoappInfo(in NanoappInfo[] appInfo);

    /**
     * This callback is passed by the Contexthub service to the HAL
     * implementation to allow the HAL to send asynchronous messages back
     * to the service and registered clients of the ContextHub service.
     *
     * @param msg             message that should be delivered to host app
     *                        clients
     * @param msgContentPerms list of Android permissions that cover the
     *                        contents of the message being sent from the app.
     *                        This is different from the permissions stored
     *                        inside of ContextHubMsg in that these must be a
     *                        subset of those permissions and are meant to
     *                        assist in properly attributing the message
     *                        contents when delivering to a ContextHub service
     *                        client.
     */
    void handleContextHubMessage(in ContextHubMessage msg, in String[] msgContentPerms);

    /**
     * This callback is passed by the Contexthub service to the HAL
     * implementation to allow the HAL to send an asynchronous event
     * to the ContextHub service.
     *
     * @param evt event being sent from the contexthub
     *
     */
    void handleContextHubAsyncEvent(in AsyncEventType evt);

    /**
     * This callback is passed by the Contexthub service to the HAL
     * implementation to allow the HAL to send the response for a
     * transaction.
     *
     * @param transactionId The ID of the transaction associated with this callback
     * @param success true if the transaction succeeded, false otherwise
     *
     */
    void handleTransactionResult(in int transactionId, in boolean success);

    /**
     * This callback is passed by the Contexthub service to the HAL implementation to allow the HAL
     * to request a WiFi NAN session is created to allow the Contexthub to be able to utilize NAN
     * functionality.
     *
     * onNanSessionStateChanged() will be invoked asynchronously after the NAN session request has
     * been completed. This must be done within CONTEXTHUB_NAN_TRANSACTION_TIMEOUT_MS. If the
     * request times out, onNanSessionStateChanged() will be invoked with the state that the session
     * was previously in.
     *
     * @param request Request from the HAL indicating the latest NAN session state it would like.
     */
    void handleNanSessionRequest(in NanSessionRequest request);

    /**
     * This callback is passed by the Contexthub service to the HAL
     * implementation to allow the HAL to send the response for a reliable message.
     * The response is the message delivery status of a recently sent message. See
     * sendMessageDeliveryStatusToHub() for more details.
     *
     * @param hostEndPointId The ID of the host endpoint associated with this message delivery
     *                       status.
     * @param messageDeliveryStatus The status to be sent.
     */
    void handleMessageDeliveryStatus(
            in char hostEndpointId, in MessageDeliveryStatus messageDeliveryStatus);

    /**
     * This callback is passed to the HAL implementation to allow the HAL to request a UUID that
     * uniquely identifies a client.
     *
     * @return a byte array representating the UUID
     */
    byte[16] getUuid();

    /**
     * This callback gets the name of a client implementing this IContextHubCallback interface,
     * which must be a hard-coded string and does not change at runtime.
     *
     * <p>The name provides a human-readable way to identify a client for troubleshooting purpose.
     */
    String getName();

    /**
     * Amount of time, in milliseconds, that a handleNanSessionRequest can be pending before the
     * Contexthub service must respond.
     */
    const int CONTEXTHUB_NAN_TRANSACTION_TIMEOUT_MS = 10000;
}
