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

#include "GrallocMapper.h"

#include <vector>

#include <string.h>

#include <hardware/gralloc1.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace implementation {

namespace {

using android::hardware::graphics::allocator::V2_0::Error;
using android::hardware::graphics::common::V1_0::PixelFormat;

class GrallocMapperHal : public IMapper {
public:
    GrallocMapperHal(const hw_module_t* module);
    ~GrallocMapperHal();

    // IMapper interface
    Return<Error> retain(const hidl_handle& bufferHandle) override;
    Return<Error> release(const hidl_handle& bufferHandle) override;
    Return<void> getDimensions(const hidl_handle& bufferHandle,
            getDimensions_cb hidl_cb) override;
    Return<void> getFormat(const hidl_handle& bufferHandle,
            getFormat_cb hidl_cb) override;
    Return<void> getLayerCount(const hidl_handle& bufferHandle,
            getLayerCount_cb hidl_cb) override;
    Return<void> getProducerUsageMask(const hidl_handle& bufferHandle,
            getProducerUsageMask_cb hidl_cb) override;
    Return<void> getConsumerUsageMask(const hidl_handle& bufferHandle,
            getConsumerUsageMask_cb hidl_cb) override;
    Return<void> getBackingStore(const hidl_handle& bufferHandle,
            getBackingStore_cb hidl_cb) override;
    Return<void> getStride(const hidl_handle& bufferHandle,
            getStride_cb hidl_cb) override;
    Return<void> lock(const hidl_handle& bufferHandle,
            uint64_t producerUsageMask, uint64_t consumerUsageMask,
            const IMapper::Rect& accessRegion, const hidl_handle& acquireFence,
            lock_cb hidl_cb) override;
    Return<void> lockFlex(const hidl_handle& bufferHandle,
            uint64_t producerUsageMask, uint64_t consumerUsageMask,
            const IMapper::Rect& accessRegion, const hidl_handle& acquireFence,
            lockFlex_cb hidl_cb) override;
    Return<void> unlock(const hidl_handle& bufferHandle,
            unlock_cb hidl_cb) override;

private:
    void initCapabilities();

    template<typename T>
    void initDispatch(gralloc1_function_descriptor_t desc, T* outPfn);
    void initDispatch();

    static gralloc1_rect_t asGralloc1Rect(const IMapper::Rect& rect);
    static bool dupFence(const hidl_handle& fenceHandle, int* outFd);

    gralloc1_device_t* mDevice;

    struct {
        bool layeredBuffers;
    } mCapabilities;

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

GrallocMapperHal::GrallocMapperHal(const hw_module_t* module)
    : mDevice(nullptr), mCapabilities(), mDispatch()
{
    int status = gralloc1_open(module, &mDevice);
    if (status) {
        LOG_ALWAYS_FATAL("failed to open gralloc1 device: %s",
                strerror(-status));
    }

    initCapabilities();
    initDispatch();
}

GrallocMapperHal::~GrallocMapperHal()
{
    gralloc1_close(mDevice);
}

void GrallocMapperHal::initCapabilities()
{
    uint32_t count;
    mDevice->getCapabilities(mDevice, &count, nullptr);

    std::vector<int32_t> caps(count);
    mDevice->getCapabilities(mDevice, &count, caps.data());
    caps.resize(count);

    for (auto cap : caps) {
        switch (cap) {
        case GRALLOC1_CAPABILITY_LAYERED_BUFFERS:
            mCapabilities.layeredBuffers = true;
            break;
        default:
            break;
        }
    }
}

template<typename T>
void GrallocMapperHal::initDispatch(gralloc1_function_descriptor_t desc,
        T* outPfn)
{
    auto pfn = mDevice->getFunction(mDevice, desc);
    if (!pfn) {
        LOG_ALWAYS_FATAL("failed to get gralloc1 function %d", desc);
    }

    *outPfn = reinterpret_cast<T>(pfn);
}

void GrallocMapperHal::initDispatch()
{
    initDispatch(GRALLOC1_FUNCTION_RETAIN, &mDispatch.retain);
    initDispatch(GRALLOC1_FUNCTION_RELEASE, &mDispatch.release);
    initDispatch(GRALLOC1_FUNCTION_GET_DIMENSIONS, &mDispatch.getDimensions);
    initDispatch(GRALLOC1_FUNCTION_GET_FORMAT, &mDispatch.getFormat);
    if (mCapabilities.layeredBuffers) {
        initDispatch(GRALLOC1_FUNCTION_GET_LAYER_COUNT,
                &mDispatch.getLayerCount);
    }
    initDispatch(GRALLOC1_FUNCTION_GET_PRODUCER_USAGE,
            &mDispatch.getProducerUsage);
    initDispatch(GRALLOC1_FUNCTION_GET_CONSUMER_USAGE,
            &mDispatch.getConsumerUsage);
    initDispatch(GRALLOC1_FUNCTION_GET_BACKING_STORE,
            &mDispatch.getBackingStore);
    initDispatch(GRALLOC1_FUNCTION_GET_STRIDE, &mDispatch.getStride);
    initDispatch(GRALLOC1_FUNCTION_GET_NUM_FLEX_PLANES,
            &mDispatch.getNumFlexPlanes);
    initDispatch(GRALLOC1_FUNCTION_LOCK, &mDispatch.lock);
    initDispatch(GRALLOC1_FUNCTION_LOCK_FLEX, &mDispatch.lockFlex);
    initDispatch(GRALLOC1_FUNCTION_UNLOCK, &mDispatch.unlock);
}

gralloc1_rect_t GrallocMapperHal::asGralloc1Rect(const IMapper::Rect& rect)
{
    return gralloc1_rect_t{rect.left, rect.top, rect.width, rect.height};
}

bool GrallocMapperHal::dupFence(const hidl_handle& fenceHandle, int* outFd)
{
    auto handle = fenceHandle.getNativeHandle();
    if (!handle || handle->numFds == 0) {
        *outFd = -1;
        return true;
    }

    if (handle->numFds > 1) {
        ALOGE("invalid fence handle with %d fds", handle->numFds);
        return false;
    }

    *outFd = dup(handle->data[0]);
    return (*outFd >= 0);
}

Return<Error> GrallocMapperHal::retain(const hidl_handle& bufferHandle)
{
    int32_t err = mDispatch.retain(mDevice, bufferHandle);
    return static_cast<Error>(err);
}

Return<Error> GrallocMapperHal::release(const hidl_handle& bufferHandle)
{
    int32_t err = mDispatch.release(mDevice, bufferHandle);
    return static_cast<Error>(err);
}

Return<void> GrallocMapperHal::getDimensions(const hidl_handle& bufferHandle,
        getDimensions_cb hidl_cb)
{
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t err = mDispatch.getDimensions(mDevice, bufferHandle,
            &width, &height);

    hidl_cb(static_cast<Error>(err), width, height);
    return Void();
}

Return<void> GrallocMapperHal::getFormat(const hidl_handle& bufferHandle,
        getFormat_cb hidl_cb)
{
    int32_t format = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
    int32_t err = mDispatch.getFormat(mDevice, bufferHandle, &format);

    hidl_cb(static_cast<Error>(err), static_cast<PixelFormat>(format));
    return Void();
}

Return<void> GrallocMapperHal::getLayerCount(const hidl_handle& bufferHandle,
        getLayerCount_cb hidl_cb)
{
    int32_t err = GRALLOC1_ERROR_NONE;
    uint32_t count = 1;
    if (mCapabilities.layeredBuffers) {
        err = mDispatch.getLayerCount(mDevice, bufferHandle, &count);
    }

    hidl_cb(static_cast<Error>(err), count);
    return Void();
}

Return<void> GrallocMapperHal::getProducerUsageMask(
        const hidl_handle& bufferHandle, getProducerUsageMask_cb hidl_cb)
{
    uint64_t mask = 0x0;
    int32_t err = mDispatch.getProducerUsage(mDevice, bufferHandle, &mask);

    hidl_cb(static_cast<Error>(err), mask);
    return Void();
}

Return<void> GrallocMapperHal::getConsumerUsageMask(
        const hidl_handle& bufferHandle, getConsumerUsageMask_cb hidl_cb)
{
    uint64_t mask = 0x0;
    int32_t err = mDispatch.getConsumerUsage(mDevice, bufferHandle, &mask);

    hidl_cb(static_cast<Error>(err), mask);
    return Void();
}

Return<void> GrallocMapperHal::getBackingStore(
        const hidl_handle& bufferHandle, getBackingStore_cb hidl_cb)
{
    uint64_t store = 0;
    int32_t err = mDispatch.getBackingStore(mDevice, bufferHandle, &store);

    hidl_cb(static_cast<Error>(err), store);
    return Void();
}

Return<void> GrallocMapperHal::getStride(const hidl_handle& bufferHandle,
        getStride_cb hidl_cb)
{
    uint32_t stride = 0;
    int32_t err = mDispatch.getStride(mDevice, bufferHandle, &stride);

    hidl_cb(static_cast<Error>(err), stride);
    return Void();
}

Return<void> GrallocMapperHal::lock(const hidl_handle& bufferHandle,
        uint64_t producerUsageMask, uint64_t consumerUsageMask,
        const IMapper::Rect& accessRegion, const hidl_handle& acquireFence,
        lock_cb hidl_cb)
{
    gralloc1_rect_t rect = asGralloc1Rect(accessRegion);

    int fence = -1;
    if (!dupFence(acquireFence, &fence)) {
        hidl_cb(Error::NO_RESOURCES, nullptr);
        return Void();
    }

    void* data = nullptr;
    int32_t err = mDispatch.lock(mDevice, bufferHandle, producerUsageMask,
            consumerUsageMask, &rect, &data, fence);
    if (err != GRALLOC1_ERROR_NONE) {
        close(fence);
    }

    hidl_cb(static_cast<Error>(err), data);
    return Void();
}

Return<void> GrallocMapperHal::lockFlex(const hidl_handle& bufferHandle,
        uint64_t producerUsageMask, uint64_t consumerUsageMask,
        const IMapper::Rect& accessRegion, const hidl_handle& acquireFence,
        lockFlex_cb hidl_cb)
{
    FlexLayout layout_reply{};

    uint32_t planeCount = 0;
    int32_t err = mDispatch.getNumFlexPlanes(mDevice, bufferHandle,
            &planeCount);
    if (err != GRALLOC1_ERROR_NONE) {
        hidl_cb(static_cast<Error>(err), layout_reply);
        return Void();
    }

    gralloc1_rect_t rect = asGralloc1Rect(accessRegion);

    int fence = -1;
    if (!dupFence(acquireFence, &fence)) {
        hidl_cb(Error::NO_RESOURCES, layout_reply);
        return Void();
    }

    std::vector<android_flex_plane_t> planes(planeCount);
    android_flex_layout_t layout{};
    layout.num_planes = planes.size();
    layout.planes = planes.data();

    err = mDispatch.lockFlex(mDevice, bufferHandle, producerUsageMask,
            consumerUsageMask, &rect, &layout, fence);
    if (err == GRALLOC1_ERROR_NONE) {
        layout_reply.format = static_cast<FlexFormat>(layout.format);

        planes.resize(layout.num_planes);
        layout_reply.planes.setToExternal(
                reinterpret_cast<FlexPlane*>(planes.data()), planes.size());
    } else {
        close(fence);
    }

    hidl_cb(static_cast<Error>(err), layout_reply);
    return Void();
}

Return<void> GrallocMapperHal::unlock(const hidl_handle& bufferHandle,
        unlock_cb hidl_cb)
{
    int32_t fence = -1;
    int32_t err = mDispatch.unlock(mDevice, bufferHandle, &fence);

    NATIVE_HANDLE_DECLARE_STORAGE(fenceStorage, 1, 0);
    hidl_handle fenceHandle;
    if (err == GRALLOC1_ERROR_NONE && fence >= 0) {
        auto nativeHandle = native_handle_init(fenceStorage, 1, 0);
        nativeHandle->data[0] = fence;

        fenceHandle = nativeHandle;
    }

    hidl_cb(static_cast<Error>(err), fenceHandle);
    return Void();
}

} // anonymous namespace

IMapper* HIDL_FETCH_IMapper(const char* /* name */) {
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

    return new GrallocMapperHal(module);
}

} // namespace implementation
} // namespace V2_0
} // namespace mapper
} // namespace graphics
} // namespace hardware
} // namespace android
