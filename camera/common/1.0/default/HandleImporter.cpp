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

#include <gralloctypes/Gralloc4.h>
#include "aidl/android/hardware/graphics/common/Smpte2086.h"
#include <log/log.h>

namespace android {
namespace hardware {
namespace camera {
namespace common {
namespace V1_0 {
namespace helper {

using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::PlaneLayoutComponent;
using aidl::android::hardware::graphics::common::PlaneLayoutComponentType;
using aidl::android::hardware::graphics::common::Smpte2086;
using MetadataType = android::hardware::graphics::mapper::V4_0::IMapper::MetadataType;
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

bool isMetadataPesent(const sp<IMapperV4> mapper, const buffer_handle_t& buf,
        MetadataType metadataType) {
    auto buffer = const_cast<native_handle_t*>(buf);
    bool ret = false;
    hidl_vec<uint8_t> vec;
    mapper->get(buffer, metadataType, [&] (const auto& tmpError,
                const auto& tmpMetadata) {
                    if (tmpError == MapperErrorV4::NONE) {
                        vec = tmpMetadata;
                    } else {
                        ALOGE("%s: failed to get metadata %d!", __FUNCTION__, tmpError);
                    }});

    if (vec.size() > 0) {
            if (metadataType == gralloc4::MetadataType_Smpte2086){
                std::optional<Smpte2086> realSmpte2086;
                gralloc4::decodeSmpte2086(vec, &realSmpte2086);
                ret = realSmpte2086.has_value();
            } else if (metadataType == gralloc4::MetadataType_Smpte2094_10) {
                std::optional<std::vector<uint8_t>> realSmpte2094_10;
                gralloc4::decodeSmpte2094_10(vec, &realSmpte2094_10);
                ret = realSmpte2094_10.has_value();
            } else if (metadataType == gralloc4::MetadataType_Smpte2094_40) {
                std::optional<std::vector<uint8_t>> realSmpte2094_40;
                gralloc4::decodeSmpte2094_40(vec, &realSmpte2094_40);
                ret = realSmpte2094_40.has_value();
            } else {
                ALOGE("%s: Unknown metadata type!", __FUNCTION__);
            }
    }

    return ret;
}

std::vector<PlaneLayout> getPlaneLayouts(const sp<IMapperV4> mapper, buffer_handle_t& buf) {
    auto buffer = const_cast<native_handle_t*>(buf);
    std::vector<PlaneLayout> planeLayouts;
    hidl_vec<uint8_t> encodedPlaneLayouts;
    mapper->get(buffer, gralloc4::MetadataType_PlaneLayouts,
                [&](const auto& tmpError, const auto& tmpEncodedPlaneLayouts) {
                    if (tmpError == MapperErrorV4::NONE) {
                        encodedPlaneLayouts = tmpEncodedPlaneLayouts;
                    } else {
                        ALOGE("%s: failed to get plane layouts %d!", __FUNCTION__, tmpError);
                    }
                });

    gralloc4::decodePlaneLayouts(encodedPlaneLayouts, &planeLayouts);

    return planeLayouts;
}

template <>
YCbCrLayout HandleImporter::lockYCbCrInternal<IMapperV4, MapperErrorV4>(
        const sp<IMapperV4> mapper, buffer_handle_t& buf, uint64_t cpuUsage,
        const IMapper::Rect& accessRegion) {
    hidl_handle acquireFenceHandle;
    auto buffer = const_cast<native_handle_t*>(buf);
    YCbCrLayout layout = {};
    void* mapped = nullptr;

    typename IMapperV4::Rect accessRegionV4 = {accessRegion.left, accessRegion.top,
                                               accessRegion.width, accessRegion.height};
    mapper->lock(buffer, cpuUsage, accessRegionV4, acquireFenceHandle,
                 [&](const auto& tmpError, const auto& tmpPtr) {
                     if (tmpError == MapperErrorV4::NONE) {
                         mapped = tmpPtr;
                     } else {
                         ALOGE("%s: failed to lock error %d!", __FUNCTION__, tmpError);
                     }
                 });

    if (mapped == nullptr) {
        return layout;
    }

    std::vector<PlaneLayout> planeLayouts = getPlaneLayouts(mapper, buf);
    for (const auto& planeLayout : planeLayouts) {
        for (const auto& planeLayoutComponent : planeLayout.components) {
            const auto& type = planeLayoutComponent.type;

            if (!gralloc4::isStandardPlaneLayoutComponentType(type)) {
                continue;
            }

            uint8_t* data = reinterpret_cast<uint8_t*>(mapped);
            data += planeLayout.offsetInBytes;
            data += planeLayoutComponent.offsetInBits / 8;

            switch (static_cast<PlaneLayoutComponentType>(type.value)) {
                case PlaneLayoutComponentType::Y:
                    layout.y = data;
                    layout.yStride = planeLayout.strideInBytes;
                    break;
                case PlaneLayoutComponentType::CB:
                    layout.cb = data;
                    layout.cStride = planeLayout.strideInBytes;
                    layout.chromaStep = planeLayout.sampleIncrementInBits / 8;
                    break;
                case PlaneLayoutComponentType::CR:
                    layout.cr = data;
                    layout.cStride = planeLayout.strideInBytes;
                    layout.chromaStep = planeLayout.sampleIncrementInBits / 8;
                    break;
                default:
                    break;
            }
        }
    }

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
    IMapper::Rect accessRegion{0, 0, static_cast<int>(size), 1};
    return lock(buf, cpuUsage, accessRegion);
}

void* HandleImporter::lock(buffer_handle_t& buf, uint64_t cpuUsage,
                           const IMapper::Rect& accessRegion) {
    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }

    void* ret = nullptr;

    if (mMapperV4 == nullptr && mMapperV3 == nullptr && mMapperV2 == nullptr) {
        ALOGE("%s: mMapperV4, mMapperV3 and mMapperV2 are all null!", __FUNCTION__);
        return ret;
    }

    hidl_handle acquireFenceHandle;
    auto buffer = const_cast<native_handle_t*>(buf);
    if (mMapperV4 != nullptr) {
        IMapperV4::Rect accessRegionV4{accessRegion.left, accessRegion.top, accessRegion.width,
                                       accessRegion.height};

        mMapperV4->lock(buffer, cpuUsage, accessRegionV4, acquireFenceHandle,
                        [&](const auto& tmpError, const auto& tmpPtr) {
                            if (tmpError == MapperErrorV4::NONE) {
                                ret = tmpPtr;
                            } else {
                                ALOGE("%s: failed to lock error %d!", __FUNCTION__, tmpError);
                            }
                        });
    } else if (mMapperV3 != nullptr) {
        IMapperV3::Rect accessRegionV3{accessRegion.left, accessRegion.top, accessRegion.width,
                                       accessRegion.height};

        mMapperV3->lock(buffer, cpuUsage, accessRegionV3, acquireFenceHandle,
                        [&](const auto& tmpError, const auto& tmpPtr, const auto& /*bytesPerPixel*/,
                            const auto& /*bytesPerStride*/) {
                            if (tmpError == MapperErrorV3::NONE) {
                                ret = tmpPtr;
                            } else {
                                ALOGE("%s: failed to lock error %d!", __FUNCTION__, tmpError);
                            }
                        });
    } else {
        mMapperV2->lock(buffer, cpuUsage, accessRegion, acquireFenceHandle,
                [&](const auto& tmpError, const auto& tmpPtr) {
                    if (tmpError == MapperErrorV2::NONE) {
                        ret = tmpPtr;
                    } else {
                        ALOGE("%s: failed to lock error %d!", __FUNCTION__, tmpError);
                    }
               });
    }

    ALOGV("%s: ptr %p accessRegion.top: %d accessRegion.left: %d accessRegion.width: %d "
          "accessRegion.height: %d",
          __FUNCTION__, ret, accessRegion.top, accessRegion.left, accessRegion.width,
          accessRegion.height);
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
        return lockYCbCrInternal<IMapperV4, MapperErrorV4>(mMapperV4, buf, cpuUsage, accessRegion);
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

status_t HandleImporter::getMonoPlanarStrideBytes(buffer_handle_t &buf, uint32_t *stride /*out*/) {
    if (stride == nullptr) {
        return BAD_VALUE;
    }

    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }

    if (mMapperV4 != nullptr) {
        std::vector<PlaneLayout> planeLayouts = getPlaneLayouts(mMapperV4, buf);
        if (planeLayouts.size() != 1) {
            ALOGE("%s: Unexpected number of planes %zu!",  __FUNCTION__, planeLayouts.size());
            return BAD_VALUE;
        }

        *stride = planeLayouts[0].strideInBytes;
    } else {
        ALOGE("%s: mMapperV4 is null! Query not supported!", __FUNCTION__);
        return NO_INIT;
    }

    return OK;
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

bool HandleImporter::isSmpte2086Present(const buffer_handle_t& buf) {
    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }

    if (mMapperV4 != nullptr) {
        return isMetadataPesent(mMapperV4, buf, gralloc4::MetadataType_Smpte2086);
    } else {
        ALOGE("%s: mMapperV4 is null! Query not supported!", __FUNCTION__);
    }

    return false;
}

bool HandleImporter::isSmpte2094_10Present(const buffer_handle_t& buf) {
    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }

    if (mMapperV4 != nullptr) {
        return isMetadataPesent(mMapperV4, buf, gralloc4::MetadataType_Smpte2094_10);
    } else {
        ALOGE("%s: mMapperV4 is null! Query not supported!", __FUNCTION__);
    }

    return false;
}

bool HandleImporter::isSmpte2094_40Present(const buffer_handle_t& buf) {
    Mutex::Autolock lock(mLock);

    if (!mInitialized) {
        initializeLocked();
    }

    if (mMapperV4 != nullptr) {
        return isMetadataPesent(mMapperV4, buf, gralloc4::MetadataType_Smpte2094_40);
    } else {
        ALOGE("%s: mMapperV4 is null! Query not supported!", __FUNCTION__);
    }

    return false;
}


} // namespace helper
} // namespace V1_0
} // namespace common
} // namespace camera
} // namespace hardware
} // namespace android
