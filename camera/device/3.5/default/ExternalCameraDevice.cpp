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

#define LOG_TAG "ExtCamDev@3.5"
//#define LOG_NDEBUG 0
#include <log/log.h>

#include "ExternalCameraDevice_3_5.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_5 {
namespace implementation {

ExternalCameraDevice::ExternalCameraDevice(
        const std::string& cameraId, const ExternalCameraConfig& cfg) :
        V3_4::implementation::ExternalCameraDevice(cameraId, cfg) {}

ExternalCameraDevice::~ExternalCameraDevice() {}

Return<void> ExternalCameraDevice::getPhysicalCameraCharacteristics(const hidl_string&,
        V3_5::ICameraDevice::getPhysicalCameraCharacteristics_cb _hidl_cb) {
    CameraMetadata cameraCharacteristics;
    // External camera HAL doesn't support physical camera functions
    _hidl_cb(Status::ILLEGAL_ARGUMENT, cameraCharacteristics);
    return Void();
}

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

status_t ExternalCameraDevice::initDefaultCharsKeys(
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata* metadata) {
    status_t res =
            V3_4::implementation::ExternalCameraDevice::initDefaultCharsKeys(metadata);

    if (res != OK) {
        return res;
    }

    const uint8_t bufMgrVer = ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION_HIDL_DEVICE_3_5;
    UPDATE(ANDROID_INFO_SUPPORTED_BUFFER_MANAGEMENT_VERSION, &bufMgrVer, 1);

    std::vector<int> availableCharacteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_3_4;
    availableCharacteristicsKeys.reserve(availableCharacteristicsKeys.size() +
            EXTRA_CHARACTERISTICS_KEYS_3_5.size());
    for (const auto& key : EXTRA_CHARACTERISTICS_KEYS_3_5) {
        availableCharacteristicsKeys.push_back(key);
    }
    UPDATE(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS,
           availableCharacteristicsKeys.data(),
           availableCharacteristicsKeys.size());

    return OK;
}

#undef UPDATE

}  // namespace implementation
}  // namespace V3_5
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

