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
#define LOG_TAG "AidlBufferPoolAcc"
//#define LOG_NDEBUG 0

#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <utils/Log.h>
#include <thread>

#include "Accessor.h"
#include "Connection.h"
#include "DataHelper.h"

namespace aidl::android::hardware::media::bufferpool2::implementation {

namespace {
    static constexpr nsecs_t kEvictGranularityNs = 1000000000; // 1 sec
    static constexpr nsecs_t kEvictDurationNs = 5000000000; // 5 secs
}

#ifdef __ANDROID_VNDK__
static constexpr uint32_t kSeqIdVndkBit = 1U << 31;
#else
static constexpr uint32_t kSeqIdVndkBit = 0;
#endif

static constexpr uint32_t kSeqIdMax = 0x7fffffff;
uint32_t Accessor::sSeqId = time(nullptr) & kSeqIdMax;

namespace {
// anonymous namespace
static std::shared_ptr<ConnectionDeathRecipient> sConnectionDeathRecipient =
    std::make_shared<ConnectionDeathRecipient>();

void serviceDied(void *cookie) {
    if (sConnectionDeathRecipient) {
        sConnectionDeathRecipient->onDead(cookie);
    }
}
}

std::shared_ptr<ConnectionDeathRecipient> Accessor::getConnectionDeathRecipient() {
    return sConnectionDeathRecipient;
}

ConnectionDeathRecipient::ConnectionDeathRecipient() {
    mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(
            AIBinder_DeathRecipient_new(serviceDied));
}

void ConnectionDeathRecipient::add(
        int64_t connectionId,
        const std::shared_ptr<Accessor> &accessor) {
    std::lock_guard<std::mutex> lock(mLock);
    if (mAccessors.find(connectionId) == mAccessors.end()) {
        mAccessors.insert(std::make_pair(connectionId, accessor));
    }
}

void ConnectionDeathRecipient::remove(int64_t connectionId) {
    std::lock_guard<std::mutex> lock(mLock);
    mAccessors.erase(connectionId);
    auto it = mConnectionToCookie.find(connectionId);
    if (it != mConnectionToCookie.end()) {
        void * cookie = it->second;
        mConnectionToCookie.erase(it);
        auto cit = mCookieToConnections.find(cookie);
        if (cit != mCookieToConnections.end()) {
            cit->second.erase(connectionId);
            if (cit->second.size() == 0) {
                mCookieToConnections.erase(cit);
            }
        }
    }
}

void ConnectionDeathRecipient::addCookieToConnection(
        void *cookie,
        int64_t connectionId) {
    std::lock_guard<std::mutex> lock(mLock);
    if (mAccessors.find(connectionId) == mAccessors.end()) {
        return;
    }
    mConnectionToCookie.insert(std::make_pair(connectionId, cookie));
    auto it = mCookieToConnections.find(cookie);
    if (it != mCookieToConnections.end()) {
        it->second.insert(connectionId);
    } else {
        mCookieToConnections.insert(std::make_pair(
                cookie, std::set<int64_t>{connectionId}));
    }
}

void ConnectionDeathRecipient::onDead(void *cookie) {
    std::map<int64_t, const std::weak_ptr<Accessor>> connectionsToClose;
    {
        std::lock_guard<std::mutex> lock(mLock);

        auto it = mCookieToConnections.find(cookie);
        if (it != mCookieToConnections.end()) {
            for (auto conIt = it->second.begin(); conIt != it->second.end(); ++conIt) {
                auto accessorIt = mAccessors.find(*conIt);
                if (accessorIt != mAccessors.end()) {
                    connectionsToClose.insert(std::make_pair(*conIt, accessorIt->second));
                    mAccessors.erase(accessorIt);
                }
                mConnectionToCookie.erase(*conIt);
            }
            mCookieToConnections.erase(it);
        }
    }

    if (connectionsToClose.size() > 0) {
        std::shared_ptr<Accessor> accessor;
        for (auto it = connectionsToClose.begin(); it != connectionsToClose.end(); ++it) {
            accessor = it->second.lock();

            if (accessor) {
                accessor->close(it->first);
                ALOGD("connection %lld closed on death", (long long)it->first);
            }
        }
    }
}

AIBinder_DeathRecipient *ConnectionDeathRecipient::getRecipient() {
    return mDeathRecipient.get();
}

::ndk::ScopedAStatus Accessor::connect(const std::shared_ptr<::aidl::android::hardware::media::bufferpool2::IObserver>& in_observer, ::aidl::android::hardware::media::bufferpool2::IAccessor::ConnectionInfo* _aidl_return) {
    std::shared_ptr<Connection> connection;
    ConnectionId connectionId;
    uint32_t msgId;
    StatusDescriptor statusDesc;
    InvalidationDescriptor invDesc;
    BufferPoolStatus status = connect(
            in_observer, false, &connection, &connectionId, &msgId, &statusDesc, &invDesc);
    if (status == ResultStatus::OK) {
        _aidl_return->connection = connection;
        _aidl_return->connectionId = connectionId;
        _aidl_return->msgId = msgId;
        _aidl_return->toFmqDesc = std::move(statusDesc);
        _aidl_return->fromFmqDesc = std::move(invDesc);
        return ::ndk::ScopedAStatus::ok();
    }
    return ::ndk::ScopedAStatus::fromServiceSpecificError(status);
}

Accessor::Accessor(const std::shared_ptr<BufferPoolAllocator> &allocator)
    : mAllocator(allocator), mScheduleEvictTs(0) {}

Accessor::~Accessor() {
}

bool Accessor::isValid() {
    return mBufferPool.isValid();
}

BufferPoolStatus Accessor::flush() {
    std::lock_guard<std::mutex> lock(mBufferPool.mMutex);
    mBufferPool.processStatusMessages();
    mBufferPool.flush(ref<Accessor>());
    return ResultStatus::OK;
}

BufferPoolStatus Accessor::allocate(
        ConnectionId connectionId,
        const std::vector<uint8_t> &params,
        BufferId *bufferId, const native_handle_t** handle) {
    std::unique_lock<std::mutex> lock(mBufferPool.mMutex);
    mBufferPool.processStatusMessages();
    BufferPoolStatus status = ResultStatus::OK;
    if (!mBufferPool.getFreeBuffer(mAllocator, params, bufferId, handle)) {
        lock.unlock();
        std::shared_ptr<BufferPoolAllocation> alloc;
        size_t allocSize;
        status = mAllocator->allocate(params, &alloc, &allocSize);
        lock.lock();
        if (status == ResultStatus::OK) {
            status = mBufferPool.addNewBuffer(alloc, allocSize, params, bufferId, handle);
        }
        ALOGV("create a buffer %d : %u %p",
              status == ResultStatus::OK, *bufferId, *handle);
    }
    if (status == ResultStatus::OK) {
        // TODO: handle ownBuffer failure
        mBufferPool.handleOwnBuffer(connectionId, *bufferId);
    }
    mBufferPool.cleanUp();
    scheduleEvictIfNeeded();
    return status;
}

BufferPoolStatus Accessor::fetch(
        ConnectionId connectionId, TransactionId transactionId,
        BufferId bufferId, const native_handle_t** handle) {
    std::lock_guard<std::mutex> lock(mBufferPool.mMutex);
    mBufferPool.processStatusMessages();
    auto found = mBufferPool.mTransactions.find(transactionId);
    if (found != mBufferPool.mTransactions.end() &&
            contains(&mBufferPool.mPendingTransactions,
                     connectionId, transactionId)) {
        if (found->second->mSenderValidated &&
                found->second->mStatus == BufferStatus::TRANSFER_FROM &&
                found->second->mBufferId == bufferId) {
            found->second->mStatus = BufferStatus::TRANSFER_FETCH;
            auto bufferIt = mBufferPool.mBuffers.find(bufferId);
            if (bufferIt != mBufferPool.mBuffers.end()) {
                mBufferPool.mStats.onBufferFetched();
                *handle = bufferIt->second->handle();
                return ResultStatus::OK;
            }
        }
    }
    mBufferPool.cleanUp();
    scheduleEvictIfNeeded();
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus Accessor::connect(
        const std::shared_ptr<IObserver> &observer, bool local,
        std::shared_ptr<Connection> *connection, ConnectionId *pConnectionId,
        uint32_t *pMsgId,
        StatusDescriptor* statusDescPtr,
        InvalidationDescriptor* invDescPtr) {
    std::shared_ptr<Connection> newConnection = ::ndk::SharedRefBase::make<Connection>();
    BufferPoolStatus status = ResultStatus::CRITICAL_ERROR;
    {
        std::lock_guard<std::mutex> lock(mBufferPool.mMutex);
        if (newConnection) {
            int32_t pid = getpid();
            ConnectionId id = (int64_t)pid << 32 | sSeqId | kSeqIdVndkBit;
            status = mBufferPool.mObserver.open(id, statusDescPtr);
            if (status == ResultStatus::OK) {
                newConnection->initialize(ref<Accessor>(), id);
                *connection = newConnection;
                *pConnectionId = id;
                *pMsgId = mBufferPool.mInvalidation.mInvalidationId;
                mBufferPool.mConnectionIds.insert(id);
                mBufferPool.mInvalidationChannel.getDesc(invDescPtr);
                mBufferPool.mInvalidation.onConnect(id, observer);
                if (sSeqId == kSeqIdMax) {
                   sSeqId = 0;
                } else {
                    ++sSeqId;
                }
            }

        }
        mBufferPool.processStatusMessages();
        mBufferPool.cleanUp();
        scheduleEvictIfNeeded();
    }
    if (!local && status == ResultStatus::OK) {
        std::shared_ptr<Accessor> accessor(ref<Accessor>());
        sConnectionDeathRecipient->add(*pConnectionId, accessor);
    }
    return status;
}

BufferPoolStatus Accessor::close(ConnectionId connectionId) {
    {
        std::lock_guard<std::mutex> lock(mBufferPool.mMutex);
        ALOGV("connection close %lld: %u", (long long)connectionId, mBufferPool.mInvalidation.mId);
        mBufferPool.processStatusMessages();
        mBufferPool.handleClose(connectionId);
        mBufferPool.mObserver.close(connectionId);
        mBufferPool.mInvalidation.onClose(connectionId);
        // Since close# will be called after all works are finished, it is OK to
        // evict unused buffers.
        mBufferPool.cleanUp(true);
        scheduleEvictIfNeeded();
    }
    sConnectionDeathRecipient->remove(connectionId);
    return ResultStatus::OK;
}

void Accessor::cleanUp(bool clearCache) {
    // transaction timeout, buffer caching TTL handling
    std::lock_guard<std::mutex> lock(mBufferPool.mMutex);
    mBufferPool.processStatusMessages();
    mBufferPool.cleanUp(clearCache);
}

void Accessor::handleInvalidateAck() {
    std::map<ConnectionId, const std::shared_ptr<IObserver>> observers;
    uint32_t invalidationId;
    {
        std::lock_guard<std::mutex> lock(mBufferPool.mMutex);
        mBufferPool.processStatusMessages();
        mBufferPool.mInvalidation.onHandleAck(&observers, &invalidationId);
    }
    // Do not hold lock for send invalidations
    size_t deadClients = 0;
    for (auto it = observers.begin(); it != observers.end(); ++it) {
        const std::shared_ptr<IObserver> observer = it->second;
        if (observer) {
            ::ndk::ScopedAStatus status = observer->onMessage(it->first, invalidationId);
            if (!status.isOk()) {
                ++deadClients;
            }
        }
    }
    if (deadClients > 0) {
        ALOGD("During invalidation found %zu dead clients", deadClients);
    }
}

void Accessor::invalidatorThread(
            std::map<uint32_t, const std::weak_ptr<Accessor>> &accessors,
            std::mutex &mutex,
            std::condition_variable &cv,
            bool &ready) {
    constexpr uint32_t NUM_SPIN_TO_INCREASE_SLEEP = 1024;
    constexpr uint32_t NUM_SPIN_TO_LOG = 1024*8;
    constexpr useconds_t MAX_SLEEP_US = 10000;
    uint32_t numSpin = 0;
    useconds_t sleepUs = 1;

    while(true) {
        std::map<uint32_t, const std::weak_ptr<Accessor>> copied;
        {
            std::unique_lock<std::mutex> lock(mutex);
            while (!ready) {
                numSpin = 0;
                sleepUs = 1;
                cv.wait(lock);
            }
            copied.insert(accessors.begin(), accessors.end());
        }
        std::list<ConnectionId> erased;
        for (auto it = copied.begin(); it != copied.end(); ++it) {
            const std::shared_ptr<Accessor> acc = it->second.lock();
            if (!acc) {
                erased.push_back(it->first);
            } else {
                acc->handleInvalidateAck();
            }
        }
        {
            std::unique_lock<std::mutex> lock(mutex);
            for (auto it = erased.begin(); it != erased.end(); ++it) {
                accessors.erase(*it);
            }
            if (accessors.size() == 0) {
                ready = false;
            } else {
                // N.B. Since there is not a efficient way to wait over FMQ,
                // polling over the FMQ is the current way to prevent draining
                // CPU.
                lock.unlock();
                ++numSpin;
                if (numSpin % NUM_SPIN_TO_INCREASE_SLEEP == 0 &&
                    sleepUs < MAX_SLEEP_US) {
                    sleepUs *= 10;
                }
                if (numSpin % NUM_SPIN_TO_LOG == 0) {
                    ALOGW("invalidator thread spinning");
                }
                ::usleep(sleepUs);
            }
        }
    }
}

Accessor::AccessorInvalidator::AccessorInvalidator() : mReady(false) {
    std::thread invalidator(
            invalidatorThread,
            std::ref(mAccessors),
            std::ref(mMutex),
            std::ref(mCv),
            std::ref(mReady));
    invalidator.detach();
}

void Accessor::AccessorInvalidator::addAccessor(
        uint32_t accessorId, const std::weak_ptr<Accessor> &accessor) {
    bool notify = false;
    std::unique_lock<std::mutex> lock(mMutex);
    if (mAccessors.find(accessorId) == mAccessors.end()) {
        if (!mReady) {
            mReady = true;
            notify = true;
        }
        mAccessors.emplace(accessorId, accessor);
        ALOGV("buffer invalidation added bp:%u %d", accessorId, notify);
    }
    lock.unlock();
    if (notify) {
        mCv.notify_one();
    }
}

void Accessor::AccessorInvalidator::delAccessor(uint32_t accessorId) {
    std::lock_guard<std::mutex> lock(mMutex);
    mAccessors.erase(accessorId);
    ALOGV("buffer invalidation deleted bp:%u", accessorId);
    if (mAccessors.size() == 0) {
        mReady = false;
    }
}

std::unique_ptr<Accessor::AccessorInvalidator> Accessor::sInvalidator;

void Accessor::createInvalidator() {
    if (!sInvalidator) {
        sInvalidator = std::make_unique<Accessor::AccessorInvalidator>();
    }
}

void Accessor::evictorThread(
        std::map<const std::weak_ptr<Accessor>, nsecs_t, std::owner_less<>> &accessors,
        std::mutex &mutex,
        std::condition_variable &cv) {
    std::list<const std::weak_ptr<Accessor>> evictList;
    while (true) {
        int expired = 0;
        int evicted = 0;
        {
            nsecs_t now = systemTime();
            std::unique_lock<std::mutex> lock(mutex);
            while (accessors.size() == 0) {
                cv.wait(lock);
            }
            auto it = accessors.begin();
            while (it != accessors.end()) {
                if (now > (it->second + kEvictDurationNs)) {
                    ++expired;
                    evictList.push_back(it->first);
                    it = accessors.erase(it);
                } else {
                    ++it;
                }
            }
        }
        // evict idle accessors;
        for (auto it = evictList.begin(); it != evictList.end(); ++it) {
            const std::shared_ptr<Accessor> accessor = it->lock();
            if (accessor) {
                accessor->cleanUp(true);
                ++evicted;
            }
        }
        if (expired > 0) {
            ALOGD("evictor expired: %d, evicted: %d", expired, evicted);
        }
        evictList.clear();
        ::usleep(kEvictGranularityNs / 1000);
    }
}

Accessor::AccessorEvictor::AccessorEvictor() {
    std::thread evictor(
            evictorThread,
            std::ref(mAccessors),
            std::ref(mMutex),
            std::ref(mCv));
    evictor.detach();
}

void Accessor::AccessorEvictor::addAccessor(
        const std::weak_ptr<Accessor> &accessor, nsecs_t ts) {
    std::lock_guard<std::mutex> lock(mMutex);
    bool notify = mAccessors.empty();
    auto it = mAccessors.find(accessor);
    if (it == mAccessors.end()) {
        mAccessors.emplace(accessor, ts);
    } else {
        it->second = ts;
    }
    if (notify) {
        mCv.notify_one();
    }
}

std::unique_ptr<Accessor::AccessorEvictor> Accessor::sEvictor;

void Accessor::createEvictor() {
    if (!sEvictor) {
        sEvictor = std::make_unique<Accessor::AccessorEvictor>();
    }
}

void Accessor::scheduleEvictIfNeeded() {
    nsecs_t now = systemTime();

    if (now > (mScheduleEvictTs + kEvictGranularityNs)) {
        mScheduleEvictTs = now;
        sEvictor->addAccessor(ref<Accessor>(), now);
    }
}

}  // namespace aidl::android::hardware::media::bufferpool2::implemntation {
