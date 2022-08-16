/*
 * Copyright 2019 The Android Open Source Project
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

#include <string>
#include <unordered_set>
#include <vector>

#include <aidl/android/hardware/graphics/allocator/AllocationError.h>
#include <aidl/android/hardware/graphics/allocator/IAllocator.h>
#include <android/hardware/graphics/allocator/4.0/IAllocator.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <gtest/gtest.h>
#include <utils/StrongPointer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V4_0 {
namespace vts {

using android::hardware::graphics::allocator::V4_0::IAllocator;

// A wrapper to IAllocator and IMapper.
class Gralloc {
  public:
    enum class Tolerance : uint32_t {
        kToleranceStrict = 0x0U,
        kToleranceBadDescriptor = 0x1U << std::underlying_type_t<Error>(Error::BAD_DESCRIPTOR),
        kToleranceBadBuffer = 0x1U << std::underlying_type_t<Error>(Error::BAD_BUFFER),
        kToleranceBadValue = 0x1U << std::underlying_type_t<Error>(Error::BAD_VALUE),
        kToleranceNoResource = 0x1U << std::underlying_type_t<Error>(Error::NO_RESOURCES),
        kToleranceUnSupported = 0x1U << std::underlying_type_t<Error>(Error::UNSUPPORTED),
        kToleranceAllErrors = ~0x0U,
    };

    Gralloc(const std::string& aidlAllocatorServiceName =
                    "android.hardware.graphics.allocator.IAllocator/default",
            const std::string& hidlAllocatorServiceName = "default",
            const std::string& mapperServiceName = "default", bool errOnFailure = true);
    ~Gralloc();

    static Error toHidlError(aidl::android::hardware::graphics::allocator::AllocationError error) {
        switch (error) {
            case aidl::android::hardware::graphics::allocator::AllocationError::BAD_DESCRIPTOR:
                return Error::BAD_DESCRIPTOR;
            case aidl::android::hardware::graphics::allocator::AllocationError::NO_RESOURCES:
                return Error::NO_RESOURCES;
            case aidl::android::hardware::graphics::allocator::AllocationError::UNSUPPORTED:
                return Error::UNSUPPORTED;
        }
    }
    static Error toHidlError(const ndk::ScopedAStatus& status) {
        if (status.isOk()) {
            return Error::NONE;
        }

        if (status.getExceptionCode() != EX_SERVICE_SPECIFIC) {
            return Error::NO_RESOURCES;
        }

        return toHidlError(
                static_cast<aidl::android::hardware::graphics::allocator::AllocationError>(
                        status.getServiceSpecificError()));
    }

    // IAllocator methods

    bool hasAllocator() { return mHidlAllocator != nullptr || mAidlAllocator != nullptr; }

    // When import is false, this simply calls IAllocator::allocate. When import
    // is true, the returned buffers are also imported into the mapper.
    //
    // Either case, the returned buffers must be freed with freeBuffer.
    std::vector<const native_handle_t*> allocate(
            const BufferDescriptor& descriptor, uint32_t count, bool import = true,
            enum Tolerance tolerance = Tolerance::kToleranceStrict, uint32_t* outStride = nullptr);

    const native_handle_t* allocate(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                    bool import, enum Tolerance tolerance, uint32_t* outStride);

    const native_handle_t* allocate(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                    bool import) {
        return allocate(descriptorInfo, import, Tolerance::kToleranceStrict);
    }

    const native_handle_t* allocate(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                    bool import, enum Tolerance tolerance) {
        return allocate(descriptorInfo, import, tolerance, nullptr);
    }

    const native_handle_t* allocate(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                    bool import, uint32_t* outStride) {
        return allocate(descriptorInfo, import, Tolerance::kToleranceStrict, outStride);
    }

    // Dispatches directly to the allocator
    void rawAllocate(const BufferDescriptor& descriptor, uint32_t count,
                     std::function<void(Error, uint32_t, const hidl_vec<hidl_handle>&)> callback);

    // IMapper methods

    sp<IMapper> getMapper() const;

    BufferDescriptor createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo);

    const native_handle_t* importBuffer(const hidl_handle& rawHandle, enum Tolerance tolerance);
    const native_handle_t* importBuffer(const hidl_handle& rawHandle) {
        return importBuffer(rawHandle, Tolerance::kToleranceStrict);
    }

    void freeBuffer(const native_handle_t* bufferHandle);

    // We use fd instead of hidl_handle in these functions to pass fences
    // in and out of the mapper.  The ownership of the fd is always transferred
    // with each of these functions.
    void* lock(const native_handle_t* bufferHandle, uint64_t cpuUsage,
               const IMapper::Rect& accessRegion, int acquireFence);
    int unlock(const native_handle_t* bufferHandle);

    int flushLockedBuffer(const native_handle_t* bufferHandle);
    void rereadLockedBuffer(const native_handle_t* bufferHandle);

    bool validateBufferSize(const native_handle_t* bufferHandle,
                            const IMapper::BufferDescriptorInfo& descriptorInfo, uint32_t stride);
    void getTransportSize(const native_handle_t* bufferHandle, uint32_t* outNumFds,
                          uint32_t* outNumInts);

    bool isSupported(const IMapper::BufferDescriptorInfo& descriptorInfo);

    // A version of isSupported that simply treats failure as no support, so it
    // does not fail the test.
    bool isSupportedNoFailure(const IMapper::BufferDescriptorInfo& descriptorInfo);

    Error get(const native_handle_t* bufferHandle, const IMapper::MetadataType& metadataType,
              hidl_vec<uint8_t>* outVec);

    Error set(const native_handle_t* bufferHandle, const IMapper::MetadataType& metadataType,
              const hidl_vec<uint8_t>& vec);

    Error getFromBufferDescriptorInfo(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                      const IMapper::MetadataType& metadataType,
                                      hidl_vec<uint8_t>* outVec);

    Error getReservedRegion(const native_handle_t* bufferHandle, void** outReservedRegion,
                            uint64_t* outReservedSize);

  private:
    bool canTolerate(Tolerance tolerance, Error error) {
        return (std::underlying_type_t<Tolerance>(tolerance) &
                0x1U << std::underlying_type_t<Error>(error)) != 0;
    }

    void init(const std::string& aidlAllocatorServiceName,
              const std::string& hidlAllocatorServiceName, const std::string& mapperServiceName);

    // initialize without checking for failure to get service
    void initNoErr(const std::string& aidlAllocatorServiceName,
                   const std::string& hidlAllocatorServiceName,
                   const std::string& mapperServiceName);
    const native_handle_t* cloneBuffer(const hidl_handle& rawHandle, enum Tolerance tolerance);
    const native_handle_t* cloneBuffer(const hidl_handle& rawHandle) {
        return cloneBuffer(rawHandle, Tolerance::kToleranceStrict);
    }

    sp<IAllocator> mHidlAllocator;
    std::shared_ptr<aidl::android::hardware::graphics::allocator::IAllocator> mAidlAllocator;
    sp<IMapper> mMapper;

    // Keep track of all cloned and imported handles.  When a test fails with
    // ASSERT_*, the destructor will free the handles for the test.
    std::unordered_set<const native_handle_t*> mClonedBuffers;
    std::unordered_set<const native_handle_t*> mImportedBuffers;
};

}  // namespace vts
}  // namespace V4_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
