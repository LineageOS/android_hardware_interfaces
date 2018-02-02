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
#warning "Allocator.h included without LOG_TAG"
#endif

#include <memory>

#include <allocator-hal/2.0/AllocatorHal.h>
#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace hal {

using mapper::V2_0::BufferDescriptor;
using mapper::V2_0::Error;

namespace detail {

// AllocatorImpl implements V2_*::IAllocator on top of V2_*::hal::AllocatorHal
template <typename Interface, typename Hal>
class AllocatorImpl : public Interface {
   public:
    bool init(std::unique_ptr<Hal> hal) {
        mHal = std::move(hal);
        return true;
    }

    // IAllocator 2.0 interface
    Return<void> dumpDebugInfo(IAllocator::dumpDebugInfo_cb hidl_cb) override {
        hidl_cb(mHal->dumpDebugInfo());
        return Void();
    }

    Return<void> allocate(const BufferDescriptor& descriptor, uint32_t count,
                          IAllocator::allocate_cb hidl_cb) override {
        uint32_t stride;
        std::vector<const native_handle_t*> buffers;
        Error error = mHal->allocateBuffers(descriptor, count, &stride, &buffers);
        if (error != Error::NONE) {
            hidl_cb(error, 0, hidl_vec<hidl_handle>());
            return Void();
        }

        hidl_vec<hidl_handle> hidlBuffers(buffers.cbegin(), buffers.cend());
        hidl_cb(Error::NONE, stride, hidlBuffers);

        // free the local handles
        mHal->freeBuffers(buffers);

        return Void();
    }

   protected:
    std::unique_ptr<Hal> mHal;
};

}  // namespace detail

using Allocator = detail::AllocatorImpl<IAllocator, AllocatorHal>;

}  // namespace hal
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
