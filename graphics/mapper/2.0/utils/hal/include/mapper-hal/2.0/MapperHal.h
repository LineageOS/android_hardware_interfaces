/*
 * Copyright 2018 The Android Open Source Project
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

#include <android-base/unique_fd.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace hal {

class MapperHal {
   public:
    virtual ~MapperHal() = default;

    // create a BufferDescriptor
    virtual Error createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                   BufferDescriptor* outDescriptor) = 0;

    // import a raw handle owned by the caller
    virtual Error importBuffer(const native_handle_t* rawHandle,
                               native_handle_t** outBufferHandle) = 0;

    // free an imported buffer handle
    virtual Error freeBuffer(native_handle_t* bufferHandle) = 0;

    // lock a buffer
    virtual Error lock(const native_handle_t* bufferHandle, uint64_t cpuUsage,
                       const IMapper::Rect& accessRegion, base::unique_fd fenceFd,
                       void** outData) = 0;

    // lock a YCbCr buffer
    virtual Error lockYCbCr(const native_handle_t* bufferHandle, uint64_t cpuUsage,
                            const IMapper::Rect& accessRegion, base::unique_fd fenceFd,
                            YCbCrLayout* outLayout) = 0;

    // unlock a buffer
    virtual Error unlock(const native_handle_t* bufferHandle, base::unique_fd* outFenceFd) = 0;
};

}  // namespace hal
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
