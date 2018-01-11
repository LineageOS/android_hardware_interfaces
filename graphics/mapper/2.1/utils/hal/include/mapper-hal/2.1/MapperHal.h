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

#include <android/hardware/graphics/mapper/2.1/IMapper.h>
#include <mapper-hal/2.0/MapperHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_1 {
namespace hal {

using V2_0::BufferDescriptor;
using V2_0::Error;

class MapperHal : public V2_0::hal::MapperHal {
   public:
    virtual ~MapperHal() = default;

    // superceded by createDescriptor_2_1
    Error createDescriptor(const V2_0::IMapper::BufferDescriptorInfo& descriptorInfo,
                           BufferDescriptor* outDescriptor) override {
        return createDescriptor_2_1(
            IMapper::BufferDescriptorInfo{
                descriptorInfo.width, descriptorInfo.height, descriptorInfo.layerCount,
                static_cast<common::V1_1::PixelFormat>(descriptorInfo.format), descriptorInfo.usage,
            },
            outDescriptor);
    }

    // validate the buffer can be safely accessed with the specified
    // descriptorInfo and stride
    virtual Error validateBufferSize(const native_handle_t* bufferHandle,
                                     const IMapper::BufferDescriptorInfo& descriptorInfo,
                                     uint32_t stride) = 0;

    // get the transport size of a buffer handle.  It can be smaller than or
    // equal to the size of the buffer handle.
    virtual Error getTransportSize(const native_handle_t* bufferHandle, uint32_t* outNumFds,
                                   uint32_t* outNumInts) = 0;

    // create a BufferDescriptor
    virtual Error createDescriptor_2_1(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                       BufferDescriptor* outDescriptor) = 0;
};

}  // namespace hal
}  // namespace V2_1
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
