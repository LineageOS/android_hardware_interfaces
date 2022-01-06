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

@VintfStability
parcelable ContextHubMessage {
    /** The unique identifier of the nanoapp. */
    long nanoappId;

    /**
     * The identifier of the host client that is sending/receiving this message.
     *
     * There are two reserved values of the host endpoint that has a specific meaning:
     * 1) BROADCAST = 0xFFFF: see CHRE_HOST_ENDPOINT_BROADCAST in
     *    system/chre/chre_api/include/chre_api/chre/event.h for details.
     * 2) UNSPECIFIED = 0xFFFE: see CHRE_HOST_ENDPOINT_UNSPECIFIED in
     *    system/chre/chre_api/include/chre_api/chre/event.h for details.
     */
    char hostEndPoint;

    /**
     * The type of this message payload, defined by the communication endpoints (i.e.
     * either the nanoapp or the host endpoint). This value can be used to distinguish
     * the handling of messageBody (e.g. for decoding).
     */
    int messageType;

    /** The payload containing the message. */
    byte[] messageBody;

    /**
     * The list of Android permissions held by the sending nanoapp at the time
     * the message was sent.
     *
     * The framework MUST drop messages to host apps that don't have a superset
     * of the permissions that the sending nanoapp is using.
     */
    String[] permissions;
}
