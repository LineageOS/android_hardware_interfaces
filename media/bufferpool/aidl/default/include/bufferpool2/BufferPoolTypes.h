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

#include <cutils/native_handle.h>
#include <fmq/AidlMessageQueue.h>
#include <aidl/android/hardware/media/bufferpool2/BufferStatusMessage.h>
#include <aidl/android/hardware/media/bufferpool2/BufferInvalidationMessage.h>
#include <aidl/android/hardware/media/bufferpool2/ResultStatus.h>

namespace aidl::android::hardware::media::bufferpool2 {

struct BufferPoolData {
    // For local use, to specify a bufferpool (client connection) for buffers.
    // Retrieved from returned info of IAccessor#connect(android.hardware.media.bufferpool@2.0).
    int64_t mConnectionId;
    // BufferId
    uint32_t mId;

    BufferPoolData() : mConnectionId(0), mId(0) {}

    BufferPoolData(
            int64_t connectionId, uint32_t id)
            : mConnectionId(connectionId), mId(id) {}

    ~BufferPoolData() {}
};

namespace implementation {

using aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using aidl::android::hardware::common::fmq::UnsynchronizedWrite;

using aidl::android::hardware::media::bufferpool2::BufferStatusMessage;
using aidl::android::hardware::media::bufferpool2::BufferInvalidationMessage;

typedef uint32_t BufferId;
typedef uint64_t TransactionId;
typedef int64_t ConnectionId;
typedef int32_t BufferPoolStatus;

// AIDL hal description language does not support unsigned.
int32_t static inline ToAidl(BufferId id) {return static_cast<int32_t>(id);}
int64_t static inline ToAidl(TransactionId id) {return static_cast<int64_t>(id);}

BufferId static inline FromAidl(int32_t id) {return static_cast<BufferId>(id);}
TransactionId static inline FromAidl(int64_t id) {return static_cast<TransactionId>(id);}

enum : ConnectionId {
    INVALID_CONNECTIONID = 0,
};

typedef ::android::AidlMessageQueue<BufferStatusMessage, SynchronizedReadWrite> BufferStatusQueue;
typedef aidl::android::hardware::common::fmq::MQDescriptor<BufferStatusMessage, SynchronizedReadWrite>
        StatusDescriptor;

typedef ::android::AidlMessageQueue<BufferInvalidationMessage, UnsynchronizedWrite>
        BufferInvalidationQueue;
typedef aidl::android::hardware::common::fmq::MQDescriptor<BufferInvalidationMessage, UnsynchronizedWrite>
        InvalidationDescriptor;

/**
 * Allocation wrapper class for buffer pool.
 */
struct BufferPoolAllocation {
    const native_handle_t *mHandle;

    const native_handle_t *handle() {
        return mHandle;
    }

    BufferPoolAllocation(const native_handle_t *handle) : mHandle(handle) {}

    ~BufferPoolAllocation() {};
};

/**
 * Allocator wrapper class for buffer pool.
 */
class BufferPoolAllocator {
public:

    /**
     * Allocate an allocation(buffer) for buffer pool.
     *
     * @param params    allocation parameters
     * @param alloc     created allocation
     * @param allocSize size of created allocation
     *
     * @return OK when an allocation is created successfully.
     */
    virtual BufferPoolStatus allocate(
            const std::vector<uint8_t> &params,
            std::shared_ptr<BufferPoolAllocation> *alloc,
            size_t *allocSize) = 0;

    /**
     * Returns whether allocation parameters of an old allocation are
     * compatible with new allocation parameters.
     */
    virtual bool compatible(const std::vector<uint8_t> &newParams,
                            const std::vector<uint8_t> &oldParams) = 0;

protected:
    BufferPoolAllocator() = default;

    virtual ~BufferPoolAllocator() = default;
};

}  // namespace implementation
}  // namespace aidl::android::hareware::media::bufferpool2

