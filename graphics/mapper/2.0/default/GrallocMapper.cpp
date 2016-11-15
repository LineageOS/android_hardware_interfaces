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

#define LOG_TAG "GrallocMapperPassthrough"

#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <hardware/gralloc1.h>
#include <log/log.h>

#include <unordered_set>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace implementation {

using Capability = allocator::V2_0::IAllocator::Capability;

class GrallocDevice : public Device {
public:
    GrallocDevice();
    ~GrallocDevice();

    // IMapper interface
    Error retain(const native_handle_t* bufferHandle);
    Error release(const native_handle_t* bufferHandle);
    Error getDimensions(const native_handle_t* bufferHandle,
            uint32_t* outWidth, uint32_t* outHeight);
    Error getFormat(const native_handle_t* bufferHandle,
            PixelFormat* outFormat);
    Error getLayerCount(const native_handle_t* bufferHandle,
            uint32_t* outLayerCount);
    Error getProducerUsageMask(const native_handle_t* bufferHandle,
            uint64_t* outUsageMask);
    Error getConsumerUsageMask(const native_handle_t* bufferHandle,
            uint64_t* outUsageMask);
    Error getBackingStore(const native_handle_t* bufferHandle,
            BackingStore* outStore);
    Error getStride(const native_handle_t* bufferHandle, uint32_t* outStride);
    Error getNumFlexPlanes(const native_handle_t* bufferHandle,
            uint32_t* outNumPlanes);
    Error lock(const native_handle_t* bufferHandle,
            uint64_t producerUsageMask, uint64_t consumerUsageMask,
            const Rect* accessRegion, int32_t acquireFence, void** outData);
    Error lockFlex(const native_handle_t* bufferHandle,
            uint64_t producerUsageMask, uint64_t consumerUsageMask,
            const Rect* accessRegion, int32_t acquireFence,
            FlexLayout* outFlexLayout);
    Error unlock(const native_handle_t* bufferHandle,
            int32_t* outReleaseFence);

private:
    void initCapabilities();

    void initDispatch();
    bool hasCapability(Capability capability) const;

    gralloc1_device_t* mDevice;

    std::unordered_set<Capability> mCapabilities;

    struct {
        GRALLOC1_PFN_RETAIN retain;
        GRALLOC1_PFN_RELEASE release;
        GRALLOC1_PFN_GET_DIMENSIONS getDimensions;
        GRALLOC1_PFN_GET_FORMAT getFormat;
        GRALLOC1_PFN_GET_LAYER_COUNT getLayerCount;
        GRALLOC1_PFN_GET_PRODUCER_USAGE getProducerUsage;
        GRALLOC1_PFN_GET_CONSUMER_USAGE getConsumerUsage;
        GRALLOC1_PFN_GET_BACKING_STORE getBackingStore;
        GRALLOC1_PFN_GET_STRIDE getStride;
        GRALLOC1_PFN_GET_NUM_FLEX_PLANES getNumFlexPlanes;
        GRALLOC1_PFN_LOCK lock;
        GRALLOC1_PFN_LOCK_FLEX lockFlex;
        GRALLOC1_PFN_UNLOCK unlock;
    } mDispatch;
};

GrallocDevice::GrallocDevice()
{
    const hw_module_t* module;
    int status = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (status) {
        LOG_ALWAYS_FATAL("failed to get gralloc module");
    }

    uint8_t major = (module->module_api_version >> 8) & 0xff;
    if (major != 1) {
        LOG_ALWAYS_FATAL("unknown gralloc module major version %d", major);
    }

    status = gralloc1_open(module, &mDevice);
    if (status) {
        LOG_ALWAYS_FATAL("failed to open gralloc1 device");
    }

    initCapabilities();
    initDispatch();
}

GrallocDevice::~GrallocDevice()
{
    gralloc1_close(mDevice);
}

void GrallocDevice::initCapabilities()
{
    uint32_t count;
    mDevice->getCapabilities(mDevice, &count, nullptr);

    std::vector<Capability> caps(count);
    mDevice->getCapabilities(mDevice, &count, reinterpret_cast<
              std::underlying_type<Capability>::type*>(caps.data()));
    caps.resize(count);

    mCapabilities.insert(caps.cbegin(), caps.cend());
}

void GrallocDevice::initDispatch()
{
#define CHECK_FUNC(func, desc) do {                                   \
    mDispatch.func = reinterpret_cast<decltype(mDispatch.func)>(      \
        mDevice->getFunction(mDevice, desc));                         \
    if (!mDispatch.func) {                                            \
        LOG_ALWAYS_FATAL("failed to get gralloc1 function %d", desc); \
    }                                                                 \
} while (0)

    CHECK_FUNC(retain, GRALLOC1_FUNCTION_RETAIN);
    CHECK_FUNC(release, GRALLOC1_FUNCTION_RELEASE);
    CHECK_FUNC(getDimensions, GRALLOC1_FUNCTION_GET_DIMENSIONS);
    CHECK_FUNC(getFormat, GRALLOC1_FUNCTION_GET_FORMAT);
    if (hasCapability(Capability::LAYERED_BUFFERS)) {
        CHECK_FUNC(getLayerCount, GRALLOC1_FUNCTION_GET_LAYER_COUNT);
    }
    CHECK_FUNC(getProducerUsage, GRALLOC1_FUNCTION_GET_PRODUCER_USAGE);
    CHECK_FUNC(getConsumerUsage, GRALLOC1_FUNCTION_GET_CONSUMER_USAGE);
    CHECK_FUNC(getBackingStore, GRALLOC1_FUNCTION_GET_BACKING_STORE);
    CHECK_FUNC(getStride, GRALLOC1_FUNCTION_GET_STRIDE);
    CHECK_FUNC(getNumFlexPlanes, GRALLOC1_FUNCTION_GET_NUM_FLEX_PLANES);
    CHECK_FUNC(lock, GRALLOC1_FUNCTION_LOCK);
    CHECK_FUNC(lockFlex, GRALLOC1_FUNCTION_LOCK_FLEX);
    CHECK_FUNC(unlock, GRALLOC1_FUNCTION_UNLOCK);

#undef CHECK_FUNC
}

bool GrallocDevice::hasCapability(Capability capability) const
{
    return (mCapabilities.count(capability) > 0);
}

Error GrallocDevice::retain(const native_handle_t* bufferHandle)
{
    int32_t error = mDispatch.retain(mDevice, bufferHandle);
    return static_cast<Error>(error);
}

Error GrallocDevice::release(const native_handle_t* bufferHandle)
{
    int32_t error = mDispatch.release(mDevice, bufferHandle);
    return static_cast<Error>(error);
}

Error GrallocDevice::getDimensions(const native_handle_t* bufferHandle,
        uint32_t* outWidth, uint32_t* outHeight)
{
    int32_t error = mDispatch.getDimensions(mDevice, bufferHandle,
            outWidth, outHeight);
    return static_cast<Error>(error);
}

Error GrallocDevice::getFormat(const native_handle_t* bufferHandle,
        PixelFormat* outFormat)
{
    int32_t error = mDispatch.getFormat(mDevice, bufferHandle,
            reinterpret_cast<int32_t*>(outFormat));
    return static_cast<Error>(error);
}

Error GrallocDevice::getLayerCount(const native_handle_t* bufferHandle,
        uint32_t* outLayerCount)
{
    if (hasCapability(Capability::LAYERED_BUFFERS)) {
        int32_t error = mDispatch.getLayerCount(mDevice, bufferHandle,
                outLayerCount);
        return static_cast<Error>(error);
    } else {
        *outLayerCount = 1;
        return Error::NONE;
    }
}

Error GrallocDevice::getProducerUsageMask(const native_handle_t* bufferHandle,
        uint64_t* outUsageMask)
{
    int32_t error = mDispatch.getProducerUsage(mDevice, bufferHandle,
            outUsageMask);
    return static_cast<Error>(error);
}

Error GrallocDevice::getConsumerUsageMask(const native_handle_t* bufferHandle,
        uint64_t* outUsageMask)
{
    int32_t error = mDispatch.getConsumerUsage(mDevice, bufferHandle,
            outUsageMask);
    return static_cast<Error>(error);
}

Error GrallocDevice::getBackingStore(const native_handle_t* bufferHandle,
        BackingStore* outStore)
{
    int32_t error = mDispatch.getBackingStore(mDevice, bufferHandle,
            outStore);
    return static_cast<Error>(error);
}

Error GrallocDevice::getStride(const native_handle_t* bufferHandle,
        uint32_t* outStride)
{
    int32_t error = mDispatch.getStride(mDevice, bufferHandle, outStride);
    return static_cast<Error>(error);
}

Error GrallocDevice::getNumFlexPlanes(const native_handle_t* bufferHandle,
        uint32_t* outNumPlanes)
{
    int32_t error = mDispatch.getNumFlexPlanes(mDevice, bufferHandle,
            outNumPlanes);
    return static_cast<Error>(error);
}

Error GrallocDevice::lock(const native_handle_t* bufferHandle,
        uint64_t producerUsageMask, uint64_t consumerUsageMask,
        const Rect* accessRegion, int32_t acquireFence,
        void** outData)
{
    int32_t error = mDispatch.lock(mDevice, bufferHandle,
            producerUsageMask, consumerUsageMask,
            reinterpret_cast<const gralloc1_rect_t*>(accessRegion),
            outData, acquireFence);
    return static_cast<Error>(error);
}

Error GrallocDevice::lockFlex(const native_handle_t* bufferHandle,
        uint64_t producerUsageMask, uint64_t consumerUsageMask,
        const Rect* accessRegion, int32_t acquireFence,
        FlexLayout* outFlexLayout)
{
    int32_t error = mDispatch.lockFlex(mDevice, bufferHandle,
            producerUsageMask, consumerUsageMask,
            reinterpret_cast<const gralloc1_rect_t*>(accessRegion),
            reinterpret_cast<android_flex_layout_t*>(outFlexLayout),
            acquireFence);
    return static_cast<Error>(error);
}

Error GrallocDevice::unlock(const native_handle_t* bufferHandle,
        int32_t* outReleaseFence)
{
    int32_t error = mDispatch.unlock(mDevice, bufferHandle, outReleaseFence);
    return static_cast<Error>(error);
}

class GrallocMapper : public IMapper {
public:
    GrallocMapper() : IMapper{
        .createDevice = createDevice,
        .destroyDevice = destroyDevice,
        .retain = retain,
        .release = release,
        .getDimensions = getDimensions,
        .getFormat = getFormat,
        .getLayerCount = getLayerCount,
        .getProducerUsageMask = getProducerUsageMask,
        .getConsumerUsageMask = getConsumerUsageMask,
        .getBackingStore = getBackingStore,
        .getStride = getStride,
        .getNumFlexPlanes = getNumFlexPlanes,
        .lock = lock,
        .lockFlex = lockFlex,
        .unlock = unlock,
    } {}

    const IMapper* getInterface() const
    {
        return static_cast<const IMapper*>(this);
    }

private:
    static GrallocDevice* cast(Device* device)
    {
        return reinterpret_cast<GrallocDevice*>(device);
    }

    static Error createDevice(Device** outDevice)
    {
        *outDevice = new GrallocDevice;
        return Error::NONE;
    }

    static Error destroyDevice(Device* device)
    {
        delete cast(device);
        return Error::NONE;
    }

    static Error retain(Device* device,
            const native_handle_t* bufferHandle)
    {
        return cast(device)->retain(bufferHandle);
    }

    static Error release(Device* device,
            const native_handle_t* bufferHandle)
    {
        return cast(device)->release(bufferHandle);
    }

    static Error getDimensions(Device* device,
            const native_handle_t* bufferHandle,
            uint32_t* outWidth, uint32_t* outHeight)
    {
        return cast(device)->getDimensions(bufferHandle, outWidth, outHeight);
    }

    static Error getFormat(Device* device,
            const native_handle_t* bufferHandle, PixelFormat* outFormat)
    {
        return cast(device)->getFormat(bufferHandle, outFormat);
    }

    static Error getLayerCount(Device* device,
            const native_handle_t* bufferHandle, uint32_t* outLayerCount)
    {
        return cast(device)->getLayerCount(bufferHandle, outLayerCount);
    }

    static Error getProducerUsageMask(Device* device,
            const native_handle_t* bufferHandle, uint64_t* outUsageMask)
    {
        return cast(device)->getProducerUsageMask(bufferHandle, outUsageMask);
    }

    static Error getConsumerUsageMask(Device* device,
            const native_handle_t* bufferHandle, uint64_t* outUsageMask)
    {
        return cast(device)->getConsumerUsageMask(bufferHandle, outUsageMask);
    }

    static Error getBackingStore(Device* device,
            const native_handle_t* bufferHandle, BackingStore* outStore)
    {
        return cast(device)->getBackingStore(bufferHandle, outStore);
    }

    static Error getStride(Device* device,
            const native_handle_t* bufferHandle, uint32_t* outStride)
    {
        return cast(device)->getStride(bufferHandle, outStride);
    }

    static Error getNumFlexPlanes(Device* device,
            const native_handle_t* bufferHandle, uint32_t* outNumPlanes)
    {
        return cast(device)->getNumFlexPlanes(bufferHandle, outNumPlanes);
    }

    static Error lock(Device* device,
            const native_handle_t* bufferHandle,
            uint64_t producerUsageMask, uint64_t consumerUsageMask,
            const Device::Rect* accessRegion, int32_t acquireFence,
            void** outData)
    {
        return cast(device)->lock(bufferHandle,
                producerUsageMask, consumerUsageMask,
                accessRegion, acquireFence, outData);
    }

    static Error lockFlex(Device* device,
            const native_handle_t* bufferHandle,
            uint64_t producerUsageMask, uint64_t consumerUsageMask,
            const Device::Rect* accessRegion, int32_t acquireFence,
            FlexLayout* outFlexLayout)
    {
        return cast(device)->lockFlex(bufferHandle,
                producerUsageMask, consumerUsageMask,
                accessRegion, acquireFence, outFlexLayout);
    }

    static Error unlock(Device* device,
            const native_handle_t* bufferHandle, int32_t* outReleaseFence)
    {
        return cast(device)->unlock(bufferHandle, outReleaseFence);
    }
};

extern "C" const void* HALLIB_FETCH_Interface(const char* name)
{
    if (strcmp(name, "android.hardware.graphics.mapper@2.0::IMapper") == 0) {
        static GrallocMapper sGrallocMapper;
        return sGrallocMapper.getInterface();
    }

    return nullptr;
}

} // namespace implementation
} // namespace V2_0
} // namespace mapper
} // namespace graphics
} // namespace hardware
} // namespace android
