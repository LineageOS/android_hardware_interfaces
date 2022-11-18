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

import android.hardware.media.bufferpool2.IAccessor;

/**
 * IClientManager manages IConnection(s) inside a process. A locally
 * created IConnection represents a communication node(receiver) with the
 * specified buffer pool(IAccessor).
 * IConnection(s) are not exposed to other processes(IClientManager).
 * IClientManager instance must be unique within a process.
 */
@VintfStability
interface IClientManager {
    /**
     * Result of registerSender.
     */
    @VintfStability
    parcelable Registration {
        /** registered connection id    */
        long connectionId;
        /** true when the connection is new */
        boolean isNew = true;
    }
    /**
     * Sets up a buffer receiving communication node for the specified
     * buffer pool. A manager must create a IConnection to the buffer
     * pool if it does not already have a connection.
     *
     * @param bufferPool a buffer pool which is specified with the IAccessor.
     *     The specified buffer pool is the owner of received buffers.
     * @return the Id of the communication node to the buffer pool.
     *     This id is used in FMQ to notify IAccessor that a buffer has been
     *     sent to that connection during transfers.
     * @throws ServiceSpecificException with one of the following values:
     *     ResultStatus::NO_MEMORY        - Memory allocation failure occurred.
     *     ResultStatus::CRITICAL_ERROR   - Other errors.
     */
    Registration registerSender(in IAccessor bufferPool);
}
