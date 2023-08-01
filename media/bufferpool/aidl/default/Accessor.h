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

#include <aidl/android/hardware/media/bufferpool2/BnAccessor.h>
#include <aidl/android/hardware/media/bufferpool2/IObserver.h>
#include <bufferpool2/BufferPoolTypes.h>

#include <memory>
#include <map>
#include <set>
#include <condition_variable>

#include "BufferPool.h"

namespace aidl::android::hardware::media::bufferpool2::implementation {

struct Connection;
using ::aidl::android::hardware::media::bufferpool2::IObserver;
using ::aidl::android::hardware::media::bufferpool2::IAccessor;

/**
 * Receives death notifications from remote connections.
 * On death notifications, the connections are closed and used resources
 * are released.
 */
struct ConnectionDeathRecipient {
    ConnectionDeathRecipient();
    /**
     * Registers a newly connected connection from remote processes.
     */
    void add(int64_t connectionId, const std::shared_ptr<Accessor> &accessor);

    /**
     * Removes a connection.
     */
    void remove(int64_t connectionId);

    void addCookieToConnection(void *cookie, int64_t connectionId);

    void onDead(void *cookie);

    AIBinder_DeathRecipient *getRecipient();

private:
    ::ndk::ScopedAIBinder_DeathRecipient mDeathRecipient;

    std::mutex mLock;
    std::map<void *, std::set<int64_t>>  mCookieToConnections;
    std::map<int64_t, void *> mConnectionToCookie;
    std::map<int64_t, const std::weak_ptr<Accessor>> mAccessors;
};

/**
 * A buffer pool accessor which enables a buffer pool to communicate with buffer
 * pool clients. 1:1 correspondense holds between a buffer pool and an accessor.
 */
struct Accessor : public BnAccessor {
    // Methods from ::aidl::android::hardware::media::bufferpool2::IAccessor.
    ::ndk::ScopedAStatus connect(const std::shared_ptr<IObserver>& in_observer,
                                 IAccessor::ConnectionInfo* _aidl_return) override;

    /**
     * Creates a buffer pool accessor which uses the specified allocator.
     *
     * @param allocator buffer allocator.
     */
    explicit Accessor(const std::shared_ptr<BufferPoolAllocator> &allocator);

    /** Destructs a buffer pool accessor. */
    ~Accessor();

    /** Returns whether the accessor is valid. */
    bool isValid();

    /** Invalidates all buffers which are owned by bufferpool */
    BufferPoolStatus flush();

    /** Allocates a buffer from a buffer pool.
     *
     * @param connectionId  the connection id of the client.
     * @param params        the allocation parameters.
     * @param bufferId      the id of the allocated buffer.
     * @param handle        the native handle of the allocated buffer.
     *
     * @return OK when a buffer is successfully allocated.
     *         NO_MEMORY when there is no memory.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus allocate(
            ConnectionId connectionId,
            const std::vector<uint8_t>& params,
            BufferId *bufferId,
            const native_handle_t** handle);

    /**
     * Fetches a buffer for the specified transaction.
     *
     * @param connectionId  the id of receiving connection(client).
     * @param transactionId the id of the transfer transaction.
     * @param bufferId      the id of the buffer to be fetched.
     * @param handle        the native handle of the fetched buffer.
     *
     * @return OK when a buffer is successfully fetched.
     *         NO_MEMORY when there is no memory.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus fetch(
            ConnectionId connectionId,
            TransactionId transactionId,
            BufferId bufferId,
            const native_handle_t** handle);

    /**
     * Makes a connection to the buffer pool. The buffer pool client uses the
     * created connection in order to communicate with the buffer pool. An
     * FMQ for buffer status message is also created for the client.
     *
     * @param observer      client observer for buffer invalidation
     * @param local         true when a connection request comes from local process,
     *                      false otherwise.
     * @param connection    created connection
     * @param pConnectionId the id of the created connection
     * @param pMsgId        the id of the recent buffer pool message
     * @param statusDescPtr FMQ descriptor for shared buffer status message
     *                      queue between a buffer pool and the client.
     * @param invDescPtr    FMQ descriptor for buffer invalidation message
     *                      queue from a buffer pool to the client.
     *
     * @return OK when a connection is successfully made.
     *         NO_MEMORY when there is no memory.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus connect(
            const std::shared_ptr<IObserver>& observer,
            bool local,
            std::shared_ptr<Connection> *connection, ConnectionId *pConnectionId,
            uint32_t *pMsgId,
            StatusDescriptor* statusDescPtr,
            InvalidationDescriptor* invDescPtr);

    /**
     * Closes the specified connection to the client.
     *
     * @param connectionId  the id of the connection.
     *
     * @return OK when the connection is closed.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus close(ConnectionId connectionId);

    /**
     * Processes pending buffer status messages and performs periodic cache
     * cleaning.
     *
     * @param clearCache    if clearCache is true, it frees all buffers waiting
     *                      to be recycled.
     */
    void cleanUp(bool clearCache);

    /**
     * ACK on buffer invalidation messages
     */
    void handleInvalidateAck();

    /**
     * Gets a death_recipient for remote connection death.
     */
    static std::shared_ptr<ConnectionDeathRecipient> getConnectionDeathRecipient();

    static void createInvalidator();

    static void createEvictor();

private:
    // ConnectionId = pid : (timestamp_created + seqId)
    // in order to guarantee uniqueness for each connection
    static uint32_t sSeqId;

    const std::shared_ptr<BufferPoolAllocator> mAllocator;
    nsecs_t mScheduleEvictTs;
    BufferPool mBufferPool;

    struct  AccessorInvalidator {
        std::map<uint32_t, const std::weak_ptr<Accessor>> mAccessors;
        std::mutex mMutex;
        std::condition_variable mCv;
        bool mReady;

        AccessorInvalidator();
        void addAccessor(uint32_t accessorId, const std::weak_ptr<Accessor> &accessor);
        void delAccessor(uint32_t accessorId);
    };

    static std::unique_ptr<AccessorInvalidator> sInvalidator;

    static void invalidatorThread(
        std::map<uint32_t, const std::weak_ptr<Accessor>> &accessors,
        std::mutex &mutex,
        std::condition_variable &cv,
        bool &ready);

    struct AccessorEvictor {
        std::map<const std::weak_ptr<Accessor>, nsecs_t, std::owner_less<>> mAccessors;
        std::mutex mMutex;
        std::condition_variable mCv;

        AccessorEvictor();
        void addAccessor(const std::weak_ptr<Accessor> &accessor, nsecs_t ts);
    };

    static std::unique_ptr<AccessorEvictor> sEvictor;

    static void evictorThread(
        std::map<const std::weak_ptr<Accessor>, nsecs_t, std::owner_less<>> &accessors,
        std::mutex &mutex,
        std::condition_variable &cv);

    void scheduleEvictIfNeeded();

    friend struct BufferPool;
};

}  // namespace aidl::android::hardware::media::bufferpool2::implementation
