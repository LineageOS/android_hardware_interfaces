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

#define LOG_TAG "GrallocPassthrough"

#include <type_traits>
#include <unordered_set>
#include <vector>

#include <string.h>

#include <hardware/gralloc1.h>
#include <log/log.h>

#include "Gralloc.h"

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace implementation {

class GrallocHal : public IAllocator {
public:
    GrallocHal(const hw_module_t* module);
    virtual ~GrallocHal();

    // IAllocator interface
    Return<void> getCapabilities(getCapabilities_cb hidl_cb) override;
    Return<void> dumpDebugInfo(dumpDebugInfo_cb hidl_cb) override;
    Return<void> createDescriptor(const BufferDescriptorInfo& descriptorInfo,
            createDescriptor_cb hidl_cb) override;
    Return<Error> destroyDescriptor(BufferDescriptor descriptor) override;

    Return<Error> testAllocate(
            const hidl_vec<BufferDescriptor>& descriptors) override;
    Return<void> allocate(const hidl_vec<BufferDescriptor>& descriptors,
            allocate_cb hidl_cb) override;
    Return<Error> free(Buffer buffer) override;

    Return<void> exportHandle(BufferDescriptor descriptor,
            Buffer buffer, exportHandle_cb hidl_cb) override;

private:
    void initCapabilities();

    template<typename T>
    void initDispatch(T& func, gralloc1_function_descriptor_t desc);
    void initDispatch();

    bool hasCapability(Capability capability) const;

    gralloc1_device_t* mDevice;

    std::unordered_set<Capability> mCapabilities;

    struct {
        GRALLOC1_PFN_DUMP dump;
        GRALLOC1_PFN_CREATE_DESCRIPTOR createDescriptor;
        GRALLOC1_PFN_DESTROY_DESCRIPTOR destroyDescriptor;
        GRALLOC1_PFN_SET_DIMENSIONS setDimensions;
        GRALLOC1_PFN_SET_FORMAT setFormat;
        GRALLOC1_PFN_SET_LAYER_COUNT setLayerCount;
        GRALLOC1_PFN_SET_CONSUMER_USAGE setConsumerUsage;
        GRALLOC1_PFN_SET_PRODUCER_USAGE setProducerUsage;
        GRALLOC1_PFN_ALLOCATE allocate;
        GRALLOC1_PFN_RELEASE release;
        GRALLOC1_PFN_GET_BACKING_STORE getBackingStore;
        GRALLOC1_PFN_GET_STRIDE getStride;
        GRALLOC1_PFN_GET_NUM_FLEX_PLANES getNumFlexPlanes;
    } mDispatch;
};

GrallocHal::GrallocHal(const hw_module_t* module)
    : mDevice(nullptr), mDispatch()
{
    int status = gralloc1_open(module, &mDevice);
    if (status) {
        LOG_ALWAYS_FATAL("failed to open gralloc1 device: %s",
                strerror(-status));
    }

    initCapabilities();
    initDispatch();
}

GrallocHal::~GrallocHal()
{
    gralloc1_close(mDevice);
}

void GrallocHal::initCapabilities()
{
    uint32_t count;
    mDevice->getCapabilities(mDevice, &count, nullptr);

    std::vector<Capability> caps(count);
    mDevice->getCapabilities(mDevice, &count, reinterpret_cast<
              std::underlying_type<Capability>::type*>(caps.data()));
    caps.resize(count);

    mCapabilities.insert(caps.cbegin(), caps.cend());
}

template<typename T>
void GrallocHal::initDispatch(T& func, gralloc1_function_descriptor_t desc)
{
    auto pfn = mDevice->getFunction(mDevice, desc);
    if (!pfn) {
        LOG_ALWAYS_FATAL("failed to get gralloc1 function %d", desc);
    }

    func = reinterpret_cast<T>(pfn);
}

void GrallocHal::initDispatch()
{
    initDispatch(mDispatch.dump, GRALLOC1_FUNCTION_DUMP);
    initDispatch(mDispatch.createDescriptor,
            GRALLOC1_FUNCTION_CREATE_DESCRIPTOR);
    initDispatch(mDispatch.destroyDescriptor,
            GRALLOC1_FUNCTION_DESTROY_DESCRIPTOR);
    initDispatch(mDispatch.setDimensions, GRALLOC1_FUNCTION_SET_DIMENSIONS);
    initDispatch(mDispatch.setFormat, GRALLOC1_FUNCTION_SET_FORMAT);
    if (hasCapability(Capability::LAYERED_BUFFERS)) {
        initDispatch(
                mDispatch.setLayerCount, GRALLOC1_FUNCTION_SET_LAYER_COUNT);
    }
    initDispatch(mDispatch.setConsumerUsage,
            GRALLOC1_FUNCTION_SET_CONSUMER_USAGE);
    initDispatch(mDispatch.setProducerUsage,
            GRALLOC1_FUNCTION_SET_PRODUCER_USAGE);
    initDispatch(mDispatch.allocate, GRALLOC1_FUNCTION_ALLOCATE);
    initDispatch(mDispatch.release, GRALLOC1_FUNCTION_RELEASE);
}

bool GrallocHal::hasCapability(Capability capability) const
{
    return (mCapabilities.count(capability) > 0);
}

Return<void> GrallocHal::getCapabilities(getCapabilities_cb hidl_cb)
{
    std::vector<Capability> caps(
            mCapabilities.cbegin(), mCapabilities.cend());

    hidl_vec<Capability> reply;
    reply.setToExternal(caps.data(), caps.size());
    hidl_cb(reply);

    return Void();
}

Return<void> GrallocHal::dumpDebugInfo(dumpDebugInfo_cb hidl_cb)
{
    uint32_t len = 0;
    mDispatch.dump(mDevice, &len, nullptr);

    std::vector<char> buf(len + 1);
    mDispatch.dump(mDevice, &len, buf.data());
    buf.resize(len + 1);
    buf[len] = '\0';

    hidl_string reply;
    reply.setToExternal(buf.data(), len);
    hidl_cb(reply);

    return Void();
}

Return<void> GrallocHal::createDescriptor(
        const BufferDescriptorInfo& descriptorInfo,
        createDescriptor_cb hidl_cb)
{
    BufferDescriptor descriptor;
    int32_t err = mDispatch.createDescriptor(mDevice, &descriptor);
    if (err == GRALLOC1_ERROR_NONE) {
        err = mDispatch.setDimensions(mDevice, descriptor,
                descriptorInfo.width, descriptorInfo.height);
    }
    if (err == GRALLOC1_ERROR_NONE) {
        err = mDispatch.setFormat(mDevice, descriptor,
                static_cast<int32_t>(descriptorInfo.format));
    }
    if (err == GRALLOC1_ERROR_NONE &&
            hasCapability(Capability::LAYERED_BUFFERS)) {
        err = mDispatch.setLayerCount(mDevice, descriptor,
                descriptorInfo.layerCount);
    }
    if (err == GRALLOC1_ERROR_NONE) {
        uint64_t producerUsageMask = descriptorInfo.producerUsageMask;
        if (producerUsageMask & GRALLOC1_PRODUCER_USAGE_CPU_READ_OFTEN) {
            producerUsageMask |= GRALLOC1_PRODUCER_USAGE_CPU_READ;
        }
        if (producerUsageMask & GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN) {
            producerUsageMask |= GRALLOC1_PRODUCER_USAGE_CPU_WRITE;
        }
        err = mDispatch.setProducerUsage(mDevice, descriptor,
                descriptorInfo.producerUsageMask);
    }
    if (err == GRALLOC1_ERROR_NONE) {
        uint64_t consumerUsageMask = descriptorInfo.consumerUsageMask;
        if (consumerUsageMask & GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN) {
            consumerUsageMask |= GRALLOC1_CONSUMER_USAGE_CPU_READ;
        }
        err = mDispatch.setConsumerUsage(mDevice, descriptor,
                consumerUsageMask);
    }

    hidl_cb(static_cast<Error>(err), descriptor);

    return Void();
}

Return<Error> GrallocHal::destroyDescriptor(
        BufferDescriptor descriptor)
{
    int32_t err = mDispatch.destroyDescriptor(mDevice, descriptor);
    return static_cast<Error>(err);
}

Return<Error> GrallocHal::testAllocate(
        const hidl_vec<BufferDescriptor>& descriptors)
{
    if (!hasCapability(Capability::TEST_ALLOCATE)) {
        return Error::UNDEFINED;
    }

    int32_t err = mDispatch.allocate(mDevice, descriptors.size(),
            &descriptors[0], nullptr);
    return static_cast<Error>(err);
}

Return<void> GrallocHal::allocate(
        const hidl_vec<BufferDescriptor>& descriptors,
        allocate_cb hidl_cb) {
    std::vector<buffer_handle_t> buffers(descriptors.size());
    int32_t err = mDispatch.allocate(mDevice, descriptors.size(),
            &descriptors[0], buffers.data());
    if (err != GRALLOC1_ERROR_NONE && err != GRALLOC1_ERROR_NOT_SHARED) {
        buffers.clear();
    }

    hidl_vec<Buffer> reply;
    reply.setToExternal(
            reinterpret_cast<Buffer*>(buffers.data()),
            buffers.size());
    hidl_cb(static_cast<Error>(err), reply);

    return Void();
}

Return<Error> GrallocHal::free(Buffer buffer)
{
    buffer_handle_t handle = reinterpret_cast<buffer_handle_t>(buffer);
    int32_t err = mDispatch.release(mDevice, handle);
    return static_cast<Error>(err);
}

Return<void> GrallocHal::exportHandle(BufferDescriptor /*descriptor*/,
        Buffer buffer, exportHandle_cb hidl_cb)
{
    // do we want to validate?
    buffer_handle_t handle = reinterpret_cast<buffer_handle_t>(buffer);

    hidl_cb(Error::NONE, handle);

    return Void();
}

IAllocator* HIDL_FETCH_IAllocator(const char* /* name */) {
    const hw_module_t* module;
    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (err) {
        ALOGE("failed to get gralloc module");
        return nullptr;
    }

    uint8_t major = (module->module_api_version >> 8) & 0xff;
    if (major != 1) {
        ALOGE("unknown gralloc module major version %d", major);
        return nullptr;
    }

    return new GrallocHal(module);
}

} // namespace implementation
} // namespace V2_0
} // namespace allocator
} // namespace graphics
} // namespace hardware
} // namespace android
