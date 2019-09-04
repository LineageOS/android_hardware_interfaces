/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "SensorsTestSharedMemory.h"

#include <log/log.h>

#include <sys/mman.h>
#include <cinttypes>

using namespace ::android::hardware::sensors::V1_0;

SharedMemInfo SensorsTestSharedMemory::getSharedMemInfo() const {
    SharedMemInfo mem = {.type = mType,
                         .format = SharedMemFormat::SENSORS_EVENT,
                         .size = static_cast<uint32_t>(mSize),
                         .memoryHandle = mNativeHandle};
    return mem;
}

char* SensorsTestSharedMemory::getBuffer() const {
    return mBuffer;
}

size_t SensorsTestSharedMemory::getSize() const {
    return mSize;
}

std::vector<Event> SensorsTestSharedMemory::parseEvents(int64_t lastCounter, size_t offset) const {
    constexpr size_t kEventSize = static_cast<size_t>(SensorsEventFormatOffset::TOTAL_LENGTH);
    constexpr size_t kOffsetSize = static_cast<size_t>(SensorsEventFormatOffset::SIZE_FIELD);
    constexpr size_t kOffsetToken = static_cast<size_t>(SensorsEventFormatOffset::REPORT_TOKEN);
    constexpr size_t kOffsetType = static_cast<size_t>(SensorsEventFormatOffset::SENSOR_TYPE);
    constexpr size_t kOffsetAtomicCounter =
        static_cast<size_t>(SensorsEventFormatOffset::ATOMIC_COUNTER);
    constexpr size_t kOffsetTimestamp = static_cast<size_t>(SensorsEventFormatOffset::TIMESTAMP);
    constexpr size_t kOffsetData = static_cast<size_t>(SensorsEventFormatOffset::DATA);

    std::vector<Event> events;
    std::vector<float> data(16);

    while (offset + kEventSize <= mSize) {
        int64_t atomicCounter =
            *reinterpret_cast<uint32_t*>(mBuffer + offset + kOffsetAtomicCounter);
        if (atomicCounter <= lastCounter) {
            ALOGV("atomicCounter = %" PRId64 ", lastCounter = %" PRId64, atomicCounter,
                  lastCounter);
            break;
        }

        int32_t size = *reinterpret_cast<int32_t*>(mBuffer + offset + kOffsetSize);
        if (size != kEventSize) {
            // unknown error, events parsed may be wrong, remove all
            events.clear();
            break;
        }

        int32_t token = *reinterpret_cast<int32_t*>(mBuffer + offset + kOffsetToken);
        int32_t type = *reinterpret_cast<int32_t*>(mBuffer + offset + kOffsetType);
        int64_t timestamp = *reinterpret_cast<int64_t*>(mBuffer + offset + kOffsetTimestamp);

        ALOGV("offset = %zu, cnt %" PRId64 ", token %" PRId32 ", type %" PRId32
              ", timestamp %" PRId64,
              offset, atomicCounter, token, type, timestamp);

        Event event = {
            .timestamp = timestamp,
            .sensorHandle = token,
            .sensorType = static_cast<SensorType>(type),
        };
        event.u.data = android::hardware::hidl_array<float, 16>(
            reinterpret_cast<float*>(mBuffer + offset + kOffsetData));

        events.push_back(event);

        lastCounter = atomicCounter;
        offset += kEventSize;
    }

    return events;
}

SensorsTestSharedMemory::SensorsTestSharedMemory(SharedMemType type, size_t size)
    : mType(type), mSize(0), mBuffer(nullptr) {
    native_handle_t* handle = nullptr;
    char* buffer = nullptr;
    switch (type) {
        case SharedMemType::ASHMEM: {
            int fd;
            handle = ::native_handle_create(1 /*nFds*/, 0 /*nInts*/);
            if (handle != nullptr) {
                handle->data[0] = fd = ::ashmem_create_region("SensorsTestSharedMemory", size);
                if (handle->data[0] > 0) {
                    // memory is pinned by default
                    buffer = static_cast<char*>(
                        ::mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
                    if (buffer != reinterpret_cast<char*>(MAP_FAILED)) {
                        break;
                    }
                    ::native_handle_close(handle);
                }
                ::native_handle_delete(handle);
                handle = nullptr;
            }
            break;
        }
        case SharedMemType::GRALLOC: {
            mGrallocWrapper = std::make_unique<::android::GrallocWrapper>();
            if (mGrallocWrapper->getAllocator() == nullptr ||
                mGrallocWrapper->getMapper() == nullptr) {
                break;
            }
            using android::hardware::graphics::common::V1_0::BufferUsage;
            using android::hardware::graphics::common::V1_0::PixelFormat;
            mapper2::IMapper::BufferDescriptorInfo buf_desc_info = {
                .width = static_cast<uint32_t>(size),
                .height = 1,
                .layerCount = 1,
                .usage = static_cast<uint64_t>(BufferUsage::SENSOR_DIRECT_DATA |
                                               BufferUsage::CPU_READ_OFTEN),
                .format = PixelFormat::BLOB};

            handle = const_cast<native_handle_t*>(mGrallocWrapper->allocate(buf_desc_info));
            if (handle != nullptr) {
                mapper2::IMapper::Rect region{0, 0, static_cast<int32_t>(buf_desc_info.width),
                                              static_cast<int32_t>(buf_desc_info.height)};
                buffer = static_cast<char*>(
                    mGrallocWrapper->lock(handle, buf_desc_info.usage, region, /*fence=*/-1));
                if (buffer != nullptr) {
                    break;
                }
                mGrallocWrapper->freeBuffer(handle);
                handle = nullptr;
            }
            break;
        }
        default:
            break;
    }

    if (buffer != nullptr) {
        mNativeHandle = handle;
        mSize = size;
        mBuffer = buffer;
    }
}

SensorsTestSharedMemory::~SensorsTestSharedMemory() {
    switch (mType) {
        case SharedMemType::ASHMEM: {
            if (mSize != 0) {
                ::munmap(mBuffer, mSize);
                mBuffer = nullptr;

                ::native_handle_close(mNativeHandle);
                ::native_handle_delete(mNativeHandle);

                mNativeHandle = nullptr;
                mSize = 0;
            }
            break;
        }
        case SharedMemType::GRALLOC: {
            if (mSize != 0) {
                mGrallocWrapper->unlock(mNativeHandle);
                mGrallocWrapper->freeBuffer(mNativeHandle);

                mNativeHandle = nullptr;
                mSize = 0;
            }
            break;
        }
        default: {
            if (mNativeHandle != nullptr || mSize != 0 || mBuffer != nullptr) {
                ALOGE(
                    "SensorsTestSharedMemory %p not properly destructed: "
                    "type %d, native handle %p, size %zu, buffer %p",
                    this, static_cast<int>(mType), mNativeHandle, mSize, mBuffer);
            }
            break;
        }
    }
}

SensorsTestSharedMemory* SensorsTestSharedMemory::create(SharedMemType type, size_t size) {
    constexpr size_t kMaxSize = 128 * 1024 * 1024;  // sensor test should not need more than 128M
    if (size == 0 || size >= kMaxSize) {
        return nullptr;
    }

    auto m = new SensorsTestSharedMemory(type, size);
    if (m->mSize != size || m->mBuffer == nullptr) {
        delete m;
        m = nullptr;
    }
    return m;
}
