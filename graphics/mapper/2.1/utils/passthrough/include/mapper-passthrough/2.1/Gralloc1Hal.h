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

#include <hardware/gralloc1.h>
#include <mapper-hal/2.1/MapperHal.h>
#include <mapper-passthrough/2.0/Gralloc1Hal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_1 {
namespace passthrough {

using V2_0::BufferDescriptor;
using V2_0::Error;

namespace detail {

// Gralloc1HalImpl implements V2_*::hal::MapperHal on top of gralloc1
template <typename Hal>
class Gralloc1HalImpl : public V2_0::passthrough::detail::Gralloc1HalImpl<Hal> {
   public:
    Error validateBufferSize(const native_handle_t* bufferHandle,
                             const IMapper::BufferDescriptorInfo& descriptorInfo,
                             uint32_t stride) override {
        uint32_t bufferWidth;
        uint32_t bufferHeight;
        uint32_t bufferLayerCount;
        int32_t bufferFormat;
        uint64_t bufferProducerUsage;
        uint64_t bufferConsumerUsage;
        uint32_t bufferStride;

        int32_t error = mDispatch.getDimensions(mDevice, bufferHandle, &bufferWidth, &bufferHeight);
        if (error != GRALLOC1_ERROR_NONE) {
            return toError(error);
        }
        error = mDispatch.getLayerCount(mDevice, bufferHandle, &bufferLayerCount);
        if (error != GRALLOC1_ERROR_NONE) {
            return toError(error);
        }
        error = mDispatch.getFormat(mDevice, bufferHandle, &bufferFormat);
        if (error != GRALLOC1_ERROR_NONE) {
            return toError(error);
        }
        error = mDispatch.getProducerUsage(mDevice, bufferHandle, &bufferProducerUsage);
        if (error != GRALLOC1_ERROR_NONE) {
            return toError(error);
        }
        error = mDispatch.getConsumerUsage(mDevice, bufferHandle, &bufferConsumerUsage);
        if (error != GRALLOC1_ERROR_NONE) {
            return toError(error);
        }
        error = mDispatch.getStride(mDevice, bufferHandle, &bufferStride);
        if (error != GRALLOC1_ERROR_NONE) {
            return toError(error);
        }

        // TODO format? usage? width > stride?
        // need a gralloc1 extension to really validate
        (void)bufferFormat;
        (void)bufferProducerUsage;
        (void)bufferConsumerUsage;

        if (descriptorInfo.width > bufferWidth || descriptorInfo.height > bufferHeight ||
            descriptorInfo.layerCount > bufferLayerCount || stride > bufferStride) {
            return Error::BAD_VALUE;
        }

        return Error::NONE;
    }

    Error getTransportSize(const native_handle_t* bufferHandle, uint32_t* outNumFds,
                           uint32_t* outNumInts) override {
        // need a gralloc1 extension to get the transport size
        *outNumFds = bufferHandle->numFds;
        *outNumInts = bufferHandle->numInts;
        return Error::NONE;
    }

    Error createDescriptor_2_1(const IMapper::BufferDescriptorInfo& descriptorInfo,
                               BufferDescriptor* outDescriptor) override {
        return createDescriptor(
            V2_0::IMapper::BufferDescriptorInfo{
                descriptorInfo.width, descriptorInfo.height, descriptorInfo.layerCount,
                static_cast<common::V1_0::PixelFormat>(descriptorInfo.format), descriptorInfo.usage,
            },
            outDescriptor);
    }

   protected:
    bool initDispatch() override {
        if (!BaseType2_0::initDispatch()) {
            return false;
        }

        if (!initDispatch(GRALLOC1_FUNCTION_GET_DIMENSIONS, &mDispatch.getDimensions) ||
            !initDispatch(GRALLOC1_FUNCTION_GET_LAYER_COUNT, &mDispatch.getLayerCount) ||
            !initDispatch(GRALLOC1_FUNCTION_GET_FORMAT, &mDispatch.getFormat) ||
            !initDispatch(GRALLOC1_FUNCTION_GET_PRODUCER_USAGE, &mDispatch.getProducerUsage) ||
            !initDispatch(GRALLOC1_FUNCTION_GET_CONSUMER_USAGE, &mDispatch.getConsumerUsage) ||
            !initDispatch(GRALLOC1_FUNCTION_GET_STRIDE, &mDispatch.getStride)) {
            return false;
        }

        return true;
    }

    struct {
        GRALLOC1_PFN_GET_DIMENSIONS getDimensions;
        GRALLOC1_PFN_GET_LAYER_COUNT getLayerCount;
        GRALLOC1_PFN_GET_FORMAT getFormat;
        GRALLOC1_PFN_GET_PRODUCER_USAGE getProducerUsage;
        GRALLOC1_PFN_GET_CONSUMER_USAGE getConsumerUsage;
        GRALLOC1_PFN_GET_STRIDE getStride;
    } mDispatch = {};

   private:
    using BaseType2_0 = V2_0::passthrough::detail::Gralloc1HalImpl<Hal>;
    using BaseType2_0::createDescriptor;
    using BaseType2_0::initDispatch;
    using BaseType2_0::mDevice;
    using BaseType2_0::toError;
};

}  // namespace detail

using Gralloc1Hal = detail::Gralloc1HalImpl<hal::MapperHal>;

}  // namespace passthrough
}  // namespace V2_1
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
