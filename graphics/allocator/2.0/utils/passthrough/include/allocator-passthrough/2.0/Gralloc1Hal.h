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
#include <hardware/gralloc1.h>

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace passthrough {

using mapper::V2_0::BufferDescriptor;
using mapper::V2_0::Error;

class Gralloc1Hal : public virtual hal::AllocatorHal {
   public:
    ~Gralloc1Hal();
    bool initWithModule(const hw_module_t* module);

    std::string dumpDebugInfo() override;

    Error allocateBuffers(const BufferDescriptor& descriptor, uint32_t count, uint32_t* outStride,
                          std::vector<const native_handle_t*>* outBuffers) override;

    void freeBuffers(const std::vector<const native_handle_t*>& buffers) override;

   protected:
    template <typename T>
    bool initDispatchFunction(gralloc1_function_descriptor_t desc, T* outPfn) {
        auto pfn = getDispatchFunction(desc);
        if (pfn) {
            *outPfn = reinterpret_cast<T>(pfn);
            return true;
        } else {
            return false;
        }
    }
    gralloc1_function_pointer_t getDispatchFunction(gralloc1_function_descriptor_t desc) const;

    virtual void initCapabilities();
    virtual bool initDispatch();

    static Error toError(int32_t error);
    static uint64_t toProducerUsage(uint64_t usage);
    static uint64_t toConsumerUsage(uint64_t usage);

    Error createDescriptor(const mapper::V2_0::IMapper::BufferDescriptorInfo& info,
                           gralloc1_buffer_descriptor_t* outDescriptor);

    Error allocateOneBuffer(gralloc1_buffer_descriptor_t descriptor,
                            const native_handle_t** outBuffer, uint32_t* outStride);

    gralloc1_device_t* mDevice = nullptr;

    struct {
        bool layeredBuffers;
    } mCapabilities = {};

    struct {
        GRALLOC1_PFN_DUMP dump;
        GRALLOC1_PFN_CREATE_DESCRIPTOR createDescriptor;
        GRALLOC1_PFN_DESTROY_DESCRIPTOR destroyDescriptor;
        GRALLOC1_PFN_SET_DIMENSIONS setDimensions;
        GRALLOC1_PFN_SET_FORMAT setFormat;
        GRALLOC1_PFN_SET_LAYER_COUNT setLayerCount;
        GRALLOC1_PFN_SET_CONSUMER_USAGE setConsumerUsage;
        GRALLOC1_PFN_SET_PRODUCER_USAGE setProducerUsage;
        GRALLOC1_PFN_GET_STRIDE getStride;
        GRALLOC1_PFN_ALLOCATE allocate;
        GRALLOC1_PFN_RELEASE release;
    } mDispatch = {};
};

}  // namespace passthrough
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
