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

#include <string>
#include <vector>

#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace hal {

using mapper::V2_0::BufferDescriptor;
using mapper::V2_0::Error;

class AllocatorHal {
   public:
    virtual ~AllocatorHal() = default;

    // dump the debug information
    virtual std::string dumpDebugInfo() = 0;

    // allocate buffers
    virtual Error allocateBuffers(const BufferDescriptor& descriptor, uint32_t count,
                                  uint32_t* outStride,
                                  std::vector<const native_handle_t*>* outBuffers) = 0;

    // free buffers
    virtual void freeBuffers(const std::vector<const native_handle_t*>& buffers) = 0;
};

}  // namespace hal
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
