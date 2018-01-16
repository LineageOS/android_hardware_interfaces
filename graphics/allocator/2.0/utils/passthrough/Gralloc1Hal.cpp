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

#include <allocator-passthrough/2.0/Gralloc1Hal.h>

#include <string.h>

#include <GrallocBufferDescriptor.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace passthrough {

using android::hardware::graphics::common::V1_0::BufferUsage;
using mapper::V2_0::implementation::grallocDecodeBufferDescriptor;

Gralloc1Hal::~Gralloc1Hal() {
    if (mDevice) {
        gralloc1_close(mDevice);
    }
}

bool Gralloc1Hal::initWithModule(const hw_module_t* module) {
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

void Gralloc1Hal::initCapabilities() {
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

gralloc1_function_pointer_t Gralloc1Hal::getDispatchFunction(
    gralloc1_function_descriptor_t desc) const {
    auto pfn = mDevice->getFunction(mDevice, desc);
    if (!pfn) {
        ALOGE("failed to get gralloc1 function %d", desc);
        return nullptr;
    }
    return pfn;
}

bool Gralloc1Hal::initDispatch() {
    if (!initDispatchFunction(GRALLOC1_FUNCTION_DUMP, &mDispatch.dump) ||
        !initDispatchFunction(GRALLOC1_FUNCTION_CREATE_DESCRIPTOR, &mDispatch.createDescriptor) ||
        !initDispatchFunction(GRALLOC1_FUNCTION_DESTROY_DESCRIPTOR, &mDispatch.destroyDescriptor) ||
        !initDispatchFunction(GRALLOC1_FUNCTION_SET_DIMENSIONS, &mDispatch.setDimensions) ||
        !initDispatchFunction(GRALLOC1_FUNCTION_SET_FORMAT, &mDispatch.setFormat) ||
        !initDispatchFunction(GRALLOC1_FUNCTION_SET_CONSUMER_USAGE, &mDispatch.setConsumerUsage) ||
        !initDispatchFunction(GRALLOC1_FUNCTION_SET_PRODUCER_USAGE, &mDispatch.setProducerUsage) ||
        !initDispatchFunction(GRALLOC1_FUNCTION_GET_STRIDE, &mDispatch.getStride) ||
        !initDispatchFunction(GRALLOC1_FUNCTION_ALLOCATE, &mDispatch.allocate) ||
        !initDispatchFunction(GRALLOC1_FUNCTION_RELEASE, &mDispatch.release)) {
        return false;
    }

    if (mCapabilities.layeredBuffers) {
        if (!initDispatchFunction(GRALLOC1_FUNCTION_SET_LAYER_COUNT, &mDispatch.setLayerCount)) {
            return false;
        }
    }

    return true;
}

std::string Gralloc1Hal::dumpDebugInfo() {
    uint32_t len = 0;
    mDispatch.dump(mDevice, &len, nullptr);

    std::vector<char> buf(len + 1);
    mDispatch.dump(mDevice, &len, buf.data());
    buf.resize(len + 1);
    buf[len] = '\0';

    return buf.data();
}

Error Gralloc1Hal::allocateBuffers(const BufferDescriptor& descriptor, uint32_t count,
                                   uint32_t* outStride,
                                   std::vector<const native_handle_t*>* outBuffers) {
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

void Gralloc1Hal::freeBuffers(const std::vector<const native_handle_t*>& buffers) {
    for (auto buffer : buffers) {
        int32_t error = mDispatch.release(mDevice, buffer);
        if (error != GRALLOC1_ERROR_NONE) {
            ALOGE("failed to free buffer %p: %d", buffer, error);
        }
    }
}

Error Gralloc1Hal::toError(int32_t error) {
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

uint64_t Gralloc1Hal::toProducerUsage(uint64_t usage) {
    // this is potentially broken as we have no idea which private flags
    // should be filtered out
    uint64_t producerUsage =
        usage & ~static_cast<uint64_t>(BufferUsage::CPU_READ_MASK | BufferUsage::CPU_WRITE_MASK |
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

uint64_t Gralloc1Hal::toConsumerUsage(uint64_t usage) {
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

Error Gralloc1Hal::createDescriptor(const mapper::V2_0::IMapper::BufferDescriptorInfo& info,
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

Error Gralloc1Hal::allocateOneBuffer(gralloc1_buffer_descriptor_t descriptor,
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

}  // namespace passthrough
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
