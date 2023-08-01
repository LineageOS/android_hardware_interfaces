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

#define LOG_TAG "AidlBufferPool"
//#define LOG_NDEBUG 0

#include <sys/types.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <utils/Log.h>
#include <thread>
#include "Accessor.h"
#include "BufferPool.h"
#include "Connection.h"
#include "DataHelper.h"

namespace aidl::android::hardware::media::bufferpool2::implementation {

namespace {
    static constexpr int64_t kCleanUpDurationMs = 500; // 0.5 sec
    static constexpr int64_t kLogDurationMs = 5000; // 5 secs

    static constexpr size_t kMinAllocBytesForEviction = 1024*1024*15;
    static constexpr size_t kMinBufferCountForEviction = 25;
    static constexpr size_t kMaxUnusedBufferCount = 64;
    static constexpr size_t kUnusedBufferCountTarget = kMaxUnusedBufferCount - 16;
}

BufferPool::BufferPool()
    : mTimestampMs(::android::elapsedRealtime()),
      mLastCleanUpMs(mTimestampMs),
      mLastLogMs(mTimestampMs),
      mSeq(0),
      mStartSeq(0) {
    mValid = mInvalidationChannel.isValid();
}


// Statistics helper
template<typename T, typename S>
int percentage(T base, S total) {
    return int(total ? 0.5 + 100. * static_cast<S>(base) / total : 0);
}

std::atomic<std::uint32_t> BufferPool::Invalidation::sInvSeqId(0);

BufferPool::~BufferPool() {
    std::lock_guard<std::mutex> lock(mMutex);
    ALOGD("Destruction - bufferpool2 %p "
          "cached: %zu/%zuM, %zu/%d%% in use; "
          "allocs: %zu, %d%% recycled; "
          "transfers: %zu, %d%% unfetched",
          this, mStats.mBuffersCached, mStats.mSizeCached >> 20,
          mStats.mBuffersInUse, percentage(mStats.mBuffersInUse, mStats.mBuffersCached),
          mStats.mTotalAllocations, percentage(mStats.mTotalRecycles, mStats.mTotalAllocations),
          mStats.mTotalTransfers,
          percentage(mStats.mTotalTransfers - mStats.mTotalFetches, mStats.mTotalTransfers));
}

void BufferPool::Invalidation::onConnect(
        ConnectionId conId, const std::shared_ptr<IObserver>& observer) {
    mAcks[conId] = mInvalidationId; // starts from current invalidationId
    mObservers.insert(std::make_pair(conId, observer));
}

void BufferPool::Invalidation::onClose(ConnectionId conId) {
    mAcks.erase(conId);
    mObservers.erase(conId);
}

void BufferPool::Invalidation::onAck(
        ConnectionId conId,
        uint32_t msgId) {
    auto it = mAcks.find(conId);
    if (it == mAcks.end()) {
        ALOGW("ACK from inconsistent connection! %lld", (long long)conId);
        return;
    }
    if (isMessageLater(msgId, it->second)) {
        mAcks[conId] = msgId;
    }
}

void BufferPool::Invalidation::onBufferInvalidated(
        BufferId bufferId,
        BufferInvalidationChannel &channel) {
    for (auto it = mPendings.begin(); it != mPendings.end();) {
        if (it->isInvalidated(bufferId)) {
            uint32_t msgId = 0;
            if (it->mNeedsAck) {
                msgId = ++mInvalidationId;
                if (msgId == 0) {
                    // wrap happens
                    msgId = ++mInvalidationId;
                }
            }
            channel.postInvalidation(msgId, it->mFrom, it->mTo);
            it = mPendings.erase(it);
            continue;
        }
        ++it;
    }
}

void BufferPool::Invalidation::onInvalidationRequest(
        bool needsAck,
        uint32_t from,
        uint32_t to,
        size_t left,
        BufferInvalidationChannel &channel,
        const std::shared_ptr<Accessor> &impl) {
        uint32_t msgId = 0;
    if (needsAck) {
        msgId = ++mInvalidationId;
        if (msgId == 0) {
            // wrap happens
            msgId = ++mInvalidationId;
        }
    }
    ALOGV("bufferpool2 invalidation requested and queued");
    if (left == 0) {
        channel.postInvalidation(msgId, from, to);
    } else {
        ALOGV("bufferpoo2 invalidation requested and pending");
        Pending pending(needsAck, from, to, left, impl);
        mPendings.push_back(pending);
    }
    Accessor::sInvalidator->addAccessor(mId, impl);
}

void BufferPool::Invalidation::onHandleAck(
        std::map<ConnectionId, const std::shared_ptr<IObserver>> *observers,
        uint32_t *invalidationId) {
    if (mInvalidationId != 0) {
        *invalidationId = mInvalidationId;
        std::set<int> deads;
        for (auto it = mAcks.begin(); it != mAcks.end(); ++it) {
            if (it->second != mInvalidationId) {
                const std::shared_ptr<IObserver> observer = mObservers[it->first];
                if (observer) {
                    observers->emplace(it->first, observer);
                    ALOGV("connection %lld will call observer (%u: %u)",
                          (long long)it->first, it->second, mInvalidationId);
                    // N.B: onMessage will be called later. ignore possibility of
                    // onMessage# oneway call being lost.
                    it->second = mInvalidationId;
                } else {
                    ALOGV("bufferpool2 observer died %lld", (long long)it->first);
                    deads.insert(it->first);
                }
            }
        }
        if (deads.size() > 0) {
            for (auto it = deads.begin(); it != deads.end(); ++it) {
                onClose(*it);
            }
        }
    }
    if (mPendings.size() == 0) {
        // All invalidation Ids are synced and no more pending invalidations.
        Accessor::sInvalidator->delAccessor(mId);
    }
}

bool BufferPool::handleOwnBuffer(
        ConnectionId connectionId, BufferId bufferId) {

    bool added = insert(&mUsingBuffers, connectionId, bufferId);
    if (added) {
        auto iter = mBuffers.find(bufferId);
        iter->second->mOwnerCount++;
    }
    insert(&mUsingConnections, bufferId, connectionId);
    return added;
}

bool BufferPool::handleReleaseBuffer(
        ConnectionId connectionId, BufferId bufferId) {
    bool deleted = erase(&mUsingBuffers, connectionId, bufferId);
    if (deleted) {
        auto iter = mBuffers.find(bufferId);
        iter->second->mOwnerCount--;
        if (iter->second->mOwnerCount == 0 &&
                iter->second->mTransactionCount == 0) {
            if (!iter->second->mInvalidated) {
                mStats.onBufferUnused(iter->second->mAllocSize);
                mFreeBuffers.insert(bufferId);
            } else {
                mStats.onBufferUnused(iter->second->mAllocSize);
                mStats.onBufferEvicted(iter->second->mAllocSize);
                mBuffers.erase(iter);
                mInvalidation.onBufferInvalidated(bufferId, mInvalidationChannel);
            }
        }
    }
    erase(&mUsingConnections, bufferId, connectionId);
    ALOGV("release buffer %u : %d", bufferId, deleted);
    return deleted;
}

bool BufferPool::handleTransferTo(const BufferStatusMessage &message) {
    auto completed = mCompletedTransactions.find(
            message.transactionId);
    if (completed != mCompletedTransactions.end()) {
        // already completed
        mCompletedTransactions.erase(completed);
        return true;
    }
    // the buffer should exist and be owned.
    auto bufferIter = mBuffers.find(message.bufferId);
    if (bufferIter == mBuffers.end() ||
            !contains(&mUsingBuffers, message.connectionId, FromAidl(message.bufferId))) {
        return false;
    }
    auto found = mTransactions.find(message.transactionId);
    if (found != mTransactions.end()) {
        // transfer_from was received earlier.
        found->second->mSender = message.connectionId;
        found->second->mSenderValidated = true;
        return true;
    }
    if (mConnectionIds.find(message.targetConnectionId) == mConnectionIds.end()) {
        // N.B: it could be fake or receive connection already closed.
        ALOGD("bufferpool2 %p receiver connection %lld is no longer valid",
              this, (long long)message.targetConnectionId);
        return false;
    }
    mStats.onBufferSent();
    mTransactions.insert(std::make_pair(
            message.transactionId,
            std::make_unique<TransactionStatus>(message, mTimestampMs)));
    insert(&mPendingTransactions, message.targetConnectionId,
           FromAidl(message.transactionId));
    bufferIter->second->mTransactionCount++;
    return true;
}

bool BufferPool::handleTransferFrom(const BufferStatusMessage &message) {
    auto found = mTransactions.find(message.transactionId);
    if (found == mTransactions.end()) {
        // TODO: is it feasible to check ownership here?
        mStats.onBufferSent();
        mTransactions.insert(std::make_pair(
                message.transactionId,
                std::make_unique<TransactionStatus>(message, mTimestampMs)));
        insert(&mPendingTransactions, message.connectionId,
               FromAidl(message.transactionId));
        auto bufferIter = mBuffers.find(message.bufferId);
        bufferIter->second->mTransactionCount++;
    } else {
        if (message.connectionId == found->second->mReceiver) {
            found->second->mStatus = BufferStatus::TRANSFER_FROM;
        }
    }
    return true;
}

bool BufferPool::handleTransferResult(const BufferStatusMessage &message) {
    auto found = mTransactions.find(message.transactionId);
    if (found != mTransactions.end()) {
        bool deleted = erase(&mPendingTransactions, message.connectionId,
                             FromAidl(message.transactionId));
        if (deleted) {
            if (!found->second->mSenderValidated) {
                mCompletedTransactions.insert(message.transactionId);
            }
            auto bufferIter = mBuffers.find(message.bufferId);
            if (message.status == BufferStatus::TRANSFER_OK) {
                handleOwnBuffer(message.connectionId, message.bufferId);
            }
            bufferIter->second->mTransactionCount--;
            if (bufferIter->second->mOwnerCount == 0
                && bufferIter->second->mTransactionCount == 0) {
                if (!bufferIter->second->mInvalidated) {
                    mStats.onBufferUnused(bufferIter->second->mAllocSize);
                    mFreeBuffers.insert(message.bufferId);
                } else {
                    mStats.onBufferUnused(bufferIter->second->mAllocSize);
                    mStats.onBufferEvicted(bufferIter->second->mAllocSize);
                    mBuffers.erase(bufferIter);
                    mInvalidation.onBufferInvalidated(message.bufferId, mInvalidationChannel);
                }
            }
            mTransactions.erase(found);
        }
        ALOGV("transfer finished %llu %u - %d", (unsigned long long)message.transactionId,
              message.bufferId, deleted);
        return deleted;
    }
    ALOGV("transfer not found %llu %u", (unsigned long long)message.transactionId,
          message.bufferId);
    return false;
}

void BufferPool::processStatusMessages() {
    std::vector<BufferStatusMessage> messages;
    mObserver.getBufferStatusChanges(messages);
    mTimestampMs = ::android::elapsedRealtime();
    for (BufferStatusMessage& message: messages) {
        bool ret = false;
        switch (message.status) {
            case BufferStatus::NOT_USED:
                ret = handleReleaseBuffer(
                        message.connectionId, message.bufferId);
                break;
            case BufferStatus::USED:
                // not happening
                break;
            case BufferStatus::TRANSFER_TO:
                ret = handleTransferTo(message);
                break;
            case BufferStatus::TRANSFER_FROM:
                ret = handleTransferFrom(message);
                break;
            case BufferStatus::TRANSFER_TIMEOUT:
                // TODO
                break;
            case BufferStatus::TRANSFER_LOST:
                // TODO
                break;
            case BufferStatus::TRANSFER_FETCH:
                // not happening
                break;
            case BufferStatus::TRANSFER_OK:
            case BufferStatus::TRANSFER_ERROR:
                ret = handleTransferResult(message);
                break;
            case BufferStatus::INVALIDATION_ACK:
                mInvalidation.onAck(message.connectionId, message.bufferId);
                ret = true;
                break;
        }
        if (ret == false) {
            ALOGW("buffer status message processing failure - message : %d connection : %lld",
                  message.status, (long long)message.connectionId);
        }
    }
    messages.clear();
}

bool BufferPool::handleClose(ConnectionId connectionId) {
    // Cleaning buffers
    auto buffers = mUsingBuffers.find(connectionId);
    if (buffers != mUsingBuffers.end()) {
        for (const BufferId& bufferId : buffers->second) {
            bool deleted = erase(&mUsingConnections, bufferId, connectionId);
            if (deleted) {
                auto bufferIter = mBuffers.find(bufferId);
                bufferIter->second->mOwnerCount--;
                if (bufferIter->second->mOwnerCount == 0 &&
                        bufferIter->second->mTransactionCount == 0) {
                    // TODO: handle freebuffer insert fail
                    if (!bufferIter->second->mInvalidated) {
                        mStats.onBufferUnused(bufferIter->second->mAllocSize);
                        mFreeBuffers.insert(bufferId);
                    } else {
                        mStats.onBufferUnused(bufferIter->second->mAllocSize);
                        mStats.onBufferEvicted(bufferIter->second->mAllocSize);
                        mBuffers.erase(bufferIter);
                        mInvalidation.onBufferInvalidated(bufferId, mInvalidationChannel);
                    }
                }
            }
        }
        mUsingBuffers.erase(buffers);
    }

    // Cleaning transactions
    auto pending = mPendingTransactions.find(connectionId);
    if (pending != mPendingTransactions.end()) {
        for (const TransactionId& transactionId : pending->second) {
            auto iter = mTransactions.find(transactionId);
            if (iter != mTransactions.end()) {
                if (!iter->second->mSenderValidated) {
                    mCompletedTransactions.insert(transactionId);
                }
                BufferId bufferId = iter->second->mBufferId;
                auto bufferIter = mBuffers.find(bufferId);
                bufferIter->second->mTransactionCount--;
                if (bufferIter->second->mOwnerCount == 0 &&
                    bufferIter->second->mTransactionCount == 0) {
                    // TODO: handle freebuffer insert fail
                    if (!bufferIter->second->mInvalidated) {
                        mStats.onBufferUnused(bufferIter->second->mAllocSize);
                        mFreeBuffers.insert(bufferId);
                    } else {
                        mStats.onBufferUnused(bufferIter->second->mAllocSize);
                        mStats.onBufferEvicted(bufferIter->second->mAllocSize);
                        mBuffers.erase(bufferIter);
                        mInvalidation.onBufferInvalidated(bufferId, mInvalidationChannel);
                    }
                }
                mTransactions.erase(iter);
            }
        }
    }
    mConnectionIds.erase(connectionId);
    return true;
}

bool BufferPool::getFreeBuffer(
        const std::shared_ptr<BufferPoolAllocator> &allocator,
        const std::vector<uint8_t> &params, BufferId *pId,
        const native_handle_t** handle) {
    auto bufferIt = mFreeBuffers.begin();
    for (;bufferIt != mFreeBuffers.end(); ++bufferIt) {
        BufferId bufferId = *bufferIt;
        if (allocator->compatible(params, mBuffers[bufferId]->mConfig)) {
            break;
        }
    }
    if (bufferIt != mFreeBuffers.end()) {
        BufferId id = *bufferIt;
        mFreeBuffers.erase(bufferIt);
        mStats.onBufferRecycled(mBuffers[id]->mAllocSize);
        *handle = mBuffers[id]->handle();
        *pId = id;
        ALOGV("recycle a buffer %u %p", id, *handle);
        return true;
    }
    return false;
}

BufferPoolStatus BufferPool::addNewBuffer(
        const std::shared_ptr<BufferPoolAllocation> &alloc,
        const size_t allocSize,
        const std::vector<uint8_t> &params,
        BufferId *pId,
        const native_handle_t** handle) {

    BufferId bufferId = mSeq++;
    if (mSeq == Connection::SYNC_BUFFERID) {
        mSeq = 0;
    }
    std::unique_ptr<InternalBuffer> buffer =
            std::make_unique<InternalBuffer>(
                    bufferId, alloc, allocSize, params);
    if (buffer) {
        auto res = mBuffers.insert(std::make_pair(
                bufferId, std::move(buffer)));
        if (res.second) {
            mStats.onBufferAllocated(allocSize);
            *handle = alloc->handle();
            *pId = bufferId;
            return ResultStatus::OK;
        }
    }
    return ResultStatus::NO_MEMORY;
}

void BufferPool::cleanUp(bool clearCache) {
    if (clearCache || mTimestampMs > mLastCleanUpMs + kCleanUpDurationMs ||
            mStats.buffersNotInUse() > kMaxUnusedBufferCount) {
        mLastCleanUpMs = mTimestampMs;
        if (mTimestampMs > mLastLogMs + kLogDurationMs ||
                mStats.buffersNotInUse() > kMaxUnusedBufferCount) {
            mLastLogMs = mTimestampMs;
            ALOGD("bufferpool2 %p : %zu(%zu size) total buffers - "
                  "%zu(%zu size) used buffers - %zu/%zu (recycle/alloc) - "
                  "%zu/%zu (fetch/transfer)",
                  this, mStats.mBuffersCached, mStats.mSizeCached,
                  mStats.mBuffersInUse, mStats.mSizeInUse,
                  mStats.mTotalRecycles, mStats.mTotalAllocations,
                  mStats.mTotalFetches, mStats.mTotalTransfers);
        }
        for (auto freeIt = mFreeBuffers.begin(); freeIt != mFreeBuffers.end();) {
            if (!clearCache && mStats.buffersNotInUse() <= kUnusedBufferCountTarget &&
                    (mStats.mSizeCached < kMinAllocBytesForEviction ||
                     mBuffers.size() < kMinBufferCountForEviction)) {
                break;
            }
            auto it = mBuffers.find(*freeIt);
            if (it != mBuffers.end() &&
                    it->second->mOwnerCount == 0 && it->second->mTransactionCount == 0) {
                mStats.onBufferEvicted(it->second->mAllocSize);
                mBuffers.erase(it);
                freeIt = mFreeBuffers.erase(freeIt);
            } else {
                ++freeIt;
                ALOGW("bufferpool2 inconsistent!");
            }
        }
    }
}

void BufferPool::invalidate(
        bool needsAck, BufferId from, BufferId to,
        const std::shared_ptr<Accessor> &impl) {
    for (auto freeIt = mFreeBuffers.begin(); freeIt != mFreeBuffers.end();) {
        if (isBufferInRange(from, to, *freeIt)) {
            auto it = mBuffers.find(*freeIt);
            if (it != mBuffers.end() &&
                it->second->mOwnerCount == 0 && it->second->mTransactionCount == 0) {
                mStats.onBufferEvicted(it->second->mAllocSize);
                mBuffers.erase(it);
                freeIt = mFreeBuffers.erase(freeIt);
                continue;
            } else {
                ALOGW("bufferpool2 inconsistent!");
            }
        }
        ++freeIt;
    }

    size_t left = 0;
    for (auto it = mBuffers.begin(); it != mBuffers.end(); ++it) {
        if (isBufferInRange(from, to, it->first)) {
            it->second->invalidate();
            ++left;
        }
    }
    mInvalidation.onInvalidationRequest(needsAck, from, to, left, mInvalidationChannel, impl);
}

void BufferPool::flush(const std::shared_ptr<Accessor> &impl) {
    BufferId from = mStartSeq;
    BufferId to = mSeq;
    mStartSeq = mSeq;
    // TODO: needsAck params
    ALOGV("buffer invalidation request bp:%u %u %u", mInvalidation.mId, from, to);
    if (from != to) {
        invalidate(true, from, to, impl);
    }
}

}  // namespace aidl::android::hardware::media::bufferpool2::implementation
