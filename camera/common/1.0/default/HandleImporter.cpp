/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "HandleImporter"
#include "HandleImporter.h"
#include <log/log.h>

namespace android {
namespace hardware {
namespace camera {
namespace common {
namespace V1_0 {
namespace helper {

using MapperError = android::hardware::graphics::mapper::V2_0::Error;

HandleImporter::HandleImporter() : mInitialized(false) {}

void HandleImporter::initializeLocked() {
    if (mInitialized) {
        return;
    }

    mMapper = IMapper::getService();
    if (mMapper == nullptr) {
        ALOGE("%s: cannnot acccess graphics mapper HAL!", __FUNCTION__);
        return;
    }

    mInitialized = true;
    return;
}

void HandleImporter::cleanup() {
    mMapper.clear();
    mInitialized = false;
}

// In IComposer, any buffer_handle_t is owned by the caller and we need to
// make a clone for hwcomposer2.  We also need to translate empty handle
// to nullptr.  This function does that, in-place.
bool HandleImporter::importBuffer(buffer_handle_t& handle) {
    if (!handle->numFds && !handle->numInts) {
        handle = nullptr;
        return true;
    }

    Mutex::Autolock lock(mLock);
    if (!mInitialized) {
        initializeLocked();
    }

    if (mMapper == nullptr) {
        ALOGE("%s: mMapper is null!", __FUNCTION__);
        return false;
    }

    MapperError error;
    buffer_handle_t importedHandle;
    auto ret = mMapper->importBuffer(
        hidl_handle(handle),
        [&](const auto& tmpError, const auto& tmpBufferHandle) {
            error = tmpError;
            importedHandle = static_cast<buffer_handle_t>(tmpBufferHandle);
        });

    if (!ret.isOk()) {
        ALOGE("%s: mapper importBuffer failed: %s",
                __FUNCTION__, ret.description().c_str());
        return false;
    }

    if (error != MapperError::NONE) {
        return false;
    }

    handle = importedHandle;

    return true;
}

void HandleImporter::freeBuffer(buffer_handle_t handle) {
    if (!handle) {
        return;
    }

    Mutex::Autolock lock(mLock);
    if (mMapper == nullptr) {
        ALOGE("%s: mMapper is null!", __FUNCTION__);
        return;
    }

    auto ret = mMapper->freeBuffer(const_cast<native_handle_t*>(handle));
    if (!ret.isOk()) {
        ALOGE("%s: mapper freeBuffer failed: %s",
                __FUNCTION__, ret.description().c_str());
    }
}

bool HandleImporter::importFence(const native_handle_t* handle, int& fd) const {
    if (handle == nullptr || handle->numFds == 0) {
        fd = -1;
    } else if (handle->numFds == 1) {
        fd = dup(handle->data[0]);
        if (fd < 0) {
            ALOGE("failed to dup fence fd %d", handle->data[0]);
            return false;
        }
    } else {
        ALOGE("invalid fence handle with %d file descriptors",
                handle->numFds);
        return false;
    }

    return true;
}

void HandleImporter::closeFence(int fd) const {
    if (fd >= 0) {
        close(fd);
    }
}

void* HandleImporter::lock(
        buffer_handle_t& buf, uint64_t cpuUsage, size_t size) {
    Mutex::Autolock lock(mLock);
    void *ret = 0;
    IMapper::Rect accessRegion { 0, 0, static_cast<int>(size), 1 };

    if (!mInitialized) {
        initializeLocked();
    }

    if (mMapper == nullptr) {
        ALOGE("%s: mMapper is null!", __FUNCTION__);
        return ret;
    }

    hidl_handle acquireFenceHandle;
    auto buffer = const_cast<native_handle_t*>(buf);
    mMapper->lock(buffer, cpuUsage, accessRegion, acquireFenceHandle,
            [&](const auto& tmpError, const auto& tmpPtr) {
                if (tmpError == MapperError::NONE) {
                    ret = tmpPtr;
                } else {
                    ALOGE("%s: failed to lock error %d!",
                          __FUNCTION__, tmpError);
                }
           });

    ALOGV("%s: ptr %p size: %zu", __FUNCTION__, ret, size);
    return ret;
}


YCbCrLayout HandleImporter::lockYCbCr(
        buffer_handle_t& buf, uint64_t cpuUsage,
        const IMapper::Rect& accessRegion) {
    Mutex::Autolock lock(mLock);
    YCbCrLayout layout = {};

    if (!mInitialized) {
        initializeLocked();
    }

    if (mMapper == nullptr) {
        ALOGE("%s: mMapper is null!", __FUNCTION__);
        return layout;
    }

    hidl_handle acquireFenceHandle;
    auto buffer = const_cast<native_handle_t*>(buf);
    mMapper->lockYCbCr(buffer, cpuUsage, accessRegion, acquireFenceHandle,
            [&](const auto& tmpError, const auto& tmpLayout) {
                if (tmpError == MapperError::NONE) {
                    layout = tmpLayout;
                } else {
                    ALOGE("%s: failed to lockYCbCr error %d!", __FUNCTION__, tmpError);
                }
           });

    ALOGV("%s: layout y %p cb %p cr %p y_str %d c_str %d c_step %d",
            __FUNCTION__, layout.y, layout.cb, layout.cr,
            layout.yStride, layout.cStride, layout.chromaStep);
    return layout;
}

int HandleImporter::unlock(buffer_handle_t& buf) {
    int releaseFence = -1;
    auto buffer = const_cast<native_handle_t*>(buf);
    mMapper->unlock(
        buffer, [&](const auto& tmpError, const auto& tmpReleaseFence) {
            if (tmpError == MapperError::NONE) {
                auto fenceHandle = tmpReleaseFence.getNativeHandle();
                if (fenceHandle) {
                    if (fenceHandle->numInts != 0 || fenceHandle->numFds != 1) {
                        ALOGE("%s: bad release fence numInts %d numFds %d",
                                __FUNCTION__, fenceHandle->numInts, fenceHandle->numFds);
                        return;
                    }
                    releaseFence = dup(fenceHandle->data[0]);
                    if (releaseFence <= 0) {
                        ALOGE("%s: bad release fence FD %d",
                                __FUNCTION__, releaseFence);
                    }
                }
            } else {
                ALOGE("%s: failed to unlock error %d!", __FUNCTION__, tmpError);
            }
        });

    return releaseFence;
}

} // namespace helper
} // namespace V1_0
} // namespace common
} // namespace camera
} // namespace hardware
} // namespace android
