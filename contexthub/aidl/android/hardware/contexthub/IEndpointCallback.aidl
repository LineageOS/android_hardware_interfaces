/*
 * Copyright (C) 2024 The Android Open Source Project
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

import android.hardware.contexthub.EndpointId;
import android.hardware.contexthub.EndpointInfo;
import android.hardware.contexthub.Message;
import android.hardware.contexthub.MessageDeliveryStatus;
import android.hardware.contexthub.Reason;
import android.hardware.contexthub.Service;

@VintfStability
interface IEndpointCallback {
    /**
     * Lifecycle event notification for endpoint starting from remote side. There is no need to
     * report already started endpoint prior to the registration of an EndpointLifecycleCallbacks
     * object. The EndpointInfo reported here should be consistent with values from getEndpoints().
     *
     * Endpoints added by registerEndpoint should not be included. registerEndpoint() should not
     * cause this call.
     *
     * @param endpointInfos An array of EndpointInfo representing endpoints that just started.
     */
    void onEndpointStarted(in EndpointInfo[] endpointInfos);

    /**
     * Lifecycle event notification for endpoint stopping from remote side. There is no need to
     * report already stopped endpoint prior to the registration of an EndpointLifecycleCallbacks
     * object. The EndpointId reported here should represent a previously started Endpoint.
     *
     * When a hub crashes or restart, events should be batched into be a single call (containing all
     * the EndpointId that were impacted).
     *
     * Endpoints added by registerEndpoint should not be included. unregisterEndpoint() should not
     * cause this call.
     *
     * @param endpointIds An array of EndpointId representing endpoints that just stopped.
     * @param reason The reason for why the endpoints stopped.
     */
    void onEndpointStopped(in EndpointId[] endpointIds, Reason reason);

    /**
     * Invoked when an endpoint sends message to another endpoint (on host) on the (currently open)
     * session.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         openEndpointSession() or onEndpointSessionOpenRequest().
     * @param msg The Message object representing a message from endpoint to an endpoint on host.
     */
    void onMessageReceived(int sessionId, in Message msg);

    /**
     * Invoked when an endpoint sends the response for a message that requires delivery status.
     *
     * The response is the message delivery status of a recently sent message within a session. See
     * sendMessageDeliveryStatusToEndpoint() for more details.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         openEndpointSession() or onEndpointSessionOpenRequest().
     * @param msgStatus The MessageDeliveryStatus object representing the delivery status for a
     *         specific message (identified by the sequenceNumber) within the session.
     */
    void onMessageDeliveryStatusReceived(int sessionId, in MessageDeliveryStatus msgStatus);

    /**
     * Invoked when session initiation is requested by a remote endpoint. The receiving host client
     * must later call endpointSessionOpenComplete() to indicate successful connection and
     * acceptance of the session, or closeEndpointSession() to indicate failure.
     *
     * @param sessionId Caller-allocated session identifier, which must be unique across all active
     *         sessions, and must not fall in a range allocated via requestSessionIdRange().
     * @param destination The EndpointId representing the destination side of the session, which
     *         must've already been published through registerEndpoint().
     * @param initiator The EndpointId representing the initiating side of the session.
     * @param serviceDescriptor Descriptor for the service specification for scoping this session
     *         (nullable). Null indicates a fully custom marshalling scheme. The value should match
     *         a published descriptor for both endpoints.
     *
     * @throws EX_ILLEGAL_ARGUMENT if any of the arguments are invalid, or the combination of the
     *         arguments is invalid.
     */
    void onEndpointSessionOpenRequest(int sessionId, in EndpointId destination,
            in EndpointId initiator, in @nullable String serviceDescriptor);

    /**
     * Invoked when a session has either failed to open, or has been closed by the remote side.
     * Upon receiving this callback, the session is closed and further messages on it will not be
     * delivered.
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         openEndpointSession() or onEndpointSessionOpenRequest().
     * @param reason The reason for this close endpoint session notification.
     */
    void onCloseEndpointSession(int sessionId, in Reason reason);

    /**
     * Callback when a session is opened. This callback is the status callback for a previous
     * openEndpointSession().
     *
     * @param sessionId The integer representing the communication session, previously set in
     *         onEndpointSessionOpenRequest(). This id is assigned by the host.
     */
    void onEndpointSessionOpenComplete(int sessionId);
}
