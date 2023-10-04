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

#include <aidl/android/hardware/graphics/common/Smpte2086.h>
#include <gralloctypes/Gralloc4.h>
#include <log/log.h>
#include <ui/GraphicBufferMapper.h>

namespace android {
namespace hardware {
namespace camera {
namespace common {
namespace helper {

using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::PlaneLayoutComponent;
using aidl::android::hardware::graphics::common::PlaneLayoutComponentType;
using aidl::android::hardware::graphics::common::Smpte2086;

HandleImporter::HandleImporter() : mInitialized(false) {}

void HandleImporter::initializeLocked() {
    if (mInitialized) {
        return;
    }

    GraphicBufferMapper::preloadHal();
    mInitialized = true;
    return;
}

void HandleImporter::cleanup() {
    mInitialized = false;
}

bool HandleImporter::importBufferInternal(buffer_handle_t& handle) {
    buffer_handle_t importedHandle;
    auto status = GraphicBufferMapper::get().importBufferNoValidate(handle, &importedHandle);
    if (status != OK) {
        ALOGE("%s: mapper importBuffer failed: %d", __FUNCTION__, status);
        return false;
    }

    handle = importedHandle;
    return true;
}

android_ycbcr HandleImporter::lockYCbCr(buffer_handle_t& buf, uint64_t cpuUsage,
                                        const android::Rect& accessRegion) {
    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }
    android_ycbcr layout;

    status_t status = GraphicBufferMapper::get().lockYCbCr(buf, cpuUsage, accessRegion, &layout);

    if (status != OK) {
        ALOGE("%s: failed to lockYCbCr error %d!", __FUNCTION__, status);
    }

    return layout;
}

std::vector<PlaneLayout> getPlaneLayouts(buffer_handle_t& buf) {
    std::vector<PlaneLayout> planeLayouts;
    status_t status = GraphicBufferMapper::get().getPlaneLayouts(buf, &planeLayouts);
    if (status != OK) {
        ALOGE("%s: failed to get PlaneLayouts! Status %d", __FUNCTION__, status);
    }

    return planeLayouts;
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

    return importBufferInternal(handle);
}

void HandleImporter::freeBuffer(buffer_handle_t handle) {
    if (!handle) {
        return;
    }

    Mutex::Autolock lock(mLock);
    if (!mInitialized) {
        initializeLocked();
    }

    status_t status = GraphicBufferMapper::get().freeBuffer(handle);
    if (status != OK) {
        ALOGE("%s: mapper freeBuffer failed. Status %d", __FUNCTION__, status);
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
        ALOGE("invalid fence handle with %d file descriptors", handle->numFds);
        return false;
    }

    return true;
}

void HandleImporter::closeFence(int fd) const {
    if (fd >= 0) {
        close(fd);
    }
}

void* HandleImporter::lock(buffer_handle_t& buf, uint64_t cpuUsage, size_t size) {
    android::Rect accessRegion{0, 0, static_cast<int>(size), 1};
    return lock(buf, cpuUsage, accessRegion);
}

void* HandleImporter::lock(buffer_handle_t& buf, uint64_t cpuUsage,
                           const android::Rect& accessRegion) {
    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }

    void* ret = nullptr;
    status_t status = GraphicBufferMapper::get().lock(buf, cpuUsage, accessRegion, &ret);
    if (status != OK) {
        ALOGE("%s: failed to lock error %d!", __FUNCTION__, status);
    }

    ALOGV("%s: ptr %p accessRegion.top: %d accessRegion.left: %d accessRegion.width: %d "
          "accessRegion.height: %d",
          __FUNCTION__, ret, accessRegion.top, accessRegion.left, accessRegion.width(),
          accessRegion.height());
    return ret;
}

status_t HandleImporter::getMonoPlanarStrideBytes(buffer_handle_t& buf, uint32_t* stride /*out*/) {
    if (stride == nullptr) {
        return BAD_VALUE;
    }

    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }

    std::vector<PlaneLayout> planeLayouts = getPlaneLayouts(buf);
    if (planeLayouts.size() != 1) {
        ALOGE("%s: Unexpected number of planes %zu!", __FUNCTION__, planeLayouts.size());
        return BAD_VALUE;
    }

    *stride = planeLayouts[0].strideInBytes;

    return OK;
}

int HandleImporter::unlock(buffer_handle_t& buf) {
    int releaseFence = -1;

    status_t status = GraphicBufferMapper::get().unlockAsync(buf, &releaseFence);
    if (status != OK) {
        ALOGE("%s: failed to unlock error %d!", __FUNCTION__, status);
    }

    return releaseFence;
}

bool HandleImporter::isSmpte2086Present(const buffer_handle_t& buf) {
    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }
    std::optional<ui::Smpte2086> metadata;
    status_t status = GraphicBufferMapper::get().getSmpte2086(buf, &metadata);
    if (status != OK) {
        ALOGE("%s: Mapper failed to get Smpte2094_40 metadata! Status: %d", __FUNCTION__, status);
        return false;
    }

    return metadata.has_value();
}

bool HandleImporter::isSmpte2094_10Present(const buffer_handle_t& buf) {
    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }

    std::optional<std::vector<uint8_t>> metadata;
    status_t status = GraphicBufferMapper::get().getSmpte2094_10(buf, &metadata);
    if (status != OK) {
        ALOGE("%s: Mapper failed to get Smpte2094_40 metadata! Status: %d", __FUNCTION__, status);
        return false;
    }

    return metadata.has_value();
}

bool HandleImporter::isSmpte2094_40Present(const buffer_handle_t& buf) {
    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }

    std::optional<std::vector<uint8_t>> metadata;
    status_t status = GraphicBufferMapper::get().getSmpte2094_40(buf, &metadata);
    if (status != OK) {
        ALOGE("%s: Mapper failed to get Smpte2094_40 metadata! Status: %d", __FUNCTION__, status);
        return false;
    }

    return metadata.has_value();
}

}  // namespace helper
}  // namespace common
}  // namespace camera
}  // namespace hardware
}  // namespace android
