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

#define LOG_TAG "CamDevSession@3.5-impl"
#include <android/log.h>

#include <utils/Trace.h>
#include "CameraDeviceSession.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_5 {
namespace implementation {

CameraDeviceSession::CameraDeviceSession(
    camera3_device_t* device,
    const camera_metadata_t* deviceInfo,
    const sp<V3_2::ICameraDeviceCallback>& callback) :
        V3_4::implementation::CameraDeviceSession(device, deviceInfo, callback) {

    mHasCallback_3_5 = false;

    auto castResult = ICameraDeviceCallback::castFrom(callback);
    if (castResult.isOk()) {
        sp<ICameraDeviceCallback> callback3_5 = castResult;
        if (callback3_5 != nullptr) {
            mHasCallback_3_5 = true;
        }
    }
}

CameraDeviceSession::~CameraDeviceSession() {
}

Return<void> CameraDeviceSession::configureStreams_3_5(
        const StreamConfiguration& requestedConfiguration,
        ICameraDeviceSession::configureStreams_3_5_cb _hidl_cb)  {
    return configureStreams_3_4(requestedConfiguration.v3_4, _hidl_cb);
}

Return<void> CameraDeviceSession::signalStreamFlush(
        const hidl_vec<int32_t>& /*requests*/, uint32_t /*streamConfigCounter*/) {
    return Void();
}

} // namespace implementation
}  // namespace V3_5
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
