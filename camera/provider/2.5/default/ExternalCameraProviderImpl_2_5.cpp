/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "CamPrvdr@2.5-external"
//#define LOG_NDEBUG 0
#include <log/log.h>

#include "ExternalCameraProviderImpl_2_5.h"

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_5 {
namespace implementation {

ExternalCameraProviderImpl_2_5::ExternalCameraProviderImpl_2_5() :
        ExternalCameraProviderImpl_2_4() {
}

ExternalCameraProviderImpl_2_5::~ExternalCameraProviderImpl_2_5() {
}

Return<void> ExternalCameraProviderImpl_2_5::notifyDeviceStateChange(
        hidl_bitfield<DeviceState> /*newState*/) {
    return Void();
}

}  // namespace implementation
}  // namespace V2_5
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android
