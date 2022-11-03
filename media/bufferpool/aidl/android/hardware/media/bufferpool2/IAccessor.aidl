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

import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;
import android.hardware.common.fmq.UnsynchronizedWrite;


import android.hardware.media.bufferpool2.BufferInvalidationMessage;
import android.hardware.media.bufferpool2.BufferStatusMessage;
import android.hardware.media.bufferpool2.IConnection;
import android.hardware.media.bufferpool2.IObserver;

/**
 * IAccessor creates IConnection which is used from IClientManager in order to
 * use functionality of the specified buffer pool.
 */
@VintfStability
interface IAccessor {
    @VintfStability
     /**
     * Connection information between the bufferpool process and the receiver
     * process. The information is used from the receiver process in order to
     * receive buffers from the bufferpool process.
     */
    parcelable ConnectionInfo {
        /**
        * The interface to get shared buffers from the bufferpool.
        */
        IConnection connection;
        /**
         * The identifier for a (sender/receiver) pair during buffer transfer.
         * This is system wide unique.
         */
        long connectionId;
        /**
         * Id of the most recent message from bufferpool. This is monotonic.
         */
        int msgId;
        /**
         * The FMQ descriptor for sending buffer status messages back to bufferpool
         */
        MQDescriptor<BufferStatusMessage, SynchronizedReadWrite> toFmqDesc;
        /**
         * The FMQ descriptor for receiving buffer invalidation messages from bufferpool
         */
        MQDescriptor<BufferInvalidationMessage, UnsynchronizedWrite> fromFmqDesc;
    }

    /**
     * Registers a new client and creates IConnection to the buffer pool for
     * the client. IConnection and FMQ are used by IClientManager in order to
     * communicate with the buffer pool. Via FMQ IClientManager sends
     * BufferStatusMessage(s) to the buffer pool.
     *
     * FMQ is used to send buffer ownership status changes to a buffer pool
     * from a buffer pool client. A buffer pool synchronizes FMQ messages when
     * there is an aidl request from the clients. Every client has its own
     * connection and FMQ to communicate with the buffer pool. So sending an
     * FMQ message on behalf of other clients is not possible.
     *
     * FMQ messages are sent when a buffer is acquired or released. Also, FMQ
     * messages are sent when a buffer is transferred from a client to another
     * client. FMQ has its own ID from a buffer pool. A client is specified
     * with the ID.
     *
     * To transfer a buffer, a sender must send an FMQ message. The message
     * must include a receiver's ID and a transaction ID. A receiver must send
     * the transaction ID to fetch a buffer from a buffer pool. Since the
     * sender already registered the receiver via an FMQ message, The buffer
     * pool must verify the receiver with the transaction ID. In order to
     * prevent faking a receiver, a connection to a buffer pool from client is
     * made and kept private. Also part of transaction ID is a sender ID in
     * order to prevent fake transactions from other clients. This must be
     * verified with an FMQ message from a buffer pool.
     *
     * @param observer The buffer pool event observer from the client.
     *     Observer is provided to ensure FMQ messages are processed even when
     *     client processes are idle. Buffer invalidation caused by
     *     reconfiguration does not call observer. Buffer invalidation caused
     *     by termination of pipeline call observer in order to ensure
     *     invalidation is done after pipeline completion.
     * @return ConnectionInfo The information regarding the established
     *     connection
     * @@throws ServiceSpecificException with one of the following values:
     *     ResultStatus::NO_MEMORY        - Memory allocation failure occurred.
     *     ResultStatus::ALREADY_EXISTS   - A connection was already made.
     *     ResultStatus::CRITICAL_ERROR   - Other errors.
     */
    ConnectionInfo connect(in IObserver observer);
}
