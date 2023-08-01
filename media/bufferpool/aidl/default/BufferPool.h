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

#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <utils/Timers.h>

#include "BufferStatus.h"

namespace aidl::android::hardware::media::bufferpool2::implementation {

using BufferStatus = aidl::android::hardware::media::bufferpool2::BufferStatus;
using BufferStatusMessage = aidl::android::hardware::media::bufferpool2::BufferStatusMessage;

struct Accessor;
struct InternalBuffer;
struct TransactionStatus;

/**
 * Buffer pool implementation.
 *
 * Handles buffer status messages. Handles buffer allocation/recycling.
 * Handles buffer transfer between buffer pool clients.
 */
struct BufferPool {
private:
    std::mutex mMutex;
    int64_t mTimestampMs;
    int64_t mLastCleanUpMs;
    int64_t mLastLogMs;
    BufferId mSeq;
    BufferId mStartSeq;
    bool mValid;
    BufferStatusObserver mObserver;
    BufferInvalidationChannel mInvalidationChannel;

    std::map<ConnectionId, std::set<BufferId>> mUsingBuffers;
    std::map<BufferId, std::set<ConnectionId>> mUsingConnections;

    std::map<ConnectionId, std::set<TransactionId>> mPendingTransactions;
    // Transactions completed before TRANSFER_TO message arrival.
    // Fetch does not occur for the transactions.
    // Only transaction id is kept for the transactions in short duration.
    std::set<TransactionId> mCompletedTransactions;
    // Currently active(pending) transations' status & information.
    std::map<TransactionId, std::unique_ptr<TransactionStatus>>
            mTransactions;

    std::map<BufferId, std::unique_ptr<InternalBuffer>> mBuffers;
    std::set<BufferId> mFreeBuffers;
    std::set<ConnectionId> mConnectionIds;

    struct Invalidation {
        static std::atomic<std::uint32_t> sInvSeqId;

        struct Pending {
            bool mNeedsAck;
            uint32_t mFrom;
            uint32_t mTo;
            size_t mLeft;
            const std::weak_ptr<Accessor> mImpl;
            Pending(bool needsAck, uint32_t from, uint32_t to, size_t left,
                    const std::shared_ptr<Accessor> &impl)
                    : mNeedsAck(needsAck),
                      mFrom(from),
                      mTo(to),
                      mLeft(left),
                      mImpl(impl)
            {}

            bool isInvalidated(uint32_t bufferId) {
                return isBufferInRange(mFrom, mTo, bufferId) && --mLeft == 0;
            }
        };

        std::list<Pending> mPendings;
        std::map<ConnectionId, uint32_t> mAcks;
        std::map<ConnectionId, const std::shared_ptr<IObserver>> mObservers;
        uint32_t mInvalidationId;
        uint32_t mId;

        Invalidation() : mInvalidationId(0), mId(sInvSeqId.fetch_add(1)) {}

        void onConnect(ConnectionId conId, const std::shared_ptr<IObserver> &observer);

        void onClose(ConnectionId conId);

        void onAck(ConnectionId conId, uint32_t msgId);

        void onBufferInvalidated(
                BufferId bufferId,
                BufferInvalidationChannel &channel);

        void onInvalidationRequest(
                bool needsAck, uint32_t from, uint32_t to, size_t left,
                BufferInvalidationChannel &channel,
                const std::shared_ptr<Accessor> &impl);

        void onHandleAck(
                std::map<ConnectionId, const std::shared_ptr<IObserver>> *observers,
                uint32_t *invalidationId);
    } mInvalidation;
    /// Buffer pool statistics which tracks allocation and transfer statistics.
    struct Stats {
        /// Total size of allocations which are used or available to use.
        /// (bytes or pixels)
        size_t mSizeCached;
        /// # of cached buffers which are used or available to use.
        size_t mBuffersCached;
        /// Total size of allocations which are currently used. (bytes or pixels)
        size_t mSizeInUse;
        /// # of currently used buffers
        size_t mBuffersInUse;

        /// # of allocations called on bufferpool. (# of fetched from BlockPool)
        size_t mTotalAllocations;
        /// # of allocations that were served from the cache.
        /// (# of allocator alloc prevented)
        size_t mTotalRecycles;
        /// # of buffer transfers initiated.
        size_t mTotalTransfers;
        /// # of transfers that had to be fetched.
        size_t mTotalFetches;

        Stats()
            : mSizeCached(0), mBuffersCached(0), mSizeInUse(0), mBuffersInUse(0),
              mTotalAllocations(0), mTotalRecycles(0), mTotalTransfers(0), mTotalFetches(0) {}

        /// # of currently unused buffers
        size_t buffersNotInUse() const {
            ALOG_ASSERT(mBuffersCached >= mBuffersInUse);
            return mBuffersCached - mBuffersInUse;
        }

        /// A new buffer is allocated on an allocation request.
        void onBufferAllocated(size_t allocSize) {
            mSizeCached += allocSize;
            mBuffersCached++;

            mSizeInUse += allocSize;
            mBuffersInUse++;

            mTotalAllocations++;
        }

        /// A buffer is evicted and destroyed.
        void onBufferEvicted(size_t allocSize) {
            mSizeCached -= allocSize;
            mBuffersCached--;
        }

        /// A buffer is recycled on an allocation request.
        void onBufferRecycled(size_t allocSize) {
            mSizeInUse += allocSize;
            mBuffersInUse++;

            mTotalAllocations++;
            mTotalRecycles++;
        }

        /// A buffer is available to be recycled.
        void onBufferUnused(size_t allocSize) {
            mSizeInUse -= allocSize;
            mBuffersInUse--;
        }

        /// A buffer transfer is initiated.
        void onBufferSent() {
            mTotalTransfers++;
        }

        /// A buffer fetch is invoked by a buffer transfer.
        void onBufferFetched() {
            mTotalFetches++;
        }
    } mStats;

    bool isValid() {
        return mValid;
    }

    void invalidate(bool needsAck, BufferId from, BufferId to,
                    const std::shared_ptr<Accessor> &impl);

    static void createInvalidator();

public:
    /** Creates a buffer pool. */
    BufferPool();

    /** Destroys a buffer pool. */
    ~BufferPool();

    /**
     * Processes all pending buffer status messages, and returns the result.
     * Each status message is handled by methods with 'handle' prefix.
     */
    void processStatusMessages();

    /**
     * Handles a buffer being owned by a connection.
     *
     * @param connectionId  the id of the buffer owning connection.
     * @param bufferId      the id of the buffer.
     *
     * @return {@code true} when the buffer is owned,
     *         {@code false} otherwise.
     */
    bool handleOwnBuffer(ConnectionId connectionId, BufferId bufferId);

    /**
     * Handles a buffer being released by a connection.
     *
     * @param connectionId  the id of the buffer owning connection.
     * @param bufferId      the id of the buffer.
     *
     * @return {@code true} when the buffer ownership is released,
     *         {@code false} otherwise.
     */
    bool handleReleaseBuffer(ConnectionId connectionId, BufferId bufferId);

    /**
     * Handles a transfer transaction start message from the sender.
     *
     * @param message   a buffer status message for the transaction.
     *
     * @result {@code true} when transfer_to message is acknowledged,
     *         {@code false} otherwise.
     */
    bool handleTransferTo(const BufferStatusMessage &message);

    /**
     * Handles a transfer transaction being acked by the receiver.
     *
     * @param message   a buffer status message for the transaction.
     *
     * @result {@code true} when transfer_from message is acknowledged,
     *         {@code false} otherwise.
     */
    bool handleTransferFrom(const BufferStatusMessage &message);

    /**
     * Handles a transfer transaction result message from the receiver.
     *
     * @param message   a buffer status message for the transaction.
     *
     * @result {@code true} when the existing transaction is finished,
     *         {@code false} otherwise.
     */
    bool handleTransferResult(const BufferStatusMessage &message);

    /**
     * Handles a connection being closed, and returns the result. All the
     * buffers and transactions owned by the connection will be cleaned up.
     * The related FMQ will be cleaned up too.
     *
     * @param connectionId  the id of the connection.
     *
     * @result {@code true} when the connection existed,
     *         {@code false} otherwise.
     */
    bool handleClose(ConnectionId connectionId);

    /**
     * Recycles a existing free buffer if it is possible.
     *
     * @param allocator the buffer allocator
     * @param params    the allocation parameters.
     * @param pId       the id of the recycled buffer.
     * @param handle    the native handle of the recycled buffer.
     *
     * @return {@code true} when a buffer is recycled, {@code false}
     *         otherwise.
     */
    bool getFreeBuffer(
            const std::shared_ptr<BufferPoolAllocator> &allocator,
            const std::vector<uint8_t> &params,
            BufferId *pId, const native_handle_t **handle);

    /**
     * Adds a newly allocated buffer to bufferpool.
     *
     * @param alloc     the newly allocated buffer.
     * @param allocSize the size of the newly allocated buffer.
     * @param params    the allocation parameters.
     * @param pId       the buffer id for the newly allocated buffer.
     * @param handle    the native handle for the newly allocated buffer.
     *
     * @return OK when an allocation is successfully allocated.
     *         NO_MEMORY when there is no memory.
     *         CRITICAL_ERROR otherwise.
     */
    BufferPoolStatus addNewBuffer(
            const std::shared_ptr<BufferPoolAllocation> &alloc,
            const size_t allocSize,
            const std::vector<uint8_t> &params,
            BufferId *pId,
            const native_handle_t **handle);

    /**
     * Processes pending buffer status messages and performs periodic cache
     * cleaning.
     *
     * @param clearCache    if clearCache is true, it frees all buffers
     *                      waiting to be recycled.
     */
    void cleanUp(bool clearCache = false);

    /**
     * Processes pending buffer status messages and invalidate all current
     * free buffers. Active buffers are invalidated after being inactive.
     */
    void flush(const std::shared_ptr<Accessor> &impl);

    friend struct Accessor;
};


}  // namespace aidl::android::hardware::media::bufferpool2::implementation
