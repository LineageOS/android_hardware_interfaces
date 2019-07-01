/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.1
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_CONFIGSTORE_V1_1_SURFACEFLINGERCONFIGS_H
#define ANDROID_HARDWARE_CONFIGSTORE_V1_1_SURFACEFLINGERCONFIGS_H

#include <android/hardware/configstore/1.1/ISurfaceFlingerConfigs.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace configstore {
namespace V1_1 {
namespace implementation {

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::configstore::V1_1::ISurfaceFlingerConfigs;

struct SurfaceFlingerConfigs : public ISurfaceFlingerConfigs {
    // ::android::hardware::configstore::V1_0::ISurfaceFlingerConfigs implementation.
    Return<void> vsyncEventPhaseOffsetNs(vsyncEventPhaseOffsetNs_cb _hidl_cb) override;
    Return<void> vsyncSfEventPhaseOffsetNs(vsyncSfEventPhaseOffsetNs_cb _hidl_cb) override;
    Return<void> useContextPriority(useContextPriority_cb _hidl_cb) override;
    Return<void> hasWideColorDisplay(hasWideColorDisplay_cb _hidl_cb) override;
    Return<void> hasHDRDisplay(hasHDRDisplay_cb _hidl_cb) override;
    Return<void> presentTimeOffsetFromVSyncNs(presentTimeOffsetFromVSyncNs_cb _hidl_cb) override;
    Return<void> useHwcForRGBtoYUV(useHwcForRGBtoYUV_cb _hidl_cb) override;
    Return<void> maxVirtualDisplaySize(maxVirtualDisplaySize_cb _hidl_cb) override;
    Return<void> hasSyncFramework(hasSyncFramework_cb _hidl_cb) override;
    Return<void> useVrFlinger(useVrFlinger_cb _hidl_cb) override;
    Return<void> maxFrameBufferAcquiredBuffers(maxFrameBufferAcquiredBuffers_cb _hidl_cb) override;
    Return<void> startGraphicsAllocatorService(startGraphicsAllocatorService_cb _hidl_cb) override;

    // ::android::hardware::configstore::V1_1::ISurfaceFlingerConfigs follow implementation.
    Return<void> primaryDisplayOrientation(primaryDisplayOrientation_cb _hidl_cb) override;
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace configstore
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CONFIGSTORE_V1_1_SURFACEFLINGERCONFIGS_H
