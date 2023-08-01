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
#define LOG_TAG "AidlBufferPoolMgr"
//#define LOG_NDEBUG 0

#include <aidl/android/hardware/media/bufferpool2/ResultStatus.h>
#include <bufferpool2/ClientManager.h>

#include <sys/types.h>
#include <utils/SystemClock.h>
#include <unistd.h>
#include <utils/Log.h>

#include <chrono>

#include "BufferPoolClient.h"
#include "Observer.h"
#include "Accessor.h"

namespace aidl::android::hardware::media::bufferpool2::implementation {

using namespace std::chrono_literals;

using Registration = aidl::android::hardware::media::bufferpool2::IClientManager::Registration;
using aidl::android::hardware::media::bufferpool2::ResultStatus;

static constexpr int64_t kRegisterTimeoutMs = 500; // 0.5 sec
static constexpr int64_t kCleanUpDurationMs = 1000; // TODO: 1 sec tune
static constexpr int64_t kClientTimeoutMs = 5000; // TODO: 5 secs tune

class ClientManager::Impl {
public:
    Impl();

    // BnRegisterSender
    BufferPoolStatus registerSender(const std::shared_ptr<IAccessor> &accessor,
                                Registration *pRegistration);

    // BpRegisterSender
    BufferPoolStatus registerSender(const std::shared_ptr<IClientManager> &receiver,
                                ConnectionId senderId,
                                ConnectionId *receiverId,
                                bool *isNew);

    BufferPoolStatus create(const std::shared_ptr<BufferPoolAllocator> &allocator,
                        ConnectionId *pConnectionId);

    BufferPoolStatus close(ConnectionId connectionId);

    BufferPoolStatus flush(ConnectionId connectionId);

    BufferPoolStatus allocate(ConnectionId connectionId,
                          const std::vector<uint8_t> &params,
                          native_handle_t **handle,
                          std::shared_ptr<BufferPoolData> *buffer);

    BufferPoolStatus receive(ConnectionId connectionId,
                         TransactionId transactionId,
                         BufferId bufferId,
                         int64_t timestampMs,
                         native_handle_t **handle,
                         std::shared_ptr<BufferPoolData> *buffer);

    BufferPoolStatus postSend(ConnectionId receiverId,
                          const std::shared_ptr<BufferPoolData> &buffer,
                          TransactionId *transactionId,
                          int64_t *timestampMs);

    BufferPoolStatus getAccessor(ConnectionId connectionId,
                             std::shared_ptr<IAccessor> *accessor);

    void cleanUp(bool clearCache = false);

private:
    // In order to prevent deadlock between multiple locks,
    // always lock ClientCache.lock before locking ActiveClients.lock.
    struct ClientCache {
        // This lock is held for brief duration.
        // Blocking operation is not performed while holding the lock.
        std::mutex mMutex;
        std::list<std::pair<const std::weak_ptr<IAccessor>, const std::weak_ptr<BufferPoolClient>>>
                mClients;
        std::condition_variable mConnectCv;
        bool mConnecting;
        int64_t mLastCleanUpMs;

        ClientCache() : mConnecting(false), mLastCleanUpMs(::android::elapsedRealtime()) {}
    } mCache;

    // Active clients which can be retrieved via ConnectionId
    struct ActiveClients {
        // This lock is held for brief duration.
        // Blocking operation is not performed holding the lock.
        std::mutex mMutex;
        std::map<ConnectionId, const std::shared_ptr<BufferPoolClient>>
                mClients;
    } mActive;

    std::shared_ptr<Observer> mObserver;
};

ClientManager::Impl::Impl()
    : mObserver(::ndk::SharedRefBase::make<Observer>()) {}

BufferPoolStatus ClientManager::Impl::registerSender(
        const std::shared_ptr<IAccessor> &accessor, Registration *pRegistration) {
    cleanUp();
    int64_t timeoutMs = ::android::elapsedRealtime() + kRegisterTimeoutMs;
    do {
        std::unique_lock<std::mutex> lock(mCache.mMutex);
        for (auto it = mCache.mClients.begin(); it != mCache.mClients.end(); ++it) {
            std::shared_ptr<IAccessor> sAccessor = it->first.lock();
            if (sAccessor && sAccessor.get() == accessor.get()) {
                const std::shared_ptr<BufferPoolClient> client = it->second.lock();
                if (client) {
                    std::lock_guard<std::mutex> lock(mActive.mMutex);
                    pRegistration->connectionId = client->getConnectionId();
                    if (mActive.mClients.find(pRegistration->connectionId)
                            != mActive.mClients.end()) {
                        ALOGV("register existing connection %lld",
                              (long long)pRegistration->connectionId);
                        pRegistration->isNew = false;
                        return ResultStatus::OK;
                    }
                }
                mCache.mClients.erase(it);
                break;
            }
        }
        if (!mCache.mConnecting) {
            mCache.mConnecting = true;
            lock.unlock();
            BufferPoolStatus result = ResultStatus::OK;
            const std::shared_ptr<BufferPoolClient> client =
                    std::make_shared<BufferPoolClient>(accessor, mObserver);
            lock.lock();
            if (!client) {
                result = ResultStatus::NO_MEMORY;
            } else if (!client->isValid()) {
                result = ResultStatus::CRITICAL_ERROR;
            }
            if (result == ResultStatus::OK) {
                // TODO: handle insert fail. (malloc fail)
                const std::weak_ptr<BufferPoolClient> wclient = client;
                mCache.mClients.push_back(std::make_pair(accessor, wclient));
                ConnectionId conId = client->getConnectionId();
                mObserver->addClient(conId, wclient);
                {
                    std::lock_guard<std::mutex> lock(mActive.mMutex);
                    mActive.mClients.insert(std::make_pair(conId, client));
                }
                pRegistration->connectionId = conId;
                pRegistration->isNew = true;
                ALOGV("register new connection %lld", (long long)conId);
            }
            mCache.mConnecting = false;
            lock.unlock();
            mCache.mConnectCv.notify_all();
            return result;
        }
        mCache.mConnectCv.wait_for(lock, kRegisterTimeoutMs*1ms);
    } while (::android::elapsedRealtime() < timeoutMs);
    // TODO: return timeout error
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus ClientManager::Impl::registerSender(
        const std::shared_ptr<IClientManager> &receiver,
        ConnectionId senderId,
        ConnectionId *receiverId,
        bool *isNew) {
    std::shared_ptr<IAccessor> accessor;
    bool local = false;
    {
        std::lock_guard<std::mutex> lock(mActive.mMutex);
        auto it = mActive.mClients.find(senderId);
        if (it == mActive.mClients.end()) {
            return ResultStatus::NOT_FOUND;
        }
        it->second->getAccessor(&accessor);
        local = it->second->isLocal();
    }
    if (accessor) {
        Registration registration;
        ::ndk::ScopedAStatus status = receiver->registerSender(accessor, &registration);
        if (!status.isOk()) {
            return ResultStatus::CRITICAL_ERROR;
        } else if (local) {
            std::shared_ptr<ConnectionDeathRecipient> recipient =
                    Accessor::getConnectionDeathRecipient();
            if (recipient)  {
                ALOGV("client death recipient registered %lld", (long long)*receiverId);
                recipient->addCookieToConnection(receiver->asBinder().get(), *receiverId);
                AIBinder_linkToDeath(receiver->asBinder().get(), recipient->getRecipient(),
                                     receiver->asBinder().get());
            }
        }
        *receiverId = registration.connectionId;
        *isNew = registration.isNew;
        return ResultStatus::OK;
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus ClientManager::Impl::create(
        const std::shared_ptr<BufferPoolAllocator> &allocator,
        ConnectionId *pConnectionId) {
    std::shared_ptr<Accessor> accessor = ::ndk::SharedRefBase::make<Accessor>(allocator);
    if (!accessor || !accessor->isValid()) {
        return ResultStatus::CRITICAL_ERROR;
    }
    // TODO: observer is local. use direct call instead of hidl call.
    std::shared_ptr<BufferPoolClient> client =
            std::make_shared<BufferPoolClient>(accessor, mObserver);
    if (!client || !client->isValid()) {
        return ResultStatus::CRITICAL_ERROR;
    }
    // Since a new bufferpool is created, evict memories which are used by
    // existing bufferpools and clients.
    cleanUp(true);
    {
        // TODO: handle insert fail. (malloc fail)
        std::lock_guard<std::mutex> lock(mCache.mMutex);
        const std::weak_ptr<BufferPoolClient> wclient = client;
        mCache.mClients.push_back(std::make_pair(accessor, wclient));
        ConnectionId conId = client->getConnectionId();
        mObserver->addClient(conId, wclient);
        {
            std::lock_guard<std::mutex> lock(mActive.mMutex);
            mActive.mClients.insert(std::make_pair(conId, client));
        }
        *pConnectionId = conId;
        ALOGV("create new connection %lld", (long long)*pConnectionId);
    }
    return ResultStatus::OK;
}

BufferPoolStatus ClientManager::Impl::close(ConnectionId connectionId) {
    std::unique_lock<std::mutex> lock1(mCache.mMutex);
    std::unique_lock<std::mutex> lock2(mActive.mMutex);
    auto it = mActive.mClients.find(connectionId);
    if (it != mActive.mClients.end()) {
        std::shared_ptr<IAccessor> accessor;
        it->second->getAccessor(&accessor);
        std::shared_ptr<BufferPoolClient> closing = it->second;
        mActive.mClients.erase(connectionId);
        for (auto cit = mCache.mClients.begin(); cit != mCache.mClients.end();) {
            // clean up dead client caches
            std::shared_ptr<IAccessor> cAccessor = cit->first.lock();
            if (!cAccessor || (accessor && cAccessor.get() ==  accessor.get())) {
                cit = mCache.mClients.erase(cit);
            } else {
                cit++;
            }
        }
        lock2.unlock();
        lock1.unlock();
        closing->flush();
        return ResultStatus::OK;
    }
    return ResultStatus::NOT_FOUND;
}

BufferPoolStatus ClientManager::Impl::flush(ConnectionId connectionId) {
    std::shared_ptr<BufferPoolClient> client;
    {
        std::lock_guard<std::mutex> lock(mActive.mMutex);
        auto it = mActive.mClients.find(connectionId);
        if (it == mActive.mClients.end()) {
            return ResultStatus::NOT_FOUND;
        }
        client = it->second;
    }
    return client->flush();
}

BufferPoolStatus ClientManager::Impl::allocate(
        ConnectionId connectionId, const std::vector<uint8_t> &params,
        native_handle_t **handle, std::shared_ptr<BufferPoolData> *buffer) {
    std::shared_ptr<BufferPoolClient> client;
    {
        std::lock_guard<std::mutex> lock(mActive.mMutex);
        auto it = mActive.mClients.find(connectionId);
        if (it == mActive.mClients.end()) {
            return ResultStatus::NOT_FOUND;
        }
        client = it->second;
    }
#ifdef BUFFERPOOL_CLONE_HANDLES
    native_handle_t *origHandle;
    BufferPoolStatus res = client->allocate(params, &origHandle, buffer);
    if (res != ResultStatus::OK) {
        return res;
    }
    *handle = native_handle_clone(origHandle);
    if (handle == NULL) {
        buffer->reset();
        return ResultStatus::NO_MEMORY;
    }
    return ResultStatus::OK;
#else
    return client->allocate(params, handle, buffer);
#endif
}

BufferPoolStatus ClientManager::Impl::receive(
        ConnectionId connectionId, TransactionId transactionId,
        BufferId bufferId, int64_t timestampMs,
        native_handle_t **handle, std::shared_ptr<BufferPoolData> *buffer) {
    std::shared_ptr<BufferPoolClient> client;
    {
        std::lock_guard<std::mutex> lock(mActive.mMutex);
        auto it = mActive.mClients.find(connectionId);
        if (it == mActive.mClients.end()) {
            return ResultStatus::NOT_FOUND;
        }
        client = it->second;
    }
#ifdef BUFFERPOOL_CLONE_HANDLES
    native_handle_t *origHandle;
    BufferPoolStatus res = client->receive(
            transactionId, bufferId, timestampMs, &origHandle, buffer);
    if (res != ResultStatus::OK) {
        return res;
    }
    *handle = native_handle_clone(origHandle);
    if (handle == NULL) {
        buffer->reset();
        return ResultStatus::NO_MEMORY;
    }
    return ResultStatus::OK;
#else
    return client->receive(transactionId, bufferId, timestampMs, handle, buffer);
#endif
}

BufferPoolStatus ClientManager::Impl::postSend(
        ConnectionId receiverId, const std::shared_ptr<BufferPoolData> &buffer,
        TransactionId *transactionId, int64_t *timestampMs) {
    ConnectionId connectionId = buffer->mConnectionId;
    std::shared_ptr<BufferPoolClient> client;
    {
        std::lock_guard<std::mutex> lock(mActive.mMutex);
        auto it = mActive.mClients.find(connectionId);
        if (it == mActive.mClients.end()) {
            return ResultStatus::NOT_FOUND;
        }
        client = it->second;
    }
    return client->postSend(receiverId, buffer, transactionId, timestampMs);
}

BufferPoolStatus ClientManager::Impl::getAccessor(
        ConnectionId connectionId, std::shared_ptr<IAccessor> *accessor) {
    std::shared_ptr<BufferPoolClient> client;
    {
        std::lock_guard<std::mutex> lock(mActive.mMutex);
        auto it = mActive.mClients.find(connectionId);
        if (it == mActive.mClients.end()) {
            return ResultStatus::NOT_FOUND;
        }
        client = it->second;
    }
    return client->getAccessor(accessor);
}

void ClientManager::Impl::cleanUp(bool clearCache) {
    int64_t now = ::android::elapsedRealtime();
    int64_t lastTransactionMs;
    std::lock_guard<std::mutex> lock1(mCache.mMutex);
    if (clearCache || mCache.mLastCleanUpMs + kCleanUpDurationMs < now) {
        std::lock_guard<std::mutex> lock2(mActive.mMutex);
        int cleaned = 0;
        for (auto it = mActive.mClients.begin(); it != mActive.mClients.end();) {
            if (!it->second->isActive(&lastTransactionMs, clearCache)) {
                if (lastTransactionMs + kClientTimeoutMs < now) {
                  std::shared_ptr<IAccessor> accessor;
                    it->second->getAccessor(&accessor);
                    it = mActive.mClients.erase(it);
                    ++cleaned;
                    continue;
                }
            }
            ++it;
        }
        for (auto cit = mCache.mClients.begin(); cit != mCache.mClients.end();) {
            // clean up dead client caches
          std::shared_ptr<IAccessor> cAccessor = cit->first.lock();
            if (!cAccessor) {
                cit = mCache.mClients.erase(cit);
            } else {
                ++cit;
            }
        }
        ALOGV("# of cleaned connections: %d", cleaned);
        mCache.mLastCleanUpMs = now;
    }
}

::ndk::ScopedAStatus ClientManager::registerSender(
        const std::shared_ptr<IAccessor>& in_bufferPool, Registration* _aidl_return) {
    BufferPoolStatus status = ResultStatus::CRITICAL_ERROR;
    if (mImpl) {
        status = mImpl->registerSender(in_bufferPool, _aidl_return);
    }
    if (status != ResultStatus::OK) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(status);
    }
    return ::ndk::ScopedAStatus::ok();
}

// Methods for local use.
std::shared_ptr<ClientManager> ClientManager::sInstance;
std::mutex ClientManager::sInstanceLock;

std::shared_ptr<ClientManager> ClientManager::getInstance() {
    std::lock_guard<std::mutex> lock(sInstanceLock);
    if (!sInstance) {
        sInstance = ::ndk::SharedRefBase::make<ClientManager>();
        // TODO: configure thread count for threadpool properly
        // after b/261652496 is resolved.
    }
    Accessor::createInvalidator();
    Accessor::createEvictor();
    return sInstance;
}

ClientManager::ClientManager() : mImpl(new Impl()) {}

ClientManager::~ClientManager() {
}

BufferPoolStatus ClientManager::create(
        const std::shared_ptr<BufferPoolAllocator> &allocator,
        ConnectionId *pConnectionId) {
    if (mImpl) {
        return mImpl->create(allocator, pConnectionId);
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus ClientManager::registerSender(
        const std::shared_ptr<IClientManager> &receiver,
        ConnectionId senderId,
        ConnectionId *receiverId,
        bool *isNew) {
    if (mImpl) {
        return mImpl->registerSender(receiver, senderId, receiverId, isNew);
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus ClientManager::close(ConnectionId connectionId) {
    if (mImpl) {
        return mImpl->close(connectionId);
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus ClientManager::flush(ConnectionId connectionId) {
    if (mImpl) {
        return mImpl->flush(connectionId);
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus ClientManager::allocate(
        ConnectionId connectionId, const std::vector<uint8_t> &params,
        native_handle_t **handle, std::shared_ptr<BufferPoolData> *buffer) {
    if (mImpl) {
        return mImpl->allocate(connectionId, params, handle, buffer);
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus ClientManager::receive(
        ConnectionId connectionId, TransactionId transactionId,
        BufferId bufferId, int64_t timestampMs,
        native_handle_t **handle, std::shared_ptr<BufferPoolData> *buffer) {
    if (mImpl) {
        return mImpl->receive(connectionId, transactionId, bufferId,
                              timestampMs, handle, buffer);
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus ClientManager::postSend(
        ConnectionId receiverId, const std::shared_ptr<BufferPoolData> &buffer,
        TransactionId *transactionId, int64_t* timestampMs) {
    if (mImpl && buffer) {
        return mImpl->postSend(receiverId, buffer, transactionId, timestampMs);
    }
    return ResultStatus::CRITICAL_ERROR;
}

void ClientManager::cleanUp() {
    if (mImpl) {
        mImpl->cleanUp(true);
    }
}

}  // namespace ::aidl::android::hardware::media::bufferpool2::implementation
