/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "GrallocWrapper.h"

#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <android/hardware/graphics/allocator/3.0/IAllocator.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <android/hardware/graphics/mapper/2.1/IMapper.h>
#include <android/hardware/graphics/mapper/3.0/IMapper.h>

#include <utils/Log.h>

#include <cinttypes>
#include <type_traits>

using IAllocator2 = ::android::hardware::graphics::allocator::V2_0::IAllocator;
using IAllocator3 = ::android::hardware::graphics::allocator::V3_0::IAllocator;
using IMapper2 = ::android::hardware::graphics::mapper::V2_0::IMapper;
using IMapper2_1 = ::android::hardware::graphics::mapper::V2_1::IMapper;
using IMapper3 = ::android::hardware::graphics::mapper::V3_0::IMapper;

using Error2 = ::android::hardware::graphics::mapper::V2_0::Error;
using Error3 = ::android::hardware::graphics::mapper::V3_0::Error;

using ::android::hardware::graphics::common::V1_0::BufferUsage;
using ::android::hardware::graphics::common::V1_0::PixelFormat;

// This is a typedef to the same underlying type across v2.0 and v3.0
using ::android::hardware::graphics::mapper::V2_0::BufferDescriptor;

using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;

namespace android {

// Since we use the same APIs across allocator/mapper HALs but they have major
// version differences (meaning they are not related through inheritance), we
// create a common interface abstraction for the IAllocator + IMapper combination
// (major versions need to match in the current HALs, e.g. IAllocator 3.0 needs to
// be paired with IMapper 3.0, so these are tied together)
class IGrallocHalWrapper {
  public:
    virtual ~IGrallocHalWrapper() = default;

    // IAllocator
    virtual std::string dumpDebugInfo() = 0;
    virtual native_handle_t* allocate(uint32_t size) = 0;
    virtual void freeBuffer(native_handle_t* bufferHandle) = 0;

    // IMapper
    virtual void* lock(native_handle_t* bufferHandle) = 0;
    virtual void unlock(native_handle_t* bufferHandle) = 0;
};

namespace {

bool failed(Error2 error) {
    return (error != Error2::NONE);
}
bool failed(Error3 error) {
    return (error != Error3::NONE);
}

// Since all the type and function names are the same for the things we use across the major HAL
// versions, we use template magic to avoid repeating ourselves.
template <typename AllocatorT, typename MapperT>
class GrallocHalWrapper : public IGrallocHalWrapper {
  public:
    GrallocHalWrapper(const sp<AllocatorT>& allocator, const sp<MapperT>& mapper)
        : mAllocator(allocator), mMapper(mapper) {
        if (mapper->isRemote()) {
            ALOGE("Mapper is in passthrough mode");
        }
    }

    virtual std::string dumpDebugInfo() override;
    virtual native_handle_t* allocate(uint32_t size) override;
    virtual void freeBuffer(native_handle_t* bufferHandle) override;

    virtual void* lock(native_handle_t* bufferHandle) override;
    virtual void unlock(native_handle_t* bufferHandle) override;

  private:
    static constexpr uint64_t kBufferUsage =
            static_cast<uint64_t>(BufferUsage::SENSOR_DIRECT_DATA | BufferUsage::CPU_READ_OFTEN);
    sp<AllocatorT> mAllocator;
    sp<MapperT> mMapper;

    BufferDescriptor getDescriptor(uint32_t size);
    native_handle_t* importBuffer(const hidl_handle& rawHandle);
};

template <typename AllocatorT, typename MapperT>
std::string GrallocHalWrapper<AllocatorT, MapperT>::dumpDebugInfo() {
    std::string debugInfo;
    mAllocator->dumpDebugInfo([&](const hidl_string& tmpDebugInfo) { debugInfo = tmpDebugInfo; });
    return debugInfo;
}

template <typename AllocatorT, typename MapperT>
native_handle_t* GrallocHalWrapper<AllocatorT, MapperT>::allocate(uint32_t size) {
    constexpr uint32_t kBufferCount = 1;
    BufferDescriptor descriptor = getDescriptor(size);
    native_handle_t* bufferHandle = nullptr;

    auto callback = [&](auto error, uint32_t /*stride*/, const hidl_vec<hidl_handle>& buffers) {
        if (failed(error)) {
            ALOGE("Failed to allocate buffer: %" PRId32, static_cast<int32_t>(error));
        } else if (buffers.size() != kBufferCount) {
            ALOGE("Invalid buffer array size (got %zu, expected %" PRIu32 ")", buffers.size(),
                  kBufferCount);
        } else {
            bufferHandle = importBuffer(buffers[0]);
        }
    };

    mAllocator->allocate(descriptor, kBufferCount, callback);
    return bufferHandle;
}

template <typename AllocatorT, typename MapperT>
void GrallocHalWrapper<AllocatorT, MapperT>::freeBuffer(native_handle_t* bufferHandle) {
    auto error = mMapper->freeBuffer(bufferHandle);
    if (!error.isOk() || failed(error)) {
        ALOGE("Failed to free buffer %p", bufferHandle);
    }
}

template <typename AllocatorT, typename MapperT>
BufferDescriptor GrallocHalWrapper<AllocatorT, MapperT>::getDescriptor(uint32_t size) {
    typename MapperT::BufferDescriptorInfo descriptorInfo = {
            .width = size,
            .height = 1,
            .layerCount = 1,
            .usage = kBufferUsage,
            .format = static_cast<decltype(descriptorInfo.format)>(PixelFormat::BLOB),
    };

    BufferDescriptor descriptor;
    auto callback = [&](auto error, const BufferDescriptor& tmpDescriptor) {
        if (failed(error)) {
            ALOGE("Failed to create descriptor: %" PRId32, static_cast<int32_t>(error));
        } else {
            descriptor = tmpDescriptor;
        }
    };

    mMapper->createDescriptor(descriptorInfo, callback);
    return descriptor;
}

template <typename AllocatorT, typename MapperT>
native_handle_t* GrallocHalWrapper<AllocatorT, MapperT>::importBuffer(
        const hidl_handle& rawHandle) {
    native_handle_t* bufferHandle = nullptr;

    mMapper->importBuffer(rawHandle, [&](auto error, void* tmpBuffer) {
        if (failed(error)) {
            ALOGE("Failed to import buffer %p: %" PRId32, rawHandle.getNativeHandle(),
                  static_cast<int32_t>(error));
        } else {
            bufferHandle = static_cast<native_handle_t*>(tmpBuffer);
        }
    });

    return bufferHandle;
}

template <typename AllocatorT, typename MapperT>
void* GrallocHalWrapper<AllocatorT, MapperT>::lock(native_handle_t* bufferHandle) {
    // Per the HAL, all-zeros Rect means the entire buffer
    typename MapperT::Rect accessRegion = {};
    hidl_handle acquireFenceHandle;  // No fence needed, already safe to lock

    void* data = nullptr;
    mMapper->lock(bufferHandle, kBufferUsage, accessRegion, acquireFenceHandle,
                  [&](auto error, void* tmpData, ...) {  // V3_0 passes extra args we don't use
                      if (failed(error)) {
                          ALOGE("Failed to lock buffer %p: %" PRId32, bufferHandle,
                                static_cast<int32_t>(error));
                      } else {
                          data = tmpData;
                      }
                  });

    return data;
}

template <typename AllocatorT, typename MapperT>
void GrallocHalWrapper<AllocatorT, MapperT>::unlock(native_handle_t* bufferHandle) {
    mMapper->unlock(bufferHandle, [&](auto error, const hidl_handle& /*releaseFence*/) {
        if (failed(error)) {
            ALOGE("Failed to unlock buffer %p: %" PRId32, bufferHandle,
                  static_cast<int32_t>(error));
        }
    });
}

}  // anonymous namespace

GrallocWrapper::GrallocWrapper() {
    sp<IAllocator3> allocator3 = IAllocator3::getService();
    sp<IMapper3> mapper3 = IMapper3::getService();

    if (allocator3 != nullptr && mapper3 != nullptr) {
        mGrallocHal = std::unique_ptr<IGrallocHalWrapper>(
                new GrallocHalWrapper<IAllocator3, IMapper3>(allocator3, mapper3));
    } else {
        ALOGD("Graphics HALs 3.0 not found (allocator %d mapper %d), falling back to 2.x",
              (allocator3 != nullptr), (mapper3 != nullptr));

        sp<IAllocator2> allocator2 = IAllocator2::getService();
        sp<IMapper2> mapper2 = IMapper2_1::getService();
        if (mapper2 == nullptr) {
            mapper2 = IMapper2::getService();
        }

        if (allocator2 != nullptr && mapper2 != nullptr) {
            mGrallocHal = std::unique_ptr<IGrallocHalWrapper>(
                    new GrallocHalWrapper<IAllocator2, IMapper2>(allocator2, mapper2));
        } else {
            ALOGE("Couldn't open 2.x/3.0 graphics HALs (2.x allocator %d mapper %d)",
                  (allocator2 != nullptr), (mapper2 != nullptr));
        }
    }
}

GrallocWrapper::~GrallocWrapper() {
    for (auto bufferHandle : mAllocatedBuffers) {
        mGrallocHal->unlock(bufferHandle);
        mGrallocHal->freeBuffer(bufferHandle);
    }
    mAllocatedBuffers.clear();
}

std::string GrallocWrapper::dumpDebugInfo() {
    return mGrallocHal->dumpDebugInfo();
}

std::pair<native_handle_t*, void*> GrallocWrapper::allocate(uint32_t size) {
    native_handle_t* bufferHandle = mGrallocHal->allocate(size);
    void* buffer = nullptr;
    if (bufferHandle) {
        buffer = mGrallocHal->lock(bufferHandle);
        if (buffer) {
            mAllocatedBuffers.insert(bufferHandle);
        } else {
            mGrallocHal->freeBuffer(bufferHandle);
            bufferHandle = nullptr;
        }
    }
    return std::make_pair<>(bufferHandle, buffer);
}

void GrallocWrapper::freeBuffer(native_handle_t* bufferHandle) {
    if (mAllocatedBuffers.erase(bufferHandle)) {
        mGrallocHal->unlock(bufferHandle);
        mGrallocHal->freeBuffer(bufferHandle);
    }
}

}  // namespace android
