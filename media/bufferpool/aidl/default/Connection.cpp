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
#define LOG_TAG "AidlBufferPoolCon"
//#define LOG_NDEBUG 0

#include <aidlcommonsupport/NativeHandle.h>

#include "Connection.h"
#include "Accessor.h"

namespace aidl::android::hardware::media::bufferpool2::implementation {

using aidl::android::hardware::media::bufferpool2::ResultStatus;
using Buffer = aidl::android::hardware::media::bufferpool2::Buffer;
using FetchInfo = aidl::android::hardware::media::bufferpool2::IConnection::FetchInfo;
using FetchResult = aidl::android::hardware::media::bufferpool2::IConnection::FetchResult;

::ndk::ScopedAStatus Connection::fetch(const std::vector<FetchInfo>& in_fetchInfos,
                           std::vector<FetchResult>* _aidl_return) {
    int success = 0;
    int failure = 0;
    if (mInitialized && mAccessor) {
        for (auto it = in_fetchInfos.begin(); it != in_fetchInfos.end(); ++it) {
            if (fetch(it->transactionId, it->bufferId, _aidl_return)) {
                success++;
            } else {
                failure++;
            }
        }
        if (failure > 0) {
            ALOGD("total fetch %d, failure %d", success + failure, failure);
        }
        return ::ndk::ScopedAStatus::ok();
    }
    return ::ndk::ScopedAStatus::fromServiceSpecificError(ResultStatus::CRITICAL_ERROR);
}

::ndk::ScopedAStatus Connection::sync() {
    if (mInitialized && mAccessor) {
        mAccessor->cleanUp(false);
    }
    return ::ndk::ScopedAStatus::ok();
}


bool Connection::fetch(TransactionId transactionId, BufferId bufferId,
                       std::vector<FetchResult> *result) {
    BufferPoolStatus status = ResultStatus::CRITICAL_ERROR;
    const native_handle_t *handle = nullptr;
    status = mAccessor->fetch(
            mConnectionId, transactionId, bufferId, &handle);
    if (status == ResultStatus::OK) {
        result->emplace_back(FetchResult::make<FetchResult::buffer>());
        result->back().get<FetchResult::buffer>().id = bufferId;
        result->back().get<FetchResult::buffer>().buffer = ::android::dupToAidl(handle);
        return true;
    }
    result->emplace_back(FetchResult::make<FetchResult::failure>(status));
    return false;
}

Connection::Connection() : mInitialized(false), mConnectionId(-1LL) {}

Connection::~Connection() {
    if (mInitialized && mAccessor) {
        mAccessor->close(mConnectionId);
    }
}

void Connection::initialize(
        const std::shared_ptr<Accessor>& accessor, ConnectionId connectionId) {
    if (!mInitialized) {
        mAccessor = accessor;
        mConnectionId = connectionId;
        mInitialized = true;
    }
}

BufferPoolStatus Connection::flush() {
    if (mInitialized && mAccessor) {
        return mAccessor->flush();
    }
    return ResultStatus::CRITICAL_ERROR;
}

BufferPoolStatus Connection::allocate(
        const std::vector<uint8_t> &params, BufferId *bufferId,
        const native_handle_t **handle) {
    if (mInitialized && mAccessor) {
        return mAccessor->allocate(mConnectionId, params, bufferId, handle);
    }
    return ResultStatus::CRITICAL_ERROR;
}

void Connection::cleanUp(bool clearCache) {
    if (mInitialized && mAccessor) {
        mAccessor->cleanUp(clearCache);
    }
}

}  // namespace ::aidl::android::hardware::media::bufferpool2::implementation
