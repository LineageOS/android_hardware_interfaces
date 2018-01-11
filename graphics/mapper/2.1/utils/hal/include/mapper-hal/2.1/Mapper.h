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

#ifndef LOG_TAG
#warning "Mapper.h included without LOG_TAG"
#endif

#include <android/hardware/graphics/mapper/2.1/IMapper.h>
#include <mapper-hal/2.0/Mapper.h>
#include <mapper-hal/2.1/MapperHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_1 {
namespace hal {

namespace detail {

// MapperImpl implements V2_*::IMapper on top of V2_*::hal::MapperHal
template <typename Interface, typename Hal>
class MapperImpl : public V2_0::hal::detail::MapperImpl<Interface, Hal> {
   public:
    // IMapper 2.1 interface
    Return<Error> validateBufferSize(void* buffer,
                                     const IMapper::BufferDescriptorInfo& descriptorInfo,
                                     uint32_t stride) {
        const native_handle_t* bufferHandle = getImportedBuffer(buffer);
        if (!bufferHandle) {
            return Error::BAD_BUFFER;
        }

        return mHal->validateBufferSize(bufferHandle, descriptorInfo, stride);
    }

    Return<void> getTransportSize(void* buffer, IMapper::getTransportSize_cb hidl_cb) {
        const native_handle_t* bufferHandle = getImportedBuffer(buffer);
        if (!bufferHandle) {
            hidl_cb(Error::BAD_BUFFER, 0, 0);
            return Void();
        }

        uint32_t numFds = 0;
        uint32_t numInts = 0;
        Error error = mHal->getTransportSize(bufferHandle, &numFds, &numInts);
        hidl_cb(error, numFds, numInts);
        return Void();
    }

    Return<void> createDescriptor_2_1(const IMapper::BufferDescriptorInfo& descriptorInfo,
                                      IMapper::createDescriptor_2_1_cb hidl_cb) override {
        BufferDescriptor descriptor;
        Error error = mHal->createDescriptor_2_1(descriptorInfo, &descriptor);
        hidl_cb(error, descriptor);
        return Void();
    }

   private:
    using BaseType2_0 = V2_0::hal::detail::MapperImpl<Interface, Hal>;
    using BaseType2_0::getImportedBuffer;
    using BaseType2_0::mHal;
};

}  // namespace detail

using Mapper = detail::MapperImpl<IMapper, MapperHal>;

}  // namespace hal
}  // namespace V2_1
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
