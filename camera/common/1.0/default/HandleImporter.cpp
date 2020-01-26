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

using MapperErrorV2 = android::hardware::graphics::mapper::V2_0::Error;
using MapperErrorV3 = android::hardware::graphics::mapper::V3_0::Error;
using MapperErrorV4 = android::hardware::graphics::mapper::V4_0::Error;
using IMapperV3 = android::hardware::graphics::mapper::V3_0::IMapper;
using IMapperV4 = android::hardware::graphics::mapper::V4_0::IMapper;

HandleImporter::HandleImporter() : mInitialized(false) {}

void HandleImporter::initializeLocked() {
    if (mInitialized) {
        return;
    }

    mMapperV4 = IMapperV4::getService();
    if (mMapperV4 != nullptr) {
        mInitialized = true;
        return;
    }

    mMapperV3 = IMapperV3::getService();
    if (mMapperV3 != nullptr) {
        mInitialized = true;
        return;
    }

    mMapperV2 = IMapper::getService();
    if (mMapperV2 == nullptr) {
        ALOGE("%s: cannnot acccess graphics mapper HAL!", __FUNCTION__);
        return;
    }

    mInitialized = true;
    return;
}

void HandleImporter::cleanup() {
    mMapperV4.clear();
    mMapperV3.clear();
    mMapperV2.clear();
    mInitialized = false;
}

template<class M, class E>
bool HandleImporter::importBufferInternal(const sp<M> mapper, buffer_handle_t& handle) {
    E error;
    buffer_handle_t importedHandle;
    auto ret = mapper->importBuffer(
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

    if (error != E::NONE) {
        return false;
    }

    handle = importedHandle;
    return true;
}

template<class M, class E>
YCbCrLayout HandleImporter::lockYCbCrInternal(const sp<M> mapper, buffer_handle_t& buf,
        uint64_t cpuUsage, const IMapper::Rect& accessRegion) {
    hidl_handle acquireFenceHandle;
    auto buffer = const_cast<native_handle_t*>(buf);
    YCbCrLayout layout = {};

    typename M::Rect accessRegionCopy = {accessRegion.left, accessRegion.top,
            accessRegion.width, accessRegion.height};
    mapper->lockYCbCr(buffer, cpuUsage, accessRegionCopy, acquireFenceHandle,
            [&](const auto& tmpError, const auto& tmpLayout) {
                if (tmpError == E::NONE) {
                    // Member by member copy from different versions of YCbCrLayout.
                    layout.y = tmpLayout.y;
                    layout.cb = tmpLayout.cb;
                    layout.cr = tmpLayout.cr;
                    layout.yStride = tmpLayout.yStride;
                    layout.cStride = tmpLayout.cStride;
                    layout.chromaStep = tmpLayout.chromaStep;
                } else {
                    ALOGE("%s: failed to lockYCbCr error %d!", __FUNCTION__, tmpError);
                }
           });
    return layout;
}

template<class M, class E>
int HandleImporter::unlockInternal(const sp<M> mapper, buffer_handle_t& buf) {
    int releaseFence = -1;
    auto buffer = const_cast<native_handle_t*>(buf);

    mapper->unlock(
        buffer, [&](const auto& tmpError, const auto& tmpReleaseFence) {
            if (tmpError == E::NONE) {
                auto fenceHandle = tmpReleaseFence.getNativeHandle();
                if (fenceHandle) {
                    if (fenceHandle->numInts != 0 || fenceHandle->numFds != 1) {
                        ALOGE("%s: bad release fence numInts %d numFds %d",
                                __FUNCTION__, fenceHandle->numInts, fenceHandle->numFds);
                        return;
                    }
                    releaseFence = dup(fenceHandle->data[0]);
                    if (releaseFence < 0) {
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

    if (mMapperV4 != nullptr) {
        return importBufferInternal<IMapperV4, MapperErrorV4>(mMapperV4, handle);
    }

    if (mMapperV3 != nullptr) {
        return importBufferInternal<IMapperV3, MapperErrorV3>(mMapperV3, handle);
    }

    if (mMapperV2 != nullptr) {
        return importBufferInternal<IMapper, MapperErrorV2>(mMapperV2, handle);
    }

    ALOGE("%s: mMapperV4, mMapperV3 and mMapperV2 are all null!", __FUNCTION__);
    return false;
}

void HandleImporter::freeBuffer(buffer_handle_t handle) {
    if (!handle) {
        return;
    }

    Mutex::Autolock lock(mLock);
    if (!mInitialized) {
        initializeLocked();
    }

    if (mMapperV4 != nullptr) {
        auto ret = mMapperV4->freeBuffer(const_cast<native_handle_t*>(handle));
        if (!ret.isOk()) {
            ALOGE("%s: mapper freeBuffer failed: %s", __FUNCTION__, ret.description().c_str());
        }
    } else if (mMapperV3 != nullptr) {
        auto ret = mMapperV3->freeBuffer(const_cast<native_handle_t*>(handle));
        if (!ret.isOk()) {
            ALOGE("%s: mapper freeBuffer failed: %s",
                    __FUNCTION__, ret.description().c_str());
        }
    } else {
        auto ret = mMapperV2->freeBuffer(const_cast<native_handle_t*>(handle));
        if (!ret.isOk()) {
            ALOGE("%s: mapper freeBuffer failed: %s",
                    __FUNCTION__, ret.description().c_str());
        }
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

    if (!mInitialized) {
        initializeLocked();
    }

    if (mMapperV4 == nullptr && mMapperV3 == nullptr && mMapperV2 == nullptr) {
        ALOGE("%s: mMapperV4, mMapperV3 and mMapperV2 are all null!", __FUNCTION__);
        return ret;
    }

    hidl_handle acquireFenceHandle;
    auto buffer = const_cast<native_handle_t*>(buf);
    if (mMapperV4 != nullptr) {
        IMapperV4::Rect accessRegion{0, 0, static_cast<int>(size), 1};
        // No need to use bytesPerPixel and bytesPerStride because we are using
        // an 1-D buffer and accressRegion.
        mMapperV4->lock(buffer, cpuUsage, accessRegion, acquireFenceHandle,
                        [&](const auto& tmpError, const auto& tmpPtr) {
                            if (tmpError == MapperErrorV4::NONE) {
                                ret = tmpPtr;
                            } else {
                                ALOGE("%s: failed to lock error %d!", __FUNCTION__, tmpError);
                            }
                        });
    } else if (mMapperV3 != nullptr) {
        IMapperV3::Rect accessRegion { 0, 0, static_cast<int>(size), 1 };
        // No need to use bytesPerPixel and bytesPerStride because we are using
        // an 1-D buffer and accressRegion.
        mMapperV3->lock(buffer, cpuUsage, accessRegion, acquireFenceHandle,
                [&](const auto& tmpError, const auto& tmpPtr, const auto& /*bytesPerPixel*/,
                        const auto& /*bytesPerStride*/) {
                    if (tmpError == MapperErrorV3::NONE) {
                        ret = tmpPtr;
                    } else {
                        ALOGE("%s: failed to lock error %d!",
                              __FUNCTION__, tmpError);
                    }
               });
    } else {
        IMapper::Rect accessRegion { 0, 0, static_cast<int>(size), 1 };
        mMapperV2->lock(buffer, cpuUsage, accessRegion, acquireFenceHandle,
                [&](const auto& tmpError, const auto& tmpPtr) {
                    if (tmpError == MapperErrorV2::NONE) {
                        ret = tmpPtr;
                    } else {
                        ALOGE("%s: failed to lock error %d!",
                              __FUNCTION__, tmpError);
                    }
               });
    }

    ALOGV("%s: ptr %p size: %zu", __FUNCTION__, ret, size);
    return ret;
}

YCbCrLayout HandleImporter::lockYCbCr(
        buffer_handle_t& buf, uint64_t cpuUsage,
        const IMapper::Rect& accessRegion) {
    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }

    if (mMapperV4 != nullptr) {
        // No device currently supports IMapper 4.0 so it is safe to just return an error code here.
        //
        // This will be supported by a combination of lock and BufferMetadata getters. We are going
        // to refactor all the IAllocator/IMapper versioning code into a shared library. We will
        // then add the IMapper 4.0 lockYCbCr support then.
        ALOGE("%s: MapperV4 doesn't support lockYCbCr directly!", __FUNCTION__);
        return {};
    }

    if (mMapperV3 != nullptr) {
        return lockYCbCrInternal<IMapperV3, MapperErrorV3>(
                mMapperV3, buf, cpuUsage, accessRegion);
    }

    if (mMapperV2 != nullptr) {
        return lockYCbCrInternal<IMapper, MapperErrorV2>(
                mMapperV2, buf, cpuUsage, accessRegion);
    }

    ALOGE("%s: mMapperV4, mMapperV3 and mMapperV2 are all null!", __FUNCTION__);
    return {};
}

int HandleImporter::unlock(buffer_handle_t& buf) {
    if (mMapperV4 != nullptr) {
        return unlockInternal<IMapperV4, MapperErrorV4>(mMapperV4, buf);
    }
    if (mMapperV3 != nullptr) {
        return unlockInternal<IMapperV3, MapperErrorV3>(mMapperV3, buf);
    }
    if (mMapperV2 != nullptr) {
        return unlockInternal<IMapper, MapperErrorV2>(mMapperV2, buf);
    }

    ALOGE("%s: mMapperV4, mMapperV3 and mMapperV2 are all null!", __FUNCTION__);
    return -1;
}

} // namespace helper
} // namespace V1_0
} // namespace common
} // namespace camera
} // namespace hardware
} // namespace android
