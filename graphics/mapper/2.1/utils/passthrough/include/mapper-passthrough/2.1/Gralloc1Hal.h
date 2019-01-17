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
using android::hardware::graphics::common::V1_1::BufferUsage;

namespace detail {

// Gralloc1HalImpl implements V2_*::hal::MapperHal on top of gralloc1
template <typename Hal>
class Gralloc1HalImpl : public V2_0::passthrough::detail::Gralloc1HalImpl<Hal> {
   public:
    Error validateBufferSize(const native_handle_t* bufferHandle,
                             const IMapper::BufferDescriptorInfo& descriptorInfo,
                             uint32_t stride) override {
        gralloc1_buffer_descriptor_info_t bufferDescriptorInfo;

        bufferDescriptorInfo.width = descriptorInfo.width;
        bufferDescriptorInfo.height = descriptorInfo.height;
        bufferDescriptorInfo.layerCount = descriptorInfo.layerCount;
        bufferDescriptorInfo.format = static_cast<android_pixel_format_t>(descriptorInfo.format);
        bufferDescriptorInfo.consumerUsage = toConsumerUsage(descriptorInfo.usage);
        bufferDescriptorInfo.producerUsage = toProducerUsage(descriptorInfo.usage);

        int32_t error =
            mDispatch.validateBufferSize(mDevice, bufferHandle, &bufferDescriptorInfo, stride);
        if (error != GRALLOC1_ERROR_NONE) {
            return toError(error);
        }

        return Error::NONE;
    }

    Error getTransportSize(const native_handle_t* bufferHandle, uint32_t* outNumFds,
                           uint32_t* outNumInts) override {
        int32_t error = mDispatch.getTransportSize(mDevice, bufferHandle, outNumFds, outNumInts);
        return toError(error);
    }

    Error importBuffer(const native_handle_t* rawHandle,
                       native_handle_t** outBufferHandle) override {
        int32_t error = mDispatch.importBuffer(
            mDevice, rawHandle, const_cast<const native_handle_t**>(outBufferHandle));
        return toError(error);
    }

    Error createDescriptor_2_1(const IMapper::BufferDescriptorInfo& descriptorInfo,
                               BufferDescriptor* outDescriptor) override {
        if (gralloc1UsageUnsupported(descriptorInfo.usage))
             return Error::BAD_DESCRIPTOR;
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

        if (!initDispatch(GRALLOC1_FUNCTION_VALIDATE_BUFFER_SIZE, &mDispatch.validateBufferSize) ||
            !initDispatch(GRALLOC1_FUNCTION_GET_TRANSPORT_SIZE, &mDispatch.getTransportSize) ||
            !initDispatch(GRALLOC1_FUNCTION_IMPORT_BUFFER, &mDispatch.importBuffer)) {
            return false;
        }

        return true;
    }

    struct {
        GRALLOC1_PFN_VALIDATE_BUFFER_SIZE validateBufferSize;
        GRALLOC1_PFN_GET_TRANSPORT_SIZE getTransportSize;
        GRALLOC1_PFN_IMPORT_BUFFER importBuffer;
    } mDispatch = {};

    static uint64_t toProducerUsage(uint64_t usage) {
        // this is potentially broken as we have no idea which private flags
        // should be filtered out
        uint64_t producerUsage = usage & ~static_cast<uint64_t>(BufferUsage::CPU_READ_MASK |
                                                                BufferUsage::CPU_WRITE_MASK |
                                                                BufferUsage::GPU_DATA_BUFFER);

        switch (usage & BufferUsage::CPU_WRITE_MASK) {
            case static_cast<uint64_t>(BufferUsage::CPU_WRITE_RARELY):
                producerUsage |= GRALLOC1_PRODUCER_USAGE_CPU_WRITE;
                break;
            case static_cast<uint64_t>(BufferUsage::CPU_WRITE_OFTEN):
                producerUsage |= GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN;
                break;
            default:
                break;
        }

        switch (usage & BufferUsage::CPU_READ_MASK) {
            case static_cast<uint64_t>(BufferUsage::CPU_READ_RARELY):
                producerUsage |= GRALLOC1_PRODUCER_USAGE_CPU_READ;
                break;
            case static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN):
                producerUsage |= GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN;
                break;
            default:
                break;
        }

        // BufferUsage::GPU_DATA_BUFFER is always filtered out

        return producerUsage;
    }

    static uint64_t toConsumerUsage(uint64_t usage) {
        // this is potentially broken as we have no idea which private flags
        // should be filtered out
        uint64_t consumerUsage =
            usage &
            ~static_cast<uint64_t>(BufferUsage::CPU_READ_MASK | BufferUsage::CPU_WRITE_MASK |
                                   BufferUsage::SENSOR_DIRECT_DATA | BufferUsage::GPU_DATA_BUFFER);

        switch (usage & BufferUsage::CPU_READ_MASK) {
            case static_cast<uint64_t>(BufferUsage::CPU_READ_RARELY):
                consumerUsage |= GRALLOC1_CONSUMER_USAGE_CPU_READ;
                break;
            case static_cast<uint64_t>(BufferUsage::CPU_READ_OFTEN):
                consumerUsage |= GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN;
                break;
            default:
                break;
        }

        // BufferUsage::SENSOR_DIRECT_DATA is always filtered out

        if (usage & BufferUsage::GPU_DATA_BUFFER) {
            consumerUsage |= GRALLOC1_CONSUMER_USAGE_GPU_DATA_BUFFER;
        }

        return consumerUsage;
    }

    static bool gralloc1UsageUnsupported(uint64_t usage) {
        // Certain newer public usage bits should not be used with gralloc1.
        // We use a blacklist instead of a whitelist here in order to avoid
        // breaking private usage flags.
        constexpr uint64_t unsupportedMask = BufferUsage::GPU_CUBE_MAP |
                                             BufferUsage::GPU_MIPMAP_COMPLETE;

        return usage & unsupportedMask;
    }

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
