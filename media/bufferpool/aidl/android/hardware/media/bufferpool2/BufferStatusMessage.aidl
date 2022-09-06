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

import android.hardware.media.bufferpool2.BufferStatus;

/**
 * Buffer ownership status change message. This message is
 * sent via fmq to the buffer pool from client processes.
 */
@VintfStability
@FixedSize
parcelable BufferStatusMessage {
    /**
     * Transaction Id = (SenderId : sender local transaction Id)
     * Transaction Id is created from sender and posted via fmq within
     * TRANSFER_TO message.
     */
    long transactionId;
    int bufferId;
    BufferStatus status;
    /**
     * Used by the buffer pool, not by client.
     */
    long connectionId;
    /**
     * Valid only when TRANSFER_TO is posted.
     */
    long targetConnectionId;
    /**
     * Used by the buffer pool, not by client.
     * Monotonic timestamp in Us since fixed point in time as decided
     * by the sender of the message
     */
    long timestampUs;
}
