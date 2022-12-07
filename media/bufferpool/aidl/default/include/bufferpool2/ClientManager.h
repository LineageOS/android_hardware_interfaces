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

#pragma once

#include <aidl/android/hardware/media/bufferpool2/IAccessor.h>
#include <aidl/android/hardware/media/bufferpool2/BnClientManager.h>
#include <memory>
#include "BufferPoolTypes.h"

namespace aidl::android::hardware::media::bufferpool2::implementation {

using aidl::android::hardware::media::bufferpool2::BnClientManager;
using aidl::android::hardware::media::bufferpool2::IClientManager;
using aidl::android::hardware::media::bufferpool2::IAccessor;

struct ClientManager : public BnClientManager {
    // Methods from ::aidl::android::hardware::media::bufferpool2::IClientManager follow.
    ::ndk::ScopedAStatus registerSender(
        const std::shared_ptr<IAccessor>& in_bufferPool,
        ::aidl::android::hardware::media::bufferpool2::IClientManager::Registration* _aidl_return)
        override;

    /** Gets an instance. */
    static std::shared_ptr<ClientManager> getInstance();

    /**
     * Creates a local connection with a newly created buffer pool.
     *
     * @param allocator     for new buffer allocation.
     * @param pConnectionId Id of the created connection. This is
     *                      system-wide unique.
     *
     * @return OK when a buffer pool and a local connection is successfully
     *         created.
     *         ResultStatus::NO_MEMORY when there is no memory.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus create(const std::shared_ptr<BufferPoolAllocator> &allocator,
                        ConnectionId *pConnectionId);

    /**
     * Register a created connection as sender for remote process.
     *
     * @param receiver      The remote receiving process.
     * @param senderId      A local connection which will send buffers to.
     * @param receiverId    Id of the created receiving connection on the receiver
     *                      process.
     * @param isNew         @true when the receiving connection is newly created.
     *
     * @return OK when the receiving connection is successfully created on the
     *         receiver process.
     *         NOT_FOUND when the sender connection was not found.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus registerSender(const std::shared_ptr<IClientManager> &receiver,
                                ConnectionId senderId,
                                ConnectionId *receiverId,
                                bool *isNew);

    /**
     * Closes the specified connection.
     *
     * @param connectionId  The id of the connection.
     *
     * @return OK when the connection is closed.
     *         NOT_FOUND when the specified connection was not found.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus close(ConnectionId connectionId);

    /**
     * Evicts cached allocations. If it's local connection, release the
     * previous allocations and do not recycle current active allocations.
     *
     * @param connectionId The id of the connection.
     *
     * @return OK when the connection is resetted.
     *         NOT_FOUND when the specified connection was not found.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus flush(ConnectionId connectionId);

    /**
     * Allocates a buffer from the specified connection. The output parameter
     * handle is cloned from the internal handle. So it is safe to use directly,
     * and it should be deleted and destroyed after use.
     *
     * @param connectionId  The id of the connection.
     * @param params        The allocation parameters.
     * @param handle        The native handle to the allocated buffer. handle
     *                      should be cloned before use.
     * @param buffer        The allocated buffer.
     *
     * @return OK when a buffer was allocated successfully.
     *         NOT_FOUND when the specified connection was not found.
     *         NO_MEMORY when there is no memory.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus allocate(ConnectionId connectionId,
                          const std::vector<uint8_t> &params,
                          native_handle_t **handle,
                          std::shared_ptr<BufferPoolData> *buffer);

    /**
     * Receives a buffer for the transaction. The output parameter handle is
     * cloned from the internal handle. So it is safe to use directly, and it
     * should be deleted and destoyed after use.
     *
     * @param connectionId  The id of the receiving connection.
     * @param transactionId The id for the transaction.
     * @param bufferId      The id for the buffer.
     * @param timestampMs   The timestamp of the buffer is being sent.
     * @param handle        The native handle to the allocated buffer. handle
     *                      should be cloned before use.
     * @param buffer        The received buffer.
     *
     * @return OK when a buffer was received successfully.
     *         NOT_FOUND when the specified connection was not found.
     *         NO_MEMORY when there is no memory.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus receive(ConnectionId connectionId,
                         TransactionId transactionId,
                         BufferId bufferId,
                         int64_t timestampMs,
                          native_handle_t **handle,
                         std::shared_ptr<BufferPoolData> *buffer);

    /**
     * Posts a buffer transfer transaction to the buffer pool. Sends a buffer
     * to other remote clients(connection) after this call has been succeeded.
     *
     * @param receiverId    The id of the receiving connection.
     * @param buffer        to transfer
     * @param transactionId Id of the transfer transaction.
     * @param timestampMs   The timestamp of the buffer transaction is being
     *                      posted.
     *
     * @return OK when a buffer transaction was posted successfully.
     *         NOT_FOUND when the sending connection was not found.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus postSend(ConnectionId receiverId,
                          const std::shared_ptr<BufferPoolData> &buffer,
                          TransactionId *transactionId,
                          int64_t *timestampMs);

    /**
     *  Time out inactive lingering connections and close.
     */
    void cleanUp();

    /** Destructs the manager of buffer pool clients.  */
    ~ClientManager();
private:
    static std::shared_ptr<ClientManager> sInstance;
    static std::mutex sInstanceLock;

    class Impl;
    const std::unique_ptr<Impl> mImpl;

    friend class ::ndk::SharedRefBase;

    ClientManager();
};

}  // namespace aidl::android::hardware::media::bufferpool2::implementation

