/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "CamPrvdr@2.5-legacy"
//#define LOG_NDEBUG 0
#include <android/log.h>
#include <inttypes.h>

#include "LegacyCameraProviderImpl_2_5.h"
#include "CameraProvider_2_5.h"

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_5 {
namespace implementation {

template struct CameraProvider<LegacyCameraProviderImpl_2_5>;

LegacyCameraProviderImpl_2_5::LegacyCameraProviderImpl_2_5() :
        LegacyCameraProviderImpl_2_4() {
}

LegacyCameraProviderImpl_2_5::~LegacyCameraProviderImpl_2_5() {}

Return<void> LegacyCameraProviderImpl_2_5::notifyDeviceStateChange(
        hidl_bitfield<DeviceState> newState) {
    ALOGD("%s: New device state: 0x%" PRIx64, __FUNCTION__, newState);
    uint64_t state = static_cast<uint64_t>(newState);
    mModule->notifyDeviceStateChange(state);
    return Void();
}

}  // namespace implementation
}  // namespace V2_5
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android
