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

#include <aidl/android/hardware/media/bufferpool2/BufferStatusMessage.h>
#include <bufferpool2/BufferPoolTypes.h>

#include <map>
#include <set>

namespace aidl::android::hardware::media::bufferpool2::implementation {

// Helper template methods for handling map of set.
template<class T, class U>
bool insert(std::map<T, std::set<U>> *mapOfSet, T key, U value) {
    auto iter = mapOfSet->find(key);
    if (iter == mapOfSet->end()) {
        std::set<U> valueSet{value};
        mapOfSet->insert(std::make_pair(key, valueSet));
        return true;
    } else if (iter->second.find(value)  == iter->second.end()) {
        iter->second.insert(value);
        return true;
    }
    return false;
}

// Helper template methods for handling map of set.
template<class T, class U>
bool erase(std::map<T, std::set<U>> *mapOfSet, T key, U value) {
    bool ret = false;
    auto iter = mapOfSet->find(key);
    if (iter != mapOfSet->end()) {
        if (iter->second.erase(value) > 0) {
            ret = true;
        }
        if (iter->second.size() == 0) {
            mapOfSet->erase(iter);
        }
    }
    return ret;
}

// Helper template methods for handling map of set.
template<class T, class U>
bool contains(std::map<T, std::set<U>> *mapOfSet, T key, U value) {
    auto iter = mapOfSet->find(key);
    if (iter != mapOfSet->end()) {
        auto setIter = iter->second.find(value);
        return setIter != iter->second.end();
    }
    return false;
}

// Buffer data structure for internal BufferPool use.(storage/fetching)
struct InternalBuffer {
    BufferId mId;
    size_t mOwnerCount;
    size_t mTransactionCount;
    const std::shared_ptr<BufferPoolAllocation> mAllocation;
    const size_t mAllocSize;
    const std::vector<uint8_t> mConfig;
    bool mInvalidated;

    InternalBuffer(
            BufferId id,
            const std::shared_ptr<BufferPoolAllocation> &alloc,
            const size_t allocSize,
            const std::vector<uint8_t> &allocConfig)
            : mId(id), mOwnerCount(0), mTransactionCount(0),
            mAllocation(alloc), mAllocSize(allocSize), mConfig(allocConfig),
            mInvalidated(false) {}

    const native_handle_t *handle() {
        return mAllocation->handle();
    }

    void invalidate() {
        mInvalidated = true;
    }
};

// Buffer transacion status/message data structure for internal BufferPool use.
struct TransactionStatus {
    TransactionId mId;
    BufferId mBufferId;
    ConnectionId mSender;
    ConnectionId mReceiver;
    BufferStatus mStatus;
    int64_t mTimestampMs;
    bool mSenderValidated;

    TransactionStatus(const BufferStatusMessage &message, int64_t timestampMs) {
        mId = message.transactionId;
        mBufferId = message.bufferId;
        mStatus = message.status;
        mTimestampMs = timestampMs;
        if (mStatus == BufferStatus::TRANSFER_TO) {
            mSender = message.connectionId;
            mReceiver = message.targetConnectionId;
            mSenderValidated = true;
        } else {
            mSender = -1LL;
            mReceiver = message.connectionId;
            mSenderValidated = false;
        }
    }
};

}  // namespace aidl::android::hardware::media::bufferpool2::implementation
