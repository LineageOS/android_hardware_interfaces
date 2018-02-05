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
#warning "Gralloc0Hal.h included without LOG_TAG"
#endif

#include <inttypes.h>

#include <hardware/gralloc.h>
#include <log/log.h>
#include <mapper-hal/2.0/MapperHal.h>
#include <mapper-passthrough/2.0/GrallocBufferDescriptor.h>
#include <sync/sync.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace passthrough {

namespace detail {

using common::V1_0::BufferUsage;

// Gralloc0HalImpl implements V2_*::hal::MapperHal on top of gralloc0
template <typename Hal>
class Gralloc0HalImpl : public Hal {
   public:
    bool initWithModule(const hw_module_t* module) {
        mModule = reinterpret_cast<const gralloc_module_t*>(module);
        mMinor = module->module_api_version & 0xff;
        return true;
    }

    Error createDescriptor(const IMapper::BufferDescriptorInfo& descriptorInfo,
                           BufferDescriptor* outDescriptor) override {
        if (!descriptorInfo.width || !descriptorInfo.height || !descriptorInfo.layerCount) {
            return Error::BAD_VALUE;
        }

        if (descriptorInfo.layerCount != 1) {
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

        if (mModule->registerBuffer(mModule, bufferHandle)) {
            native_handle_close(bufferHandle);
            native_handle_delete(bufferHandle);
            return Error::BAD_BUFFER;
        }

        *outBufferHandle = bufferHandle;

        return Error::NONE;
    }

    Error freeBuffer(native_handle_t* bufferHandle) override {
        if (mModule->unregisterBuffer(mModule, bufferHandle)) {
            return Error::BAD_BUFFER;
        }

        native_handle_close(bufferHandle);
        native_handle_delete(bufferHandle);
        return Error::NONE;
    }

    Error lock(const native_handle_t* bufferHandle, uint64_t cpuUsage,
               const IMapper::Rect& accessRegion, base::unique_fd fenceFd,
               void** outData) override {
        int result;
        void* data = nullptr;
        if (mMinor >= 3 && mModule->lockAsync) {
            result = mModule->lockAsync(mModule, bufferHandle, cpuUsage, accessRegion.left,
                                        accessRegion.top, accessRegion.width, accessRegion.height,
                                        &data, fenceFd.release());
        } else {
            waitFenceFd(fenceFd, "Gralloc0Hal::lock");

            result =
                mModule->lock(mModule, bufferHandle, cpuUsage, accessRegion.left, accessRegion.top,
                              accessRegion.width, accessRegion.height, &data);
        }

        if (result) {
            return Error::BAD_VALUE;
        }

        *outData = data;
        return Error::NONE;
    }

    Error lockYCbCr(const native_handle_t* bufferHandle, uint64_t cpuUsage,
                    const IMapper::Rect& accessRegion, base::unique_fd fenceFd,
                    YCbCrLayout* outLayout) override {
        int result;
        android_ycbcr ycbcr = {};
        if (mMinor >= 3 && mModule->lockAsync_ycbcr) {
            result = mModule->lockAsync_ycbcr(mModule, bufferHandle, cpuUsage, accessRegion.left,
                                              accessRegion.top, accessRegion.width,
                                              accessRegion.height, &ycbcr, fenceFd.release());
        } else {
            waitFenceFd(fenceFd, "Gralloc0Hal::lockYCbCr");

            if (mModule->lock_ycbcr) {
                result = mModule->lock_ycbcr(mModule, bufferHandle, cpuUsage, accessRegion.left,
                                             accessRegion.top, accessRegion.width,
                                             accessRegion.height, &ycbcr);
            } else {
                result = -EINVAL;
            }
        }

        if (result) {
            return Error::BAD_VALUE;
        }

        outLayout->y = ycbcr.y;
        outLayout->cb = ycbcr.cb;
        outLayout->cr = ycbcr.cr;
        outLayout->yStride = ycbcr.ystride;
        outLayout->cStride = ycbcr.cstride;
        outLayout->chromaStep = ycbcr.chroma_step;
        return Error::NONE;
    }

    Error unlock(const native_handle_t* bufferHandle, base::unique_fd* outFenceFd) override {
        int result;
        int fenceFd = -1;
        if (mMinor >= 3 && mModule->unlockAsync) {
            result = mModule->unlockAsync(mModule, bufferHandle, &fenceFd);
        } else {
            result = mModule->unlock(mModule, bufferHandle);
        }

        // we always own the fenceFd even when unlock failed
        outFenceFd->reset(fenceFd);
        return result ? Error::BAD_VALUE : Error::NONE;
    }

   protected:
    virtual uint64_t getValidBufferUsageMask() const {
        return BufferUsage::CPU_READ_MASK | BufferUsage::CPU_WRITE_MASK | BufferUsage::GPU_TEXTURE |
               BufferUsage::GPU_RENDER_TARGET | BufferUsage::COMPOSER_OVERLAY |
               BufferUsage::COMPOSER_CLIENT_TARGET | BufferUsage::PROTECTED |
               BufferUsage::COMPOSER_CURSOR | BufferUsage::VIDEO_ENCODER |
               BufferUsage::CAMERA_OUTPUT | BufferUsage::CAMERA_INPUT | BufferUsage::RENDERSCRIPT |
               BufferUsage::VIDEO_DECODER | BufferUsage::SENSOR_DIRECT_DATA |
               BufferUsage::GPU_DATA_BUFFER | BufferUsage::VENDOR_MASK;
    }

    static void waitFenceFd(const base::unique_fd& fenceFd, const char* logname) {
        if (fenceFd < 0) {
            return;
        }

        const int warningTimeout = 3500;
        const int error = sync_wait(fenceFd, warningTimeout);
        if (error < 0 && errno == ETIME) {
            ALOGE("%s: fence %d didn't signal in %u ms", logname, fenceFd.get(), warningTimeout);
            sync_wait(fenceFd, -1);
        }
    }

    const gralloc_module_t* mModule = nullptr;
    uint8_t mMinor = 0;
};

}  // namespace detail

using Gralloc0Hal = detail::Gralloc0HalImpl<hal::MapperHal>;

}  // namespace passthrough
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
