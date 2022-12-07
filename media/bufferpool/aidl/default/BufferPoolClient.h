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

#include <memory>
#include <aidl/android/hardware/media/bufferpool2/IAccessor.h>
#include <aidl/android/hardware/media/bufferpool2/IObserver.h>
#include <bufferpool2/BufferPoolTypes.h>

namespace aidl::android::hardware::media::bufferpool2::implementation {

using aidl::android::hardware::media::bufferpool2::IAccessor;
using aidl::android::hardware::media::bufferpool2::IObserver;

struct Accessor;

/**
 * A buffer pool client for a buffer pool. For a specific buffer pool, at most
 * one buffer pool client exists per process. This class will not be exposed
 * outside. A buffer pool client will be used via ClientManager.
 */
class BufferPoolClient {
public:
    /**
     * Creates a buffer pool client from a local buffer pool
     * (via ClientManager#create).
     */
    explicit BufferPoolClient(const std::shared_ptr<Accessor> &accessor,
                              const std::shared_ptr<IObserver> &observer);

    /**
     * Creates a buffer pool client from a remote buffer pool
     * (via ClientManager#registerSender).
     * Note: A buffer pool client created with remote buffer pool cannot
     * allocate a buffer.
     */
    explicit BufferPoolClient(const std::shared_ptr<IAccessor> &accessor,
                              const std::shared_ptr<IObserver> &observer);

    /** Destructs a buffer pool client. */
    ~BufferPoolClient();

private:
    bool isValid();

    bool isLocal();

    bool isActive(int64_t *lastTransactionMs, bool clearCache);

    ConnectionId getConnectionId();

    BufferPoolStatus getAccessor(std::shared_ptr<IAccessor> *accessor);

    void receiveInvalidation(uint32_t msgId);

    BufferPoolStatus flush();

    BufferPoolStatus allocate(const std::vector<uint8_t> &params,
                          native_handle_t **handle,
                          std::shared_ptr<BufferPoolData> *buffer);

    BufferPoolStatus receive(TransactionId transactionId,
                         BufferId bufferId,
                         int64_t timestampMs,
                         native_handle_t **handle,
                         std::shared_ptr<BufferPoolData> *buffer);

    BufferPoolStatus postSend(ConnectionId receiver,
                          const std::shared_ptr<BufferPoolData> &buffer,
                          TransactionId *transactionId,
                          int64_t *timestampMs);

    class Impl;
    std::shared_ptr<Impl> mImpl;

    friend struct ClientManager;
    friend struct Observer;
};

}  // namespace aidl::android::hardware::bufferpool2::implementation
