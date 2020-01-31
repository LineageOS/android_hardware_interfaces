/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "ExtCamDevSsn@3.6"
#include <android/log.h>

#include <utils/Trace.h>
#include "ExternalCameraDeviceSession.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_6 {
namespace implementation {

ExternalCameraDeviceSession::ExternalCameraDeviceSession(
        const sp<V3_2::ICameraDeviceCallback>& callback,
        const ExternalCameraConfig& cfg,
        const std::vector<SupportedV4L2Format>& sortedFormats,
        const CroppingType& croppingType,
        const common::V1_0::helper::CameraMetadata& chars,
        const std::string& cameraId,
        unique_fd v4l2Fd) :
        V3_5::implementation::ExternalCameraDeviceSession(
                callback, cfg, sortedFormats, croppingType, chars, cameraId, std::move(v4l2Fd)) {
}

ExternalCameraDeviceSession::~ExternalCameraDeviceSession() {}


Return<void> ExternalCameraDeviceSession::configureStreams_3_6(
        const StreamConfiguration& requestedConfiguration,
        ICameraDeviceSession::configureStreams_3_6_cb _hidl_cb)  {
    V3_2::StreamConfiguration config_v32;
    V3_3::HalStreamConfiguration outStreams_v33;
    V3_6::HalStreamConfiguration outStreams;
    const V3_4::StreamConfiguration& requestedConfiguration_3_4 = requestedConfiguration.v3_4;
    Mutex::Autolock _il(mInterfaceLock);

    config_v32.operationMode = requestedConfiguration_3_4.operationMode;
    config_v32.streams.resize(requestedConfiguration_3_4.streams.size());
    uint32_t blobBufferSize = 0;
    int numStallStream = 0;
    for (size_t i = 0; i < config_v32.streams.size(); i++) {
        config_v32.streams[i] = requestedConfiguration_3_4.streams[i].v3_2;
        if (config_v32.streams[i].format == PixelFormat::BLOB) {
            blobBufferSize = requestedConfiguration_3_4.streams[i].bufferSize;
            numStallStream++;
        }
    }

    // Fail early if there are multiple BLOB streams
    if (numStallStream > kMaxStallStream) {
        ALOGE("%s: too many stall streams (expect <= %d, got %d)", __FUNCTION__,
                kMaxStallStream, numStallStream);
        _hidl_cb(Status::ILLEGAL_ARGUMENT, outStreams);
        return Void();
    }

    Status status = configureStreams(config_v32, &outStreams_v33, blobBufferSize);

    outStreams.streams.resize(outStreams_v33.streams.size());
    for (size_t i = 0; i < outStreams.streams.size(); i++) {
        outStreams.streams[i].v3_4.v3_3 = outStreams_v33.streams[i];
        // TODO: implement it later
        outStreams.streams[i].supportOffline = false;
    }
    _hidl_cb(status, outStreams);
    return Void();
}

Return<void> ExternalCameraDeviceSession::switchToOffline(
        const hidl_vec<int32_t>& streamsToKeep,
        ICameraDeviceSession::switchToOffline_cb _hidl_cb) {
    // TODO: implement this
    (void) streamsToKeep;
    (void) _hidl_cb;
    return Void();
}

} // namespace implementation
}  // namespace V3_6
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android
