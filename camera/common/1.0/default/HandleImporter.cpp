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

HandleImporter HandleImporter::sHandleImporter;

HandleImporter& HandleImporter::getInstance() {
    sHandleImporter.initialize();
    return sHandleImporter;
}

bool HandleImporter::initialize() {
    // allow only one client
    if (mInitialized) {
        return false;
    }

    if (!openGralloc()) {
        return false;
    }

    mInitialized = true;
    return true;
}

void HandleImporter::cleanup() {
    if (!mInitialized) {
        return;
    }

    closeGralloc();
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

    buffer_handle_t clone = cloneBuffer(handle);
    if (!clone) {
        return false;
    }

    handle = clone;
    return true;
}

void HandleImporter::freeBuffer(buffer_handle_t handle) {
    if (!handle) {
        return;
    }

    releaseBuffer(handle);
}

bool HandleImporter::importFence(const native_handle_t* handle, int& fd) {
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

void HandleImporter::closeFence(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

bool HandleImporter::openGralloc() {
    const hw_module_t* module;
    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (err) {
        ALOGE("failed to get gralloc module");
        return false;
    }

    uint8_t major = (module->module_api_version >> 8) & 0xff;
    if (major > 1) {
        ALOGE("unknown gralloc module major version %d", major);
        return false;
    }

    if (major == 1) {
        err = gralloc1_open(module, &mDevice);
        if (err) {
            ALOGE("failed to open gralloc1 device");
            return false;
        }

        mRetain = reinterpret_cast<GRALLOC1_PFN_RETAIN>(
                mDevice->getFunction(mDevice, GRALLOC1_FUNCTION_RETAIN));
        mRelease = reinterpret_cast<GRALLOC1_PFN_RELEASE>(
                mDevice->getFunction(mDevice, GRALLOC1_FUNCTION_RELEASE));
        if (!mRetain || !mRelease) {
            ALOGE("invalid gralloc1 device");
            gralloc1_close(mDevice);
            return false;
        }
    } else {
        mModule = reinterpret_cast<const gralloc_module_t*>(module);
    }

    return true;
}

void HandleImporter::closeGralloc() {
    if (mDevice) {
        gralloc1_close(mDevice);
    }
}

buffer_handle_t HandleImporter::cloneBuffer(buffer_handle_t handle) {
    native_handle_t* clone = native_handle_clone(handle);
    if (!clone) {
        ALOGE("failed to clone buffer %p", handle);
        return nullptr;
    }

    bool err;
    if (mDevice) {
        err = (mRetain(mDevice, clone) != GRALLOC1_ERROR_NONE);
    } else {
        err = (mModule->registerBuffer(mModule, clone) != 0);
    }

    if (err) {
        ALOGE("failed to retain/register buffer %p", clone);
        native_handle_close(clone);
        native_handle_delete(clone);
        return nullptr;
    }

    return clone;
}

void HandleImporter::releaseBuffer(buffer_handle_t handle) {
    if (mDevice) {
        mRelease(mDevice, handle);
    } else {
        mModule->unregisterBuffer(mModule, handle);
    }
    native_handle_close(handle);
    native_handle_delete(const_cast<native_handle_t*>(handle));
}

} // namespace helper
} // namespace V1_0
} // namespace common
} // namespace camera
} // namespace hardware
} // namespace android
