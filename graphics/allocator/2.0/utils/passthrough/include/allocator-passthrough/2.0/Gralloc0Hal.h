/*
 * Copyright 2016 The Android Open Source Project
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

#include <allocator-hal/2.0/AllocatorHal.h>

struct alloc_device_t;
struct hw_module_t;

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace passthrough {

using mapper::V2_0::BufferDescriptor;
using mapper::V2_0::Error;

class Gralloc0Hal : public virtual hal::AllocatorHal {
   public:
    ~Gralloc0Hal();
    bool initWithModule(const hw_module_t* module);

    std::string dumpDebugInfo() override;

    Error allocateBuffers(const BufferDescriptor& descriptor, uint32_t count, uint32_t* outStride,
                          std::vector<const native_handle_t*>* outBuffers) override;

    void freeBuffers(const std::vector<const native_handle_t*>& buffers) override;

   protected:
    Error allocateOneBuffer(const mapper::V2_0::IMapper::BufferDescriptorInfo& info,
                            const native_handle_t** outBuffer, uint32_t* outStride);

    alloc_device_t* mDevice = nullptr;
};

}  // namespace passthrough
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
