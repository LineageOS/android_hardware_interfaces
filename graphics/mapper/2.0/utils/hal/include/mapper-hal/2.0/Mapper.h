/*
 * Copyright 2018 The Android Open Source Project
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
#warning "Mapper.h included without LOG_TAG"
#endif

#include <memory>

#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <log/log.h>
#include <mapper-hal/2.0/MapperHal.h>

namespace android {
namespace hardware {
namespace graphics {
namespace mapper {
namespace V2_0 {
namespace hal {

namespace detail {

// MapperImpl implements V2_*::IMapper on top of V2_*::hal::MapperHal
template <typename Interface, typename Hal>
class MapperImpl : public Interface {
   public:
    bool init(std::unique_ptr<Hal> hal) {
        mHal = std::move(hal);
        return true;
    }

    // IMapper 2.0 interface

    Return<void> createDescriptor(const V2_0::IMapper::BufferDescriptorInfo& descriptorInfo,
                                  IMapper::createDescriptor_cb hidl_cb) override {
        BufferDescriptor descriptor;
        Error error = mHal->createDescriptor(descriptorInfo, &descriptor);
        hidl_cb(error, descriptor);
        return Void();
    }

    Return<void> importBuffer(const hidl_handle& rawHandle,
                              IMapper::importBuffer_cb hidl_cb) override {
        if (!rawHandle.getNativeHandle()) {
            hidl_cb(Error::BAD_BUFFER, nullptr);
            return Void();
        }

        native_handle_t* bufferHandle = nullptr;
        Error error = mHal->importBuffer(rawHandle.getNativeHandle(), &bufferHandle);
        if (error != Error::NONE) {
            hidl_cb(error, nullptr);
            return Void();
        }

        void* buffer = addImportedBuffer(bufferHandle);
        if (!buffer) {
            mHal->freeBuffer(bufferHandle);
            hidl_cb(Error::NO_RESOURCES, nullptr);
            return Void();
        }

        hidl_cb(error, buffer);
        return Void();
    }

    Return<Error> freeBuffer(void* buffer) override {
        native_handle_t* bufferHandle = removeImportedBuffer(buffer);
        if (!bufferHandle) {
            return Error::BAD_BUFFER;
        }

        return mHal->freeBuffer(bufferHandle);
    }

    Return<void> lock(void* buffer, uint64_t cpuUsage, const V2_0::IMapper::Rect& accessRegion,
                      const hidl_handle& acquireFence, IMapper::lock_cb hidl_cb) override {
        const native_handle_t* bufferHandle = getImportedBuffer(buffer);
        if (!bufferHandle) {
            hidl_cb(Error::BAD_BUFFER, nullptr);
            return Void();
        }

        base::unique_fd fenceFd;
        Error error = getFenceFd(acquireFence, &fenceFd);
        if (error != Error::NONE) {
            hidl_cb(error, nullptr);
            return Void();
        }

        void* data = nullptr;
        error = mHal->lock(bufferHandle, cpuUsage, accessRegion, std::move(fenceFd), &data);
        hidl_cb(error, data);
        return Void();
    }

    Return<void> lockYCbCr(void* buffer, uint64_t cpuUsage, const V2_0::IMapper::Rect& accessRegion,
                           const hidl_handle& acquireFence,
                           IMapper::lockYCbCr_cb hidl_cb) override {
        const native_handle_t* bufferHandle = getImportedBuffer(buffer);
        if (!bufferHandle) {
            hidl_cb(Error::BAD_BUFFER, YCbCrLayout{});
            return Void();
        }

        base::unique_fd fenceFd;
        Error error = getFenceFd(acquireFence, &fenceFd);
        if (error != Error::NONE) {
            hidl_cb(error, YCbCrLayout{});
            return Void();
        }

        YCbCrLayout layout{};
        error = mHal->lockYCbCr(bufferHandle, cpuUsage, accessRegion, std::move(fenceFd), &layout);
        hidl_cb(error, layout);
        return Void();
    }

    Return<void> unlock(void* buffer, IMapper::unlock_cb hidl_cb) override {
        const native_handle_t* bufferHandle = getImportedBuffer(buffer);
        if (!bufferHandle) {
            hidl_cb(Error::BAD_BUFFER, nullptr);
            return Void();
        }

        base::unique_fd fenceFd;
        Error error = mHal->unlock(bufferHandle, &fenceFd);
        if (error != Error::NONE) {
            hidl_cb(error, nullptr);
            return Void();
        }

        NATIVE_HANDLE_DECLARE_STORAGE(fenceStorage, 1, 0);
        hidl_cb(error, getFenceHandle(fenceFd, fenceStorage));
        return Void();
    }

   protected:
    // these functions can be overriden to do true imported buffer management
    virtual void* addImportedBuffer(native_handle_t* bufferHandle) {
        return static_cast<void*>(bufferHandle);
    }

    virtual native_handle_t* removeImportedBuffer(void* buffer) {
        return static_cast<native_handle_t*>(buffer);
    }

    virtual const native_handle_t* getImportedBuffer(void* buffer) const {
        return static_cast<const native_handle_t*>(buffer);
    }

    // convert fenceFd to or from hidl_handle
    static Error getFenceFd(const hidl_handle& fenceHandle, base::unique_fd* outFenceFd) {
        auto handle = fenceHandle.getNativeHandle();
        if (handle && handle->numFds > 1) {
            ALOGE("invalid fence handle with %d fds", handle->numFds);
            return Error::BAD_VALUE;
        }

        int fenceFd = (handle && handle->numFds == 1) ? handle->data[0] : -1;
        if (fenceFd >= 0) {
            fenceFd = dup(fenceFd);
            if (fenceFd < 0) {
                return Error::NO_RESOURCES;
            }
        }

        outFenceFd->reset(fenceFd);

        return Error::NONE;
    }

    static hidl_handle getFenceHandle(const base::unique_fd& fenceFd, char* handleStorage) {
        native_handle_t* handle = nullptr;
        if (fenceFd >= 0) {
            handle = native_handle_init(handleStorage, 1, 0);
            handle->data[0] = fenceFd;
        }

        return hidl_handle(handle);
    }

    std::unique_ptr<Hal> mHal;
};

}  // namespace detail

using Mapper = detail::MapperImpl<IMapper, MapperHal>;

}  // namespace hal
}  // namespace V2_0
}  // namespace mapper
}  // namespace graphics
}  // namespace hardware
}  // namespace android
