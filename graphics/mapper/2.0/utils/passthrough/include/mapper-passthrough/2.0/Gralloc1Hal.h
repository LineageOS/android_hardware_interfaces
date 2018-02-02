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

#ifndef LOG_TAG
#warning "Gralloc1Hal.h included without LOG_TAG"
#endif

#include <inttypes.h>

#include <vector>

#include <hardware/gralloc1.h>
#include <log/log.h>
#include <mapper-hal/2.0/MapperHal.h>
#include <mapper-passthrough/2.0/GrallocBufferDescriptor.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace passthrough {

namespace detail {

using common::V1_0::BufferUsage;

// Gralloc1HalImpl implements V2_*::hal::MapperHal on top of gralloc1
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

    Error createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo,
                           BufferDescriptor* outDescriptor) override {
        if (!descriptorInfo.width || !descriptorInfo.height || !descriptorInfo.layerCount) {
            return Error::BAD_VALUE;
        }

        if (!mCapabilities.layeredBuffers && descriptorInfo.layerCount != 1) {
            return Error::UNSUPPORTED;
        }

        if (descriptorInfo.format == static_cast<PixelFormat>(0)) {
            return Error::BAD_VALUE;
        }

        const uint64_t validUsageBits = getValidBufferUsageMask();
        if (descriptorInfo.usage & ~validUsageBits) {
            ALOGW("buffer descriptor with invalid usage bits 0x%" PRIx64,
                  descriptorInfo.usage & ~validUsageBits);
        }

        *outDescriptor = grallocEncodeBufferDescriptor(descriptorInfo);

        return Error::NONE;
    }

    Error importBuffer(const native_handle_t* rawHandle,
                       native_handle_t** outBufferHandle) override {
        native_handle_t* bufferHandle = native_handle_clone(rawHandle);
        if (!bufferHandle) {
            return Error::NO_RESOURCES;
        }

        int32_t error = mDispatch.retain(mDevice, bufferHandle);
        if (error != GRALLOC1_ERROR_NONE) {
            native_handle_close(bufferHandle);
            native_handle_delete(bufferHandle);
            return toError(error);
        }

        *outBufferHandle = bufferHandle;

        return Error::NONE;
    }

    Error freeBuffer(native_handle_t* bufferHandle) override {
        int32_t error = mDispatch.release(mDevice, bufferHandle);
        if (error == GRALLOC1_ERROR_NONE && !mCapabilities.releaseImplyDelete) {
            native_handle_close(bufferHandle);
            native_handle_delete(bufferHandle);
        }
        return toError(error);
    }

    Error lock(const native_handle_t* bufferHandle, uint64_t cpuUsage,
               const IMapper::Rect& accessRegion, base::unique_fd fenceFd,
               void** outData) override {
        const uint64_t consumerUsage =
            cpuUsage & ~static_cast<uint64_t>(BufferUsage::CPU_WRITE_MASK);
        const auto accessRect = asGralloc1Rect(accessRegion);
        void* data = nullptr;
        int32_t error = mDispatch.lock(mDevice, bufferHandle, cpuUsage, consumerUsage, &accessRect,
                                       &data, fenceFd.release());
        if (error == GRALLOC1_ERROR_NONE) {
            *outData = data;
        }

        return toError(error);
    }

    Error lockYCbCr(const native_handle_t* bufferHandle, uint64_t cpuUsage,
                    const IMapper::Rect& accessRegion, base::unique_fd fenceFd,
                    YCbCrLayout* outLayout) override {
        // prepare flex layout
        android_flex_layout flex = {};
        int32_t error = mDispatch.getNumFlexPlanes(mDevice, bufferHandle, &flex.num_planes);
        if (error != GRALLOC1_ERROR_NONE) {
            return toError(error);
        }
        std::vector<android_flex_plane_t> flexPlanes(flex.num_planes);
        flex.planes = flexPlanes.data();

        const uint64_t consumerUsage =
            cpuUsage & ~static_cast<uint64_t>(BufferUsage::CPU_WRITE_MASK);
        const auto accessRect = asGralloc1Rect(accessRegion);
        error = mDispatch.lockFlex(mDevice, bufferHandle, cpuUsage, consumerUsage, &accessRect,
                                   &flex, fenceFd.release());
        if (error == GRALLOC1_ERROR_NONE && !toYCbCrLayout(flex, outLayout)) {
            ALOGD("unable to convert android_flex_layout to YCbCrLayout");
            // undo the lock
            unlock(bufferHandle, &fenceFd);
            error = GRALLOC1_ERROR_BAD_HANDLE;
        }

        return toError(error);
    }

    Error unlock(const native_handle_t* bufferHandle, base::unique_fd* outFenceFd) override {
        int fenceFd = -1;
        int32_t error = mDispatch.unlock(mDevice, bufferHandle, &fenceFd);

        // we always own the fenceFd even when unlock failed
        outFenceFd->reset(fenceFd);
        return toError(error);
    }

   protected:
    virtual void initCapabilities() {
        uint32_t count = 0;
        mDevice->getCapabilities(mDevice, &count, nullptr);

        std::vector<int32_t> capabilities(count);
        mDevice->getCapabilities(mDevice, &count, capabilities.data());
        capabilities.resize(count);

        for (auto capability : capabilities) {
            switch (capability) {
                case GRALLOC1_CAPABILITY_LAYERED_BUFFERS:
                    mCapabilities.layeredBuffers = true;
                    break;
                case GRALLOC1_CAPABILITY_RELEASE_IMPLY_DELETE:
                    mCapabilities.releaseImplyDelete = true;
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
        if (!initDispatch(GRALLOC1_FUNCTION_RETAIN, &mDispatch.retain) ||
            !initDispatch(GRALLOC1_FUNCTION_RELEASE, &mDispatch.release) ||
            !initDispatch(GRALLOC1_FUNCTION_GET_NUM_FLEX_PLANES, &mDispatch.getNumFlexPlanes) ||
            !initDispatch(GRALLOC1_FUNCTION_LOCK, &mDispatch.lock) ||
            !initDispatch(GRALLOC1_FUNCTION_LOCK_FLEX, &mDispatch.lockFlex) ||
            !initDispatch(GRALLOC1_FUNCTION_UNLOCK, &mDispatch.unlock)) {
            return false;
        }

        return true;
    }

    virtual uint64_t getValidBufferUsageMask() const {
        return BufferUsage::CPU_READ_MASK | BufferUsage::CPU_WRITE_MASK | BufferUsage::GPU_TEXTURE |
               BufferUsage::GPU_RENDER_TARGET | BufferUsage::COMPOSER_OVERLAY |
               BufferUsage::COMPOSER_CLIENT_TARGET | BufferUsage::PROTECTED |
               BufferUsage::COMPOSER_CURSOR | BufferUsage::VIDEO_ENCODER |
               BufferUsage::CAMERA_OUTPUT | BufferUsage::CAMERA_INPUT | BufferUsage::RENDERSCRIPT |
               BufferUsage::VIDEO_DECODER | BufferUsage::SENSOR_DIRECT_DATA |
               BufferUsage::GPU_DATA_BUFFER | BufferUsage::VENDOR_MASK |
               BufferUsage::VENDOR_MASK_HI;
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

    static bool toYCbCrLayout(const android_flex_layout& flex, YCbCrLayout* outLayout) {
        // must be YCbCr
        if (flex.format != FLEX_FORMAT_YCbCr || flex.num_planes < 3) {
            return false;
        }

        for (int i = 0; i < 3; i++) {
            const auto& plane = flex.planes[i];
            // must have 8-bit depth
            if (plane.bits_per_component != 8 || plane.bits_used != 8) {
                return false;
            }

            if (plane.component == FLEX_COMPONENT_Y) {
                // Y must not be interleaved
                if (plane.h_increment != 1) {
                    return false;
                }
            } else {
                // Cb and Cr can be interleaved
                if (plane.h_increment != 1 && plane.h_increment != 2) {
                    return false;
                }
            }

            if (!plane.v_increment) {
                return false;
            }
        }

        if (flex.planes[0].component != FLEX_COMPONENT_Y ||
            flex.planes[1].component != FLEX_COMPONENT_Cb ||
            flex.planes[2].component != FLEX_COMPONENT_Cr) {
            return false;
        }

        const auto& y = flex.planes[0];
        const auto& cb = flex.planes[1];
        const auto& cr = flex.planes[2];

        if (cb.h_increment != cr.h_increment || cb.v_increment != cr.v_increment) {
            return false;
        }

        outLayout->y = y.top_left;
        outLayout->cb = cb.top_left;
        outLayout->cr = cr.top_left;
        outLayout->yStride = y.v_increment;
        outLayout->cStride = cb.v_increment;
        outLayout->chromaStep = cb.h_increment;

        return true;
    }

    static gralloc1_rect_t asGralloc1Rect(const IMapper::Rect& rect) {
        return gralloc1_rect_t{rect.left, rect.top, rect.width, rect.height};
    }

    gralloc1_device_t* mDevice = nullptr;

    struct {
        bool layeredBuffers;
        bool releaseImplyDelete;
    } mCapabilities = {};

    struct {
        GRALLOC1_PFN_RETAIN retain;
        GRALLOC1_PFN_RELEASE release;
        GRALLOC1_PFN_GET_NUM_FLEX_PLANES getNumFlexPlanes;
        GRALLOC1_PFN_LOCK lock;
        GRALLOC1_PFN_LOCK_FLEX lockFlex;
        GRALLOC1_PFN_UNLOCK unlock;
    } mDispatch = {};
};

}  // namespace detail

using Gralloc1Hal = detail::Gralloc1HalImpl<hal::MapperHal>;

}  // namespace passthrough
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
