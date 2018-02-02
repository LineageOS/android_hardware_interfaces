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

#ifndef LOG_TAG
#warning "Gralloc1Hal.h included without LOG_TAG"
#endif

#include <cstring>  // for strerror

#include <allocator-hal/2.0/AllocatorHal.h>
#include <hardware/gralloc1.h>
#include <log/log.h>
#include <mapper-passthrough/2.0/GrallocBufferDescriptor.h>

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace passthrough {

namespace detail {

using common::V1_0::BufferUsage;
using mapper::V2_0::BufferDescriptor;
using mapper::V2_0::Error;
using mapper::V2_0::passthrough::grallocDecodeBufferDescriptor;

// Gralloc1HalImpl implements V2_*::hal::AllocatorHal on top of gralloc1
template <typename Hal>
class Gralloc1HalImpl : public Hal {
   public:
    ~Gralloc1HalImpl() {
        if (mDevice) {
            gralloc1_close(mDevice);
        }
    }

    bool initWithModule(const hw_module_t* module) {
        int result = gralloc1_open(module, &mDevice);
        if (result) {
            ALOGE("failed to open gralloc1 device: %s", strerror(-result));
            mDevice = nullptr;
            return false;
        }

        initCapabilities();
        if (!initDispatch()) {
            gralloc1_close(mDevice);
            mDevice = nullptr;
            return false;
        }

        return true;
    }

    std::string dumpDebugInfo() override {
        uint32_t len = 0;
        mDispatch.dump(mDevice, &len, nullptr);

        std::vector<char> buf(len + 1);
        mDispatch.dump(mDevice, &len, buf.data());
        buf.resize(len + 1);
        buf[len] = '\0';

        return buf.data();
    }

    Error allocateBuffers(const BufferDescriptor& descriptor, uint32_t count, uint32_t* outStride,
                          std::vector<const native_handle_t*>* outBuffers) override {
        mapper::V2_0::IMapper::BufferDescriptorInfo descriptorInfo;
        if (!grallocDecodeBufferDescriptor(descriptor, &descriptorInfo)) {
            return Error::BAD_DESCRIPTOR;
        }

        gralloc1_buffer_descriptor_t desc;
        Error error = createDescriptor(descriptorInfo, &desc);
        if (error != Error::NONE) {
            return error;
        }

        uint32_t stride = 0;
        std::vector<const native_handle_t*> buffers;
        buffers.reserve(count);

        // allocate the buffers
        for (uint32_t i = 0; i < count; i++) {
            const native_handle_t* tmpBuffer;
            uint32_t tmpStride;
            error = allocateOneBuffer(desc, &tmpBuffer, &tmpStride);
            if (error != Error::NONE) {
                break;
            }

            buffers.push_back(tmpBuffer);

            if (stride == 0) {
                stride = tmpStride;
            } else if (stride != tmpStride) {
                // non-uniform strides
                error = Error::UNSUPPORTED;
                break;
            }
        }

        mDispatch.destroyDescriptor(mDevice, desc);

        if (error != Error::NONE) {
            freeBuffers(buffers);
            return error;
        }

        *outStride = stride;
        *outBuffers = std::move(buffers);

        return Error::NONE;
    }

    void freeBuffers(const std::vector<const native_handle_t*>& buffers) override {
        for (auto buffer : buffers) {
            int32_t error = mDispatch.release(mDevice, buffer);
            if (error != GRALLOC1_ERROR_NONE) {
                ALOGE("failed to free buffer %p: %d", buffer, error);
            }
        }
    }

   protected:
    virtual void initCapabilities() {
        uint32_t count = 0;
        mDevice->getCapabilities(mDevice, &count, nullptr);

        std::vector<int32_t> capabilities(count);
        mDevice->getCapabilities(mDevice, &count, capabilities.data());
        capabilities.resize(count);

        for (auto capability : capabilities) {
            if (capability == GRALLOC1_CAPABILITY_LAYERED_BUFFERS) {
                mCapabilities.layeredBuffers = true;
                break;
            }
        }
    }

    template <typename T>
    bool initDispatch(gralloc1_function_descriptor_t desc, T* outPfn) {
        auto pfn = mDevice->getFunction(mDevice, desc);
        if (pfn) {
            *outPfn = reinterpret_cast<T>(pfn);
            return true;
        } else {
            ALOGE("failed to get gralloc1 function %d", desc);
            return false;
        }
    }

    virtual bool initDispatch() {
        if (!initDispatch(GRALLOC1_FUNCTION_DUMP, &mDispatch.dump) ||
            !initDispatch(GRALLOC1_FUNCTION_CREATE_DESCRIPTOR, &mDispatch.createDescriptor) ||
            !initDispatch(GRALLOC1_FUNCTION_DESTROY_DESCRIPTOR, &mDispatch.destroyDescriptor) ||
            !initDispatch(GRALLOC1_FUNCTION_SET_DIMENSIONS, &mDispatch.setDimensions) ||
            !initDispatch(GRALLOC1_FUNCTION_SET_FORMAT, &mDispatch.setFormat) ||
            !initDispatch(GRALLOC1_FUNCTION_SET_CONSUMER_USAGE, &mDispatch.setConsumerUsage) ||
            !initDispatch(GRALLOC1_FUNCTION_SET_PRODUCER_USAGE, &mDispatch.setProducerUsage) ||
            !initDispatch(GRALLOC1_FUNCTION_GET_STRIDE, &mDispatch.getStride) ||
            !initDispatch(GRALLOC1_FUNCTION_ALLOCATE, &mDispatch.allocate) ||
            !initDispatch(GRALLOC1_FUNCTION_RELEASE, &mDispatch.release)) {
            return false;
        }

        if (mCapabilities.layeredBuffers) {
            if (!initDispatch(GRALLOC1_FUNCTION_SET_LAYER_COUNT, &mDispatch.setLayerCount)) {
                return false;
            }
        }

        return true;
    }

    static Error toError(int32_t error) {
        switch (error) {
            case GRALLOC1_ERROR_NONE:
                return Error::NONE;
            case GRALLOC1_ERROR_BAD_DESCRIPTOR:
                return Error::BAD_DESCRIPTOR;
            case GRALLOC1_ERROR_BAD_HANDLE:
                return Error::BAD_BUFFER;
            case GRALLOC1_ERROR_BAD_VALUE:
                return Error::BAD_VALUE;
            case GRALLOC1_ERROR_NOT_SHARED:
                return Error::NONE;  // this is fine
            case GRALLOC1_ERROR_NO_RESOURCES:
                return Error::NO_RESOURCES;
            case GRALLOC1_ERROR_UNDEFINED:
            case GRALLOC1_ERROR_UNSUPPORTED:
            default:
                return Error::UNSUPPORTED;
        }
    }

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

    Error createDescriptor(const mapper::V2_0::IMapper::BufferDescriptorInfo& info,
                           gralloc1_buffer_descriptor_t* outDescriptor) {
        gralloc1_buffer_descriptor_t descriptor;

        int32_t error = mDispatch.createDescriptor(mDevice, &descriptor);

        if (error == GRALLOC1_ERROR_NONE) {
            error = mDispatch.setDimensions(mDevice, descriptor, info.width, info.height);
        }
        if (error == GRALLOC1_ERROR_NONE) {
            error = mDispatch.setFormat(mDevice, descriptor, static_cast<int32_t>(info.format));
        }
        if (error == GRALLOC1_ERROR_NONE) {
            if (mCapabilities.layeredBuffers) {
                error = mDispatch.setLayerCount(mDevice, descriptor, info.layerCount);
            } else if (info.layerCount > 1) {
                error = GRALLOC1_ERROR_UNSUPPORTED;
            }
        }
        if (error == GRALLOC1_ERROR_NONE) {
            error = mDispatch.setProducerUsage(mDevice, descriptor, toProducerUsage(info.usage));
        }
        if (error == GRALLOC1_ERROR_NONE) {
            error = mDispatch.setConsumerUsage(mDevice, descriptor, toConsumerUsage(info.usage));
        }

        if (error == GRALLOC1_ERROR_NONE) {
            *outDescriptor = descriptor;
        } else {
            mDispatch.destroyDescriptor(mDevice, descriptor);
        }

        return toError(error);
    }

    Error allocateOneBuffer(gralloc1_buffer_descriptor_t descriptor,
                            const native_handle_t** outBuffer, uint32_t* outStride) {
        const native_handle_t* buffer = nullptr;
        int32_t error = mDispatch.allocate(mDevice, 1, &descriptor, &buffer);
        if (error != GRALLOC1_ERROR_NONE && error != GRALLOC1_ERROR_NOT_SHARED) {
            return toError(error);
        }

        uint32_t stride = 0;
        error = mDispatch.getStride(mDevice, buffer, &stride);
        if (error != GRALLOC1_ERROR_NONE && error != GRALLOC1_ERROR_UNDEFINED) {
            mDispatch.release(mDevice, buffer);
            return toError(error);
        }

        *outBuffer = buffer;
        *outStride = stride;

        return Error::NONE;
    }

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

}  // namespace detail

using Gralloc1Hal = detail::Gralloc1HalImpl<hal::AllocatorHal>;

}  // namespace passthrough
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
