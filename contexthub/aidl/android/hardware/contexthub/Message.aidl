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

@VintfStability
parcelable Message {
    /**
     * Bitmask for flags field if this message requires a MessageDeliveryStatus for the
     * sequenceNumber within 1 second.
     */
    const int FLAG_REQUIRES_DELIVERY_STATUS = 1;

    /** Bitset of flags */
    int flags;

    /** Sequence number of this message */
    int sequenceNumber;

    /**
     * Per message permission (used for app-op permission attribution).
     */
    String[] permissions;

    /**
     * The type of this message payload, following a scheme specific to the service or sending
     * endpoint's communication protocol. This value can be used to distinguish the handling of
     * content (e.g. for decoding). This could also be used as the complete content of the message
     * if no additional payload is needed.
     */
    int type;

    /**
     * Content (payload) of the message. The format of the message is specific to the context of the
     * message: the service or endpoints involved in the session, and the message type.
     */
    byte[] content;
}
