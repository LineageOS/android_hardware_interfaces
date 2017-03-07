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

#include <mutex>
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
    Return<void> createClient(createClient_cb hidl_cb) override;

    Error createDescriptor(
            const IAllocatorClient::BufferDescriptorInfo& descriptorInfo,
            BufferDescriptor* outDescriptor);
    Error destroyDescriptor(BufferDescriptor descriptor);

    Error testAllocate(const hidl_vec<BufferDescriptor>& descriptors);
    Error allocate(const hidl_vec<BufferDescriptor>& descriptors,
            hidl_vec<Buffer>* outBuffers);
    Error free(Buffer buffer);

    Error exportHandle(Buffer buffer, const native_handle_t** outHandle);

private:
    void initCapabilities();

    template<typename T>
    void initDispatch(gralloc1_function_descriptor_t desc, T* outPfn);
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
    } mDispatch;
};

class GrallocClient : public IAllocatorClient {
public:
    GrallocClient(GrallocHal& hal);
    virtual ~GrallocClient();

    // IAllocatorClient interface
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
    GrallocHal& mHal;

    std::mutex mMutex;
    std::unordered_set<BufferDescriptor> mDescriptors;
    std::unordered_set<Buffer> mBuffers;
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
    uint32_t count = 0;
    mDevice->getCapabilities(mDevice, &count, nullptr);

    std::vector<Capability> caps(count);
    mDevice->getCapabilities(mDevice, &count, reinterpret_cast<
              std::underlying_type<Capability>::type*>(caps.data()));
    caps.resize(count);

    mCapabilities.insert(caps.cbegin(), caps.cend());
}

template<typename T>
void GrallocHal::initDispatch(gralloc1_function_descriptor_t desc, T* outPfn)
{
    auto pfn = mDevice->getFunction(mDevice, desc);
    if (!pfn) {
        LOG_ALWAYS_FATAL("failed to get gralloc1 function %d", desc);
    }

    *outPfn = reinterpret_cast<T>(pfn);
}

void GrallocHal::initDispatch()
{
    initDispatch(GRALLOC1_FUNCTION_DUMP, &mDispatch.dump);
    initDispatch(GRALLOC1_FUNCTION_CREATE_DESCRIPTOR,
            &mDispatch.createDescriptor);
    initDispatch(GRALLOC1_FUNCTION_DESTROY_DESCRIPTOR,
            &mDispatch.destroyDescriptor);
    initDispatch(GRALLOC1_FUNCTION_SET_DIMENSIONS, &mDispatch.setDimensions);
    initDispatch(GRALLOC1_FUNCTION_SET_FORMAT, &mDispatch.setFormat);
    if (hasCapability(Capability::LAYERED_BUFFERS)) {
        initDispatch(GRALLOC1_FUNCTION_SET_LAYER_COUNT,
                &mDispatch.setLayerCount);
    }
    initDispatch(GRALLOC1_FUNCTION_SET_CONSUMER_USAGE,
            &mDispatch.setConsumerUsage);
    initDispatch(GRALLOC1_FUNCTION_SET_PRODUCER_USAGE,
            &mDispatch.setProducerUsage);
    initDispatch(GRALLOC1_FUNCTION_ALLOCATE, &mDispatch.allocate);
    initDispatch(GRALLOC1_FUNCTION_RELEASE, &mDispatch.release);
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

Return<void> GrallocHal::createClient(createClient_cb hidl_cb)
{
    sp<IAllocatorClient> client = new GrallocClient(*this);
    hidl_cb(Error::NONE, client);

    return Void();
}

Error GrallocHal::createDescriptor(
        const IAllocatorClient::BufferDescriptorInfo& descriptorInfo,
        BufferDescriptor* outDescriptor)
{
    gralloc1_buffer_descriptor_t descriptor;
    int32_t err = mDispatch.createDescriptor(mDevice, &descriptor);
    if (err != GRALLOC1_ERROR_NONE) {
        return static_cast<Error>(err);
    }

    err = mDispatch.setDimensions(mDevice, descriptor,
            descriptorInfo.width, descriptorInfo.height);
    if (err == GRALLOC1_ERROR_NONE) {
        err = mDispatch.setFormat(mDevice, descriptor,
                static_cast<int32_t>(descriptorInfo.format));
    }
    if (err == GRALLOC1_ERROR_NONE) {
        if (hasCapability(Capability::LAYERED_BUFFERS)) {
            err = mDispatch.setLayerCount(mDevice, descriptor,
                    descriptorInfo.layerCount);
        } else if (descriptorInfo.layerCount != 1) {
            err = GRALLOC1_ERROR_BAD_VALUE;
        }
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

    if (err == GRALLOC1_ERROR_NONE) {
        *outDescriptor = descriptor;
    } else {
        mDispatch.destroyDescriptor(mDevice, descriptor);
    }

    return static_cast<Error>(err);
}

Error GrallocHal::destroyDescriptor(BufferDescriptor descriptor)
{
    int32_t err = mDispatch.destroyDescriptor(mDevice, descriptor);
    return static_cast<Error>(err);
}

Error GrallocHal::testAllocate(const hidl_vec<BufferDescriptor>& descriptors)
{
    if (!hasCapability(Capability::TEST_ALLOCATE)) {
        return Error::UNDEFINED;
    }

    int32_t err = mDispatch.allocate(mDevice, descriptors.size(),
            descriptors.data(), nullptr);
    return static_cast<Error>(err);
}

Error GrallocHal::allocate(const hidl_vec<BufferDescriptor>& descriptors,
        hidl_vec<Buffer>* outBuffers)
{
    std::vector<buffer_handle_t> buffers(descriptors.size());
    int32_t err = mDispatch.allocate(mDevice, descriptors.size(),
            descriptors.data(), buffers.data());
    if (err == GRALLOC1_ERROR_NONE || err == GRALLOC1_ERROR_NOT_SHARED) {
        outBuffers->resize(buffers.size());
        for (size_t i = 0; i < outBuffers->size(); i++) {
            (*outBuffers)[i] = static_cast<Buffer>(
                    reinterpret_cast<uintptr_t>(buffers[i]));
        }
    }

    return static_cast<Error>(err);
}

Error GrallocHal::free(Buffer buffer)
{
    buffer_handle_t handle = reinterpret_cast<buffer_handle_t>(
            static_cast<uintptr_t>(buffer));

    int32_t err = mDispatch.release(mDevice, handle);
    return static_cast<Error>(err);
}

Error GrallocHal::exportHandle(Buffer buffer,
        const native_handle_t** outHandle)
{
    // we rely on the caller to validate buffer here
    *outHandle = reinterpret_cast<buffer_handle_t>(
            static_cast<uintptr_t>(buffer));
    return Error::NONE;
}

GrallocClient::GrallocClient(GrallocHal& hal)
    : mHal(hal)
{
}

GrallocClient::~GrallocClient()
{
    if (!mBuffers.empty()) {
        ALOGW("client destroyed with valid buffers");
        for (auto buf : mBuffers) {
            mHal.free(buf);
        }
    }

    if (!mDescriptors.empty()) {
        ALOGW("client destroyed with valid buffer descriptors");
        for (auto desc : mDescriptors) {
            mHal.destroyDescriptor(desc);
        }
    }
}

Return<void> GrallocClient::createDescriptor(
        const BufferDescriptorInfo& descriptorInfo,
        createDescriptor_cb hidl_cb)
{
    BufferDescriptor descriptor = 0;
    Error err = mHal.createDescriptor(descriptorInfo, &descriptor);

    if (err == Error::NONE) {
        std::lock_guard<std::mutex> lock(mMutex);

        auto result = mDescriptors.insert(descriptor);
        if (!result.second) {
            ALOGW("duplicated buffer descriptor id returned");
            mHal.destroyDescriptor(descriptor);
            err = Error::NO_RESOURCES;
        }
    }

    hidl_cb(err, descriptor);
    return Void();
}

Return<Error> GrallocClient::destroyDescriptor(BufferDescriptor descriptor)
{
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mDescriptors.erase(descriptor)) {
            return Error::BAD_DESCRIPTOR;
        }
    }

    return mHal.destroyDescriptor(descriptor);
}

Return<Error> GrallocClient::testAllocate(
        const hidl_vec<BufferDescriptor>& descriptors)
{
    return mHal.testAllocate(descriptors);
}

Return<void> GrallocClient::allocate(
        const hidl_vec<BufferDescriptor>& descriptors,
        allocate_cb hidl_cb) {
    hidl_vec<Buffer> buffers;
    Error err = mHal.allocate(descriptors, &buffers);

    if (err == Error::NONE || err == Error::NOT_SHARED) {
        std::lock_guard<std::mutex> lock(mMutex);

        for (size_t i = 0; i < buffers.size(); i++) {
            auto result = mBuffers.insert(buffers[i]);
            if (!result.second) {
                ALOGW("duplicated buffer id returned");

                for (size_t j = 0; j < buffers.size(); j++) {
                    if (j < i) {
                        mBuffers.erase(buffers[i]);
                    }
                    mHal.free(buffers[i]);
                }

                buffers = hidl_vec<Buffer>();
                err = Error::NO_RESOURCES;
                break;
            }
        }
    }

    hidl_cb(err, buffers);
    return Void();
}

Return<Error> GrallocClient::free(Buffer buffer)
{
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mBuffers.erase(buffer)) {
            return Error::BAD_BUFFER;
        }
    }

    return mHal.free(buffer);
}

Return<void> GrallocClient::exportHandle(BufferDescriptor /*descriptor*/,
        Buffer buffer, exportHandle_cb hidl_cb)
{
    const native_handle_t* handle = nullptr;

    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mBuffers.count(buffer) == 0) {
            hidl_cb(Error::BAD_BUFFER, handle);
            return Void();
        }
    }

    Error err = mHal.exportHandle(buffer, &handle);

    hidl_cb(err, handle);
    return Void();
}

IAllocator* HIDL_FETCH_IAllocator(const char* /* name */) {
    const hw_module_t* module = nullptr;
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
