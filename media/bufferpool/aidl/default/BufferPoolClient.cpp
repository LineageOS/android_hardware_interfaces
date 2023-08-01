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

#define LOG_TAG "AidlBufferPoolCli"
//#define LOG_NDEBUG 0

#include <thread>
#include <aidlcommonsupport/NativeHandle.h>
#include <utils/Log.h>
#include "BufferPoolClient.h"
#include "Accessor.h"
#include "Connection.h"

namespace aidl::android::hardware::media::bufferpool2::implementation {

using aidl::android::hardware::media::bufferpool2::IConnection;
using aidl::android::hardware::media::bufferpool2::ResultStatus;
using FetchInfo = aidl::android::hardware::media::bufferpool2::IConnection::FetchInfo;
using FetchResult = aidl::android::hardware::media::bufferpool2::IConnection::FetchResult;

static constexpr int64_t kReceiveTimeoutMs = 2000; // 2s
static constexpr int kPostMaxRetry = 3;
static constexpr int kCacheTtlMs = 1000;
static constexpr size_t kMaxCachedBufferCount = 64;
static constexpr size_t kCachedBufferCountTarget = kMaxCachedBufferCount - 16;

class BufferPoolClient::Impl
        : public std::enable_shared_from_this<BufferPoolClient::Impl> {
public:
    explicit Impl(const std::shared_ptr<Accessor> &accessor,
                  const std::shared_ptr<IObserver> &observer);

    explicit Impl(const std::shared_ptr<IAccessor> &accessor,
                  const std::shared_ptr<IObserver> &observer);

    bool isValid() {
        return mValid;
    }

    bool isLocal() {
        return mValid && mLocal;
    }

    ConnectionId getConnectionId() {
        return mConnectionId;
    }

    std::shared_ptr<IAccessor> &getAccessor() {
        return mAccessor;
    }

    bool isActive(int64_t *lastTransactionMs, bool clearCache);

    void receiveInvalidation(uint32_t msgID);

    BufferPoolStatus flush();

    BufferPoolStatus allocate(const std::vector<uint8_t> &params,
                          native_handle_t **handle,
                          std::shared_ptr<BufferPoolData> *buffer);

    BufferPoolStatus receive(
            TransactionId transactionId, BufferId bufferId,
            int64_t timestampMs,
            native_handle_t **handle, std::shared_ptr<BufferPoolData> *buffer);

    void postBufferRelease(BufferId bufferId);

    bool postSend(
            BufferId bufferId, ConnectionId receiver,
            TransactionId *transactionId, int64_t *timestampMs);
private:

    bool postReceive(
            BufferId bufferId, TransactionId transactionId,
            int64_t timestampMs);

    bool postReceiveResult(
            BufferId bufferId, TransactionId transactionId, bool result, bool *needsSync);

    void trySyncFromRemote();

    bool syncReleased(uint32_t msgId = 0);

    void evictCaches(bool clearCache = false);

    void invalidateBuffer(BufferId id);

    void invalidateRange(BufferId from, BufferId to);

    BufferPoolStatus allocateBufferHandle(
            const std::vector<uint8_t>& params, BufferId *bufferId,
            native_handle_t **handle);

    BufferPoolStatus fetchBufferHandle(
            TransactionId transactionId, BufferId bufferId,
            native_handle_t **handle);

    struct BlockPoolDataDtor;
    struct ClientBuffer;

    bool mLocal;
    bool mValid;
    std::shared_ptr<IAccessor> mAccessor;
    std::shared_ptr<Connection> mLocalConnection;
    std::shared_ptr<IConnection> mRemoteConnection;
    uint32_t mSeqId;
    ConnectionId mConnectionId;
    int64_t mLastEvictCacheMs;
    std::unique_ptr<BufferInvalidationListener> mInvalidationListener;

    // CachedBuffers
    struct BufferCache {
        std::mutex mLock;
        bool mCreating;
        std::condition_variable mCreateCv;
        std::map<BufferId, std::unique_ptr<ClientBuffer>> mBuffers;
        int mActive;
        int64_t mLastChangeMs;

        BufferCache() : mCreating(false), mActive(0),
                mLastChangeMs(::android::elapsedRealtime()) {}

        void incActive_l() {
            ++mActive;
            mLastChangeMs = ::android::elapsedRealtime();
        }

        void decActive_l() {
            --mActive;
            mLastChangeMs = ::android::elapsedRealtime();
        }

        int cachedBufferCount() const {
            return mBuffers.size() - mActive;
        }
    } mCache;

    // FMQ - release notifier
    struct ReleaseCache {
        std::mutex mLock;
        std::list<BufferId> mReleasingIds;
        std::list<BufferId> mReleasedIds;
        uint32_t mInvalidateId; // TODO: invalidation ACK to bufferpool
        bool mInvalidateAck;
        std::unique_ptr<BufferStatusChannel> mStatusChannel;

        ReleaseCache() : mInvalidateId(0), mInvalidateAck(true) {}
    } mReleasing;

    // This lock is held during synchronization from remote side.
    // In order to minimize remote calls and locking duration, this lock is held
    // by best effort approach using try_lock().
    std::mutex mRemoteSyncLock;
};

struct BufferPoolClient::Impl::BlockPoolDataDtor {
    BlockPoolDataDtor(const std::shared_ptr<BufferPoolClient::Impl> &impl)
            : mImpl(impl) {}

    void operator()(BufferPoolData *buffer) {
        BufferId id = buffer->mId;
        delete buffer;

        auto impl = mImpl.lock();
        if (impl && impl->isValid()) {
            impl->postBufferRelease(id);
        }
    }
    const std::weak_ptr<BufferPoolClient::Impl> mImpl;
};

struct BufferPoolClient::Impl::ClientBuffer {
private:
    int64_t mExpireMs;
    bool mHasCache;
    ConnectionId mConnectionId;
    BufferId mId;
    native_handle_t *mHandle;
    std::weak_ptr<BufferPoolData> mCache;

    void updateExpire() {
        mExpireMs = ::android::elapsedRealtime() + kCacheTtlMs;
    }

public:
    ClientBuffer(
            ConnectionId connectionId, BufferId id, native_handle_t *handle)
            : mHasCache(false), mConnectionId(connectionId),
              mId(id), mHandle(handle) {
        mExpireMs = ::android::elapsedRealtime() + kCacheTtlMs;
    }

    ~ClientBuffer() {
        if (mHandle) {
            native_handle_close(mHandle);
            native_handle_delete(mHandle);
        }
    }

    BufferId id() const {
        return mId;
    }

    bool expire() const {
        int64_t now = ::android::elapsedRealtime();
        return now >= mExpireMs;
    }

    bool hasCache() const {
        return mHasCache;
    }

    std::shared_ptr<BufferPoolData> fetchCache(native_handle_t **pHandle) {
        if (mHasCache) {
            std::shared_ptr<BufferPoolData> cache = mCache.lock();
            if (cache) {
                *pHandle = mHandle;
            }
            return cache;
        }
        return nullptr;
    }

    std::shared_ptr<BufferPoolData> createCache(
            const std::shared_ptr<BufferPoolClient::Impl> &impl,
            native_handle_t **pHandle) {
        if (!mHasCache) {
            // Allocates a raw ptr in order to avoid sending #postBufferRelease
            // from deleter, in case of native_handle_clone failure.
            BufferPoolData *ptr = new BufferPoolData(mConnectionId, mId);
            if (ptr) {
                std::shared_ptr<BufferPoolData> cache(ptr, BlockPoolDataDtor(impl));
                if (cache) {
                    mCache = cache;
                    mHasCache = true;
                    *pHandle = mHandle;
                    return cache;
                }
            }
            if (ptr) {
                delete ptr;
            }
        }
        return nullptr;
    }

    bool onCacheRelease() {
        if (mHasCache) {
            // TODO: verify mCache is not valid;
            updateExpire();
            mHasCache = false;
            return true;
        }
        return false;
    }
};

BufferPoolClient::Impl::Impl(const std::shared_ptr<Accessor> &accessor,
                             const std::shared_ptr<IObserver> &observer)
    : mLocal(true), mValid(false), mAccessor(accessor), mSeqId(0),
      mLastEvictCacheMs(::android::elapsedRealtime()) {
    StatusDescriptor statusDesc;
    InvalidationDescriptor invDesc;
    BufferPoolStatus status = accessor->connect(
            observer, true,
            &mLocalConnection, &mConnectionId, &mReleasing.mInvalidateId,
            &statusDesc, &invDesc);
    if (status == ResultStatus::OK) {
        mReleasing.mStatusChannel =
                std::make_unique<BufferStatusChannel>(statusDesc);
        mInvalidationListener =
                std::make_unique<BufferInvalidationListener>(invDesc);
        mValid = mReleasing.mStatusChannel &&
                mReleasing.mStatusChannel->isValid() &&
                mInvalidationListener &&
                mInvalidationListener->isValid();
    }
}

BufferPoolClient::Impl::Impl(const std::shared_ptr<IAccessor> &accessor,
                             const std::shared_ptr<IObserver> &observer)
    : mLocal(false), mValid(false), mAccessor(accessor), mSeqId(0),
      mLastEvictCacheMs(::android::elapsedRealtime()) {
    IAccessor::ConnectionInfo conInfo;
    bool valid = false;
    if(accessor->connect(observer, &conInfo).isOk()) {
        auto channel = std::make_unique<BufferStatusChannel>(conInfo.toFmqDesc);
        auto observer = std::make_unique<BufferInvalidationListener>(conInfo.fromFmqDesc);

        if (channel && channel->isValid()
            && observer && observer->isValid()) {
            mRemoteConnection = conInfo.connection;
            mConnectionId = conInfo.connectionId;
            mReleasing.mInvalidateId = conInfo.msgId;
            mReleasing.mStatusChannel = std::move(channel);
            mInvalidationListener = std::move(observer);
            valid = true;
        }
    }
    mValid = valid;
}

bool BufferPoolClient::Impl::isActive(int64_t *lastTransactionMs, bool clearCache) {
    bool active = false;
    {
        std::lock_guard<std::mutex> lock(mCache.mLock);
        syncReleased();
        evictCaches(clearCache);
        *lastTransactionMs = mCache.mLastChangeMs;
        active = mCache.mActive > 0;
    }
    if (mValid && mLocal && mLocalConnection) {
        mLocalConnection->cleanUp(clearCache);
        return true;
    }
    return active;
}

void BufferPoolClient::Impl::receiveInvalidation(uint32_t messageId) {
    std::lock_guard<std::mutex> lock(mCache.mLock);
    syncReleased(messageId);
    // TODO: evict cache required?
}

BufferPoolStatus BufferPoolClient::Impl::flush() {
    if (!mLocal || !mLocalConnection || !mValid) {
        return ResultStatus::CRITICAL_ERROR;
    }
    {
        std::unique_lock<std::mutex> lock(mCache.mLock);
        syncReleased();
        evictCaches();
        return mLocalConnection->flush();
    }
}

BufferPoolStatus BufferPoolClient::Impl::allocate(
        const std::vector<uint8_t> &params,
        native_handle_t **pHandle,
        std::shared_ptr<BufferPoolData> *buffer) {
    if (!mLocal || !mLocalConnection || !mValid) {
        return ResultStatus::CRITICAL_ERROR;
    }
    BufferId bufferId;
    native_handle_t *handle = nullptr;
    buffer->reset();
    BufferPoolStatus status = allocateBufferHandle(params, &bufferId, &handle);
    if (status == ResultStatus::OK) {
        if (handle) {
            std::unique_lock<std::mutex> lock(mCache.mLock);
            syncReleased();
            evictCaches();
            auto cacheIt = mCache.mBuffers.find(bufferId);
            if (cacheIt != mCache.mBuffers.end()) {
                // TODO: verify it is recycled. (not having active ref)
                mCache.mBuffers.erase(cacheIt);
            }
            auto clientBuffer = std::make_unique<ClientBuffer>(
                    mConnectionId, bufferId, handle);
            if (clientBuffer) {
                auto result = mCache.mBuffers.insert(std::make_pair(
                        bufferId, std::move(clientBuffer)));
                if (result.second) {
                    *buffer = result.first->second->createCache(
                            shared_from_this(), pHandle);
                    if (*buffer) {
                        mCache.incActive_l();
                    }
                }
            }
        }
        if (!*buffer) {
            ALOGV("client cache creation failure %d: %lld",
                  handle != nullptr, (long long)mConnectionId);
            status = ResultStatus::NO_MEMORY;
            postBufferRelease(bufferId);
        }
    }
    return status;
}

BufferPoolStatus BufferPoolClient::Impl::receive(
        TransactionId transactionId, BufferId bufferId, int64_t timestampMs,
        native_handle_t **pHandle,
        std::shared_ptr<BufferPoolData> *buffer) {
    if (!mValid) {
        return ResultStatus::CRITICAL_ERROR;
    }
    if (timestampMs != 0) {
        timestampMs += kReceiveTimeoutMs;
    }
    if (!postReceive(bufferId, transactionId, timestampMs)) {
        return ResultStatus::CRITICAL_ERROR;
    }
    BufferPoolStatus status = ResultStatus::CRITICAL_ERROR;
    buffer->reset();
    while(1) {
        std::unique_lock<std::mutex> lock(mCache.mLock);
        syncReleased();
        evictCaches();
        auto cacheIt = mCache.mBuffers.find(bufferId);
        if (cacheIt != mCache.mBuffers.end()) {
            if (cacheIt->second->hasCache()) {
                *buffer = cacheIt->second->fetchCache(pHandle);
                if (!*buffer) {
                    // check transfer time_out
                    lock.unlock();
                    std::this_thread::yield();
                    continue;
                }
                ALOGV("client receive from reference %lld", (long long)mConnectionId);
                break;
            } else {
                *buffer = cacheIt->second->createCache(shared_from_this(), pHandle);
                if (*buffer) {
                    mCache.incActive_l();
                }
                ALOGV("client receive from cache %lld", (long long)mConnectionId);
                break;
            }
        } else {
            if (!mCache.mCreating) {
                mCache.mCreating = true;
                lock.unlock();
                native_handle_t* handle = nullptr;
                status = fetchBufferHandle(transactionId, bufferId, &handle);
                lock.lock();
                if (status == ResultStatus::OK) {
                    if (handle) {
                        auto clientBuffer = std::make_unique<ClientBuffer>(
                                mConnectionId, bufferId, handle);
                        if (clientBuffer) {
                            auto result = mCache.mBuffers.insert(
                                    std::make_pair(bufferId, std::move(
                                            clientBuffer)));
                            if (result.second) {
                                *buffer = result.first->second->createCache(
                                        shared_from_this(), pHandle);
                                if (*buffer) {
                                    mCache.incActive_l();
                                }
                            }
                        }
                    }
                    if (!*buffer) {
                        status = ResultStatus::NO_MEMORY;
                    }
                }
                mCache.mCreating = false;
                lock.unlock();
                mCache.mCreateCv.notify_all();
                break;
            }
            mCache.mCreateCv.wait(lock);
        }
    }
    bool needsSync = false;
    bool posted = postReceiveResult(bufferId, transactionId,
                                      *buffer ? true : false, &needsSync);
    ALOGV("client receive %lld - %u : %s (%d)", (long long)mConnectionId, bufferId,
          *buffer ? "ok" : "fail", posted);
    if (mValid && mLocal && mLocalConnection) {
        mLocalConnection->cleanUp(false);
    }
    if (needsSync && mRemoteConnection) {
        trySyncFromRemote();
    }
    if (*buffer) {
        if (!posted) {
            buffer->reset();
            return ResultStatus::CRITICAL_ERROR;
        }
        return ResultStatus::OK;
    }
    return status;
}


void BufferPoolClient::Impl::postBufferRelease(BufferId bufferId) {
    std::lock_guard<std::mutex> lock(mReleasing.mLock);
    mReleasing.mReleasingIds.push_back(bufferId);
    mReleasing.mStatusChannel->postBufferRelease(
            mConnectionId, mReleasing.mReleasingIds, mReleasing.mReleasedIds);
}

// TODO: revise ad-hoc posting data structure
bool BufferPoolClient::Impl::postSend(
        BufferId bufferId, ConnectionId receiver,
        TransactionId *transactionId, int64_t *timestampMs) {
    {
        // TODO: don't need to call syncReleased every time
        std::lock_guard<std::mutex> lock(mCache.mLock);
        syncReleased();
    }
    bool ret = false;
    bool needsSync = false;
    {
        std::lock_guard<std::mutex> lock(mReleasing.mLock);
        *timestampMs = ::android::elapsedRealtime();
        *transactionId = (mConnectionId << 32) | mSeqId++;
        // TODO: retry, add timeout, target?
        ret =  mReleasing.mStatusChannel->postBufferStatusMessage(
                *transactionId, bufferId, BufferStatus::TRANSFER_TO, mConnectionId,
                receiver, mReleasing.mReleasingIds, mReleasing.mReleasedIds);
        needsSync = !mLocal && mReleasing.mStatusChannel->needsSync();
    }
    if (mValid && mLocal && mLocalConnection) {
        mLocalConnection->cleanUp(false);
    }
    if (needsSync && mRemoteConnection) {
        trySyncFromRemote();
    }
    return ret;
}

bool BufferPoolClient::Impl::postReceive(
        BufferId bufferId, TransactionId transactionId, int64_t timestampMs) {
    for (int i = 0; i < kPostMaxRetry; ++i) {
        std::unique_lock<std::mutex> lock(mReleasing.mLock);
        int64_t now = ::android::elapsedRealtime();
        if (timestampMs == 0 || now < timestampMs) {
            bool result = mReleasing.mStatusChannel->postBufferStatusMessage(
                    transactionId, bufferId, BufferStatus::TRANSFER_FROM,
                    mConnectionId, -1, mReleasing.mReleasingIds,
                    mReleasing.mReleasedIds);
            if (result) {
                return true;
            }
            lock.unlock();
            std::this_thread::yield();
        } else {
            mReleasing.mStatusChannel->postBufferStatusMessage(
                    transactionId, bufferId, BufferStatus::TRANSFER_TIMEOUT,
                    mConnectionId, -1, mReleasing.mReleasingIds,
                    mReleasing.mReleasedIds);
            return false;
        }
    }
    return false;
}

bool BufferPoolClient::Impl::postReceiveResult(
        BufferId bufferId, TransactionId transactionId, bool result, bool *needsSync) {
    std::lock_guard<std::mutex> lock(mReleasing.mLock);
    // TODO: retry, add timeout
    bool ret = mReleasing.mStatusChannel->postBufferStatusMessage(
            transactionId, bufferId,
            result ? BufferStatus::TRANSFER_OK : BufferStatus::TRANSFER_ERROR,
            mConnectionId, -1, mReleasing.mReleasingIds,
            mReleasing.mReleasedIds);
    *needsSync = !mLocal && mReleasing.mStatusChannel->needsSync();
    return ret;
}

void BufferPoolClient::Impl::trySyncFromRemote() {
    if (mRemoteSyncLock.try_lock()) {
        bool needsSync = false;
        {
            std::lock_guard<std::mutex> lock(mReleasing.mLock);
            needsSync = mReleasing.mStatusChannel->needsSync();
        }
        if (needsSync) {
            if (!mRemoteConnection->sync().isOk()) {
                ALOGD("sync from client %lld failed: bufferpool process died.",
                      (long long)mConnectionId);
            }
        }
        mRemoteSyncLock.unlock();
    }
}

// should have mCache.mLock
bool BufferPoolClient::Impl::syncReleased(uint32_t messageId) {
    bool cleared = false;
    {
        std::lock_guard<std::mutex> lock(mReleasing.mLock);
        if (mReleasing.mReleasingIds.size() > 0) {
            mReleasing.mStatusChannel->postBufferRelease(
                    mConnectionId, mReleasing.mReleasingIds,
                    mReleasing.mReleasedIds);
        }
        if (mReleasing.mReleasedIds.size() > 0) {
            for (BufferId& id: mReleasing.mReleasedIds) {
                ALOGV("client release buffer %lld - %u", (long long)mConnectionId, id);
                auto found = mCache.mBuffers.find(id);
                if (found != mCache.mBuffers.end()) {
                    if (found->second->onCacheRelease()) {
                        mCache.decActive_l();
                    } else {
                        // should not happen!
                        ALOGW("client %lld cache release status inconsistent!",
                            (long long)mConnectionId);
                    }
                } else {
                    // should not happen!
                    ALOGW("client %lld cache status inconsistent!", (long long)mConnectionId);
                }
            }
            mReleasing.mReleasedIds.clear();
            cleared = true;
        }
    }
    std::vector<BufferInvalidationMessage> invalidations;
    mInvalidationListener->getInvalidations(invalidations);
    uint32_t lastMsgId = 0;
    if (invalidations.size() > 0) {
        for (auto it = invalidations.begin(); it != invalidations.end(); ++it) {
            if (it->messageId != 0) {
                lastMsgId = it->messageId;
            }
            if (it->fromBufferId == it->toBufferId) {
                // TODO: handle fromBufferId = UINT32_MAX
                invalidateBuffer(it->fromBufferId);
            } else {
                invalidateRange(it->fromBufferId, it->toBufferId);
            }
        }
    }
    {
        std::lock_guard<std::mutex> lock(mReleasing.mLock);
        if (lastMsgId != 0) {
            if (isMessageLater(lastMsgId, mReleasing.mInvalidateId)) {
                mReleasing.mInvalidateId = lastMsgId;
                mReleasing.mInvalidateAck = false;
            }
        } else if (messageId != 0) {
            // messages are drained.
            if (isMessageLater(messageId, mReleasing.mInvalidateId)) {
                mReleasing.mInvalidateId = messageId;
                mReleasing.mInvalidateAck = true;
            }
        }
        if (!mReleasing.mInvalidateAck) {
            // post ACK
            mReleasing.mStatusChannel->postBufferInvalidateAck(
                    mConnectionId,
                    mReleasing.mInvalidateId, &mReleasing.mInvalidateAck);
            ALOGV("client %lld invalidateion ack (%d) %u",
                (long long)mConnectionId,
                mReleasing.mInvalidateAck, mReleasing.mInvalidateId);
        }
    }
    return cleared;
}

// should have mCache.mLock
void BufferPoolClient::Impl::evictCaches(bool clearCache) {
    int64_t now = ::android::elapsedRealtime();
    if (now >= mLastEvictCacheMs + kCacheTtlMs ||
            clearCache || mCache.cachedBufferCount() > kMaxCachedBufferCount) {
        size_t evicted = 0;
        for (auto it = mCache.mBuffers.begin(); it != mCache.mBuffers.end();) {
            if (!it->second->hasCache() && (it->second->expire() ||
                        clearCache || mCache.cachedBufferCount() > kCachedBufferCountTarget)) {
                it = mCache.mBuffers.erase(it);
                ++evicted;
            } else {
                ++it;
            }
        }
        ALOGV("cache count %lld : total %zu, active %d, evicted %zu",
              (long long)mConnectionId, mCache.mBuffers.size(), mCache.mActive, evicted);
        mLastEvictCacheMs = now;
    }
}

// should have mCache.mLock
void BufferPoolClient::Impl::invalidateBuffer(BufferId id) {
    for (auto it = mCache.mBuffers.begin(); it != mCache.mBuffers.end(); ++it) {
        if (id == it->second->id()) {
            if (!it->second->hasCache()) {
                mCache.mBuffers.erase(it);
                ALOGV("cache invalidated %lld : buffer %u",
                      (long long)mConnectionId, id);
            } else {
                ALOGW("Inconsistent invalidation %lld : activer buffer!! %u",
                      (long long)mConnectionId, (unsigned int)id);
            }
            break;
        }
    }
}

// should have mCache.mLock
void BufferPoolClient::Impl::invalidateRange(BufferId from, BufferId to) {
    size_t invalidated = 0;
    for (auto it = mCache.mBuffers.begin(); it != mCache.mBuffers.end();) {
        if (!it->second->hasCache()) {
            BufferId bid = it->second->id();
            if (from < to) {
                if (from <= bid && bid < to) {
                    ++invalidated;
                    it = mCache.mBuffers.erase(it);
                    continue;
                }
            } else {
                if (from <= bid || bid < to) {
                    ++invalidated;
                    it = mCache.mBuffers.erase(it);
                    continue;
                }
            }
        }
        ++it;
    }
    ALOGV("cache invalidated %lld : # of invalidated %zu",
          (long long)mConnectionId, invalidated);
}

BufferPoolStatus BufferPoolClient::Impl::allocateBufferHandle(
        const std::vector<uint8_t>& params, BufferId *bufferId,
        native_handle_t** handle) {
    if (mLocalConnection) {
        const native_handle_t* allocHandle = nullptr;
        BufferPoolStatus status = mLocalConnection->allocate(
                params, bufferId, &allocHandle);
        if (status == ResultStatus::OK) {
            *handle = native_handle_clone(allocHandle);
        }
        ALOGV("client allocate result %lld %d : %u clone %p",
              (long long)mConnectionId, status == ResultStatus::OK,
              *handle ? *bufferId : 0 , *handle);
        return status;
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus BufferPoolClient::Impl::fetchBufferHandle(
        TransactionId transactionId, BufferId bufferId,
        native_handle_t **handle) {
    std::shared_ptr<IConnection> connection;
    if (mLocal) {
        connection = mLocalConnection;
    } else {
        connection = mRemoteConnection;
    }
    std::vector<FetchInfo> infos;
    std::vector<FetchResult> results;
    infos.emplace_back(FetchInfo{ToAidl(transactionId), ToAidl(bufferId)});
    ndk::ScopedAStatus status = connection->fetch(infos, &results);
    if (!status.isOk()) {
        BufferPoolStatus svcSpecific = status.getServiceSpecificError();
        return svcSpecific ? svcSpecific : ResultStatus::CRITICAL_ERROR;
    }
    if (results[0].getTag() == FetchResult::buffer) {
        *handle = ::android::dupFromAidl(results[0].get<FetchResult::buffer>().buffer);
        return ResultStatus::OK;
    }
    return results[0].get<FetchResult::failure>();
}


BufferPoolClient::BufferPoolClient(const std::shared_ptr<Accessor> &accessor,
                                   const std::shared_ptr<IObserver> &observer) {
    mImpl = std::make_shared<Impl>(accessor, observer);
}

BufferPoolClient::BufferPoolClient(const std::shared_ptr<IAccessor> &accessor,
                                   const std::shared_ptr<IObserver> &observer) {
    mImpl = std::make_shared<Impl>(accessor, observer);
}

BufferPoolClient::~BufferPoolClient() {
    // TODO: how to handle orphaned buffers?
}

bool BufferPoolClient::isValid() {
    return mImpl && mImpl->isValid();
}

bool BufferPoolClient::isLocal() {
    return mImpl && mImpl->isLocal();
}

bool BufferPoolClient::isActive(int64_t *lastTransactionMs, bool clearCache) {
    if (!isValid()) {
        *lastTransactionMs = 0;
        return false;
    }
    return mImpl->isActive(lastTransactionMs, clearCache);
}

ConnectionId BufferPoolClient::getConnectionId() {
    if (isValid()) {
        return mImpl->getConnectionId();
    }
    return -1;
}

BufferPoolStatus BufferPoolClient::getAccessor(std::shared_ptr<IAccessor> *accessor) {
    if (isValid()) {
        *accessor = mImpl->getAccessor();
        return ResultStatus::OK;
    }
    return ResultStatus::CRITICAL_ERROR;
}

void BufferPoolClient::receiveInvalidation(uint32_t msgId) {
    ALOGV("bufferpool2 client recv inv %u", msgId);
    if (isValid()) {
        mImpl->receiveInvalidation(msgId);
    }
}

BufferPoolStatus BufferPoolClient::flush() {
    if (isValid()) {
        return mImpl->flush();
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus BufferPoolClient::allocate(
        const std::vector<uint8_t> &params,
        native_handle_t **handle,
        std::shared_ptr<BufferPoolData> *buffer) {
    if (isValid()) {
        return mImpl->allocate(params, handle, buffer);
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus BufferPoolClient::receive(
        TransactionId transactionId, BufferId bufferId, int64_t timestampMs,
        native_handle_t **handle, std::shared_ptr<BufferPoolData> *buffer) {
    if (isValid()) {
        return mImpl->receive(transactionId, bufferId, timestampMs, handle, buffer);
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus BufferPoolClient::postSend(
        ConnectionId receiverId,
        const std::shared_ptr<BufferPoolData> &buffer,
        TransactionId *transactionId,
        int64_t *timestampMs) {
    if (isValid()) {
        bool result = mImpl->postSend(
                buffer->mId, receiverId, transactionId, timestampMs);
        return result ? ResultStatus::OK : ResultStatus::CRITICAL_ERROR;
    }
    return ResultStatus::CRITICAL_ERROR;
}

}  // namespace aidl::android::hardware::media::bufferpool2::implementation
