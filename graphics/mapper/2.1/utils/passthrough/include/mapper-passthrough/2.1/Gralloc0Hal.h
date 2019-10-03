/*
 * Copyright 2016 The Android Open Source Project
 * * Licensed under the Apache License, Version 2.0 (the "License");
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

#include <mapper-hal/2.1/MapperHal.h>
#include <mapper-passthrough/2.0/Gralloc0Hal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_1 {
namespace passthrough {

namespace detail {

using V2_0::BufferDescriptor;
using V2_0::Error;

// Gralloc0HalImpl implements V2_*::hal::MapperHal on top of gralloc0
template <typename Hal>
class Gralloc0HalImpl : public V2_0::passthrough::detail::Gralloc0HalImpl<Hal> {
   public:
     Error validateBufferSize(const native_handle_t* bufferHandle,
                              const IMapper::BufferDescriptorInfo& descriptorInfo,
                              uint32_t stride) override {
         if (descriptorInfo.layerCount != 1) {
             return Error::BAD_VALUE;
         }

         if (!mModule->validateBufferSize) {
             return Error::NONE;
         }

         int32_t ret = mModule->validateBufferSize(
                 mModule, bufferHandle, descriptorInfo.width, descriptorInfo.height,
                 static_cast<int32_t>(descriptorInfo.format),
                 static_cast<uint64_t>(descriptorInfo.usage), stride);
         return static_cast<Error>(ret);
     }
     Error getTransportSize(const native_handle_t* bufferHandle, uint32_t* outNumFds,
                            uint32_t* outNumInts) override {
         if (!mModule->getTransportSize) {
             *outNumFds = bufferHandle->numFds;
             *outNumInts = bufferHandle->numInts;
             return Error::NONE;
         }

         int32_t ret = mModule->getTransportSize(mModule, bufferHandle, outNumFds, outNumInts);
         return static_cast<Error>(ret);
    }

    Error createDescriptor_2_1(const IMapper::BufferDescriptorInfo& descriptorInfo,
                               BufferDescriptor* outDescriptor) override {
        return createDescriptor(
                V2_0::IMapper::BufferDescriptorInfo{
                        descriptorInfo.width,
                        descriptorInfo.height,
                        descriptorInfo.layerCount,
                        static_cast<common::V1_0::PixelFormat>(descriptorInfo.format),
                        descriptorInfo.usage,
                },
                outDescriptor);
    }

   private:
    using BaseType2_0 = V2_0::passthrough::detail::Gralloc0HalImpl<Hal>;
    using BaseType2_0::createDescriptor;
    using BaseType2_0::mModule;
};

}  // namespace detail

using Gralloc0Hal = detail::Gralloc0HalImpl<hal::MapperHal>;

}  // namespace passthrough
}  // namespace V2_1
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
