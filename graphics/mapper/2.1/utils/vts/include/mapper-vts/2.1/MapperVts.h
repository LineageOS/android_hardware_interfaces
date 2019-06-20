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

#pragma once

#include <android/hardware/graphics/mapper/2.1/IMapper.h>
#include <mapper-vts/2.0/MapperVts.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_1 {
namespace vts {

using android::hardware::graphics::allocator::V2_0::IAllocator;
using V2_0::BufferDescriptor;

// A wrapper to IAllocator and IMapper.
class Gralloc : public V2_0::vts::Gralloc {
   public:
     Gralloc(bool errOnFailure = true);
     Gralloc(const std::string& allocatorServiceName, const std::string& mapperServiceName,
             bool errOnFailure = true);

     sp<IMapper> getMapper() const;

     bool validateBufferSize(const native_handle_t* bufferHandle,
                             const IMapper::BufferDescriptorInfo& descriptorInfo, uint32_t stride);
     void getTransportSize(const native_handle_t* bufferHandle, uint32_t* outNumFds,
                           uint32_t* outNumInts);

     BufferDescriptor createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo);

     const native_handle_t* allocate(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                     bool import = true, uint32_t* outStride = nullptr);

   protected:
     void init(bool errOnFailure = true);

     sp<IMapper> mMapperV2_1;
};

}  // namespace vts
}  // namespace V2_1
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
