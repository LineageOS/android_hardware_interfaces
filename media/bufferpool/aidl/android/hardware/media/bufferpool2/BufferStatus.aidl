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

package android.hardware.media.bufferpool2;

/**
 * Buffer ownership status for the specified client.
 * Buffer transfer status for the specified buffer transafer transaction.
 * BufferStatus is posted along with BufferStatusMessage from a client to
 * the buffer pool for synchronization after status change.
 */
@VintfStability
@Backing(type="int")
enum BufferStatus {
    /**
     * No longer used by the specified client.
     */
    NOT_USED = 0,
    /**
     * Buffer is acquired by the specified client.
     */
    USED = 1,
    /**
     * Buffer is sent by the specified client.
     */
    TRANSFER_TO = 2,
    /**
     * Buffer transfer is acked by the receiver client.
     */
    TRANSFER_FROM = 3,
    /**
     * Buffer transfer is timed out by receiver client.
     */
    TRANSFER_TIMEOUT = 4,
    /**
     * Buffer transfer is not acked by the receiver.
     */
    TRANSFER_LOST = 5,
    /**
     * Buffer fetch request from the client.
     */
    TRANSFER_FETCH = 6,
    /**
     * Buffer transaction succeeded.
     */
    TRANSFER_OK = 7,
    /**
     * Buffer transaction failure.
     */
    TRANSFER_ERROR = 8,
    /**
     * Buffer invalidation ack.
     */
    INVALIDATION_ACK = 9,
}
