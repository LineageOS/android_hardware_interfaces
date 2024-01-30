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

#ifndef CAMERA_COMMON_1_0_HANDLEIMPORTED_H
#define CAMERA_COMMON_1_0_HANDLEIMPORTED_H

#include <cutils/native_handle.h>
#include <system/graphics.h>
#include <ui/Rect.h>
#include <utils/Mutex.h>

namespace android {
namespace hardware {
namespace camera {
namespace common {
namespace helper {

// Borrowed from graphics HAL. Use this until gralloc mapper HAL is working
class HandleImporter {
  public:
    HandleImporter();

    // In IComposer, any buffer_handle_t is owned by the caller and we need to
    // make a clone for hwcomposer2.  We also need to translate empty handle
    // to nullptr.  This function does that, in-place.
    bool importBuffer(buffer_handle_t& handle);
    void freeBuffer(buffer_handle_t handle);
    bool importFence(const native_handle_t* handle, int& fd) const;
    void closeFence(int fd) const;

    // Locks 1-D buffer. Assumes caller has waited for acquire fences.
    void* lock(buffer_handle_t& buf, uint64_t cpuUsage, size_t size);

    // Locks 2-D buffer. Assumes caller has waited for acquire fences.
    void* lock(buffer_handle_t& buf, uint64_t cpuUsage, const android::Rect& accessRegion);

    // Assumes caller has waited for acquire fences.
    android_ycbcr lockYCbCr(buffer_handle_t& buf, uint64_t cpuUsage,
                            const android::Rect& accessRegion);

    // Query the stride of the first plane in bytes.
    status_t getMonoPlanarStrideBytes(buffer_handle_t& buf, uint32_t* stride /*out*/);

    int unlock(buffer_handle_t& buf);  // returns release fence

    // Query Gralloc4 metadata
    bool isSmpte2086Present(const buffer_handle_t& buf);
    bool isSmpte2094_10Present(const buffer_handle_t& buf);
    bool isSmpte2094_40Present(const buffer_handle_t& buf);

  private:
    void initializeLocked();
    void cleanup();

    bool importBufferInternal(buffer_handle_t& handle);
    int unlockInternal(buffer_handle_t& buf);

    Mutex mLock;
    bool mInitialized;
};

}  // namespace helper

// NOTE: Deprecated namespace. This namespace should no longer be used for the following symbols
namespace V1_0::helper {
// Export symbols to the old namespace to preserve compatibility
typedef android::hardware::camera::common::helper::HandleImporter HandleImporter;
}  // namespace V1_0::helper

}  // namespace common
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // CAMERA_COMMON_1_0_HANDLEIMPORTED_H
