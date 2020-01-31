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

#define LOG_TAG "ExtCamDev@3.6"
//#define LOG_NDEBUG 0
#include <log/log.h>

#include "ExternalCameraDevice_3_6.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_6 {
namespace implementation {

ExternalCameraDevice::ExternalCameraDevice(
        const std::string& cameraId, const ExternalCameraConfig& cfg) :
        V3_5::implementation::ExternalCameraDevice(cameraId, cfg) {}

ExternalCameraDevice::~ExternalCameraDevice() {}

sp<V3_4::implementation::ExternalCameraDeviceSession> ExternalCameraDevice::createSession(
        const sp<V3_2::ICameraDeviceCallback>& cb,
        const ExternalCameraConfig& cfg,
        const std::vector<SupportedV4L2Format>& sortedFormats,
        const CroppingType& croppingType,
        const common::V1_0::helper::CameraMetadata& chars,
        const std::string& cameraId,
        unique_fd v4l2Fd) {
    return new ExternalCameraDeviceSession(
            cb, cfg, sortedFormats, croppingType, chars, cameraId, std::move(v4l2Fd));
}

#define UPDATE(tag, data, size)                    \
do {                                               \
  if (metadata->update((tag), (data), (size))) {   \
    ALOGE("Update " #tag " failed!");              \
    return -EINVAL;                                \
  }                                                \
} while (0)

status_t ExternalCameraDevice::initAvailableCapabilities(
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata* metadata) {
    status_t res =
            V3_4::implementation::ExternalCameraDevice::initAvailableCapabilities(metadata);

    if (res != OK) {
        return res;
    }

    camera_metadata_entry caps = metadata->find(ANDROID_REQUEST_AVAILABLE_CAPABILITIES);
    std::vector<uint8_t> availableCapabilities;

    for (size_t i = 0; i < caps.count; i++) {
        uint8_t capability = caps.data.u8[i];
        availableCapabilities.push_back(capability);
    }

    // Add OFFLINE_PROCESSING capability to device 3.6
    availableCapabilities.push_back(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_OFFLINE_PROCESSING);

    UPDATE(ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
           availableCapabilities.data(),
           availableCapabilities.size());

    return OK;
}

#undef UPDATE

}  // namespace implementation
}  // namespace V3_6
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

