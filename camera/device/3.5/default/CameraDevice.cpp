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

#define LOG_TAG "CamDev@3.5-impl"
#include <log/log.h>

#include "CameraModule.h"
#include "CameraDevice_3_5.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_5 {
namespace implementation {

using namespace ::android::hardware::camera::device;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::device::V3_2::CameraMetadata;

CameraDevice::CameraDevice(sp<CameraModule> module, const std::string& cameraId,
        const SortedVector<std::pair<std::string, std::string>>& cameraDeviceNames) :
        V3_4::implementation::CameraDevice(module, cameraId, cameraDeviceNames) {
}

CameraDevice::~CameraDevice() {
}

sp<V3_2::implementation::CameraDeviceSession> CameraDevice::createSession(camera3_device_t* device,
        const camera_metadata_t* deviceInfo,
        const sp<V3_2::ICameraDeviceCallback>& callback) {
    sp<CameraDeviceSession> session = new CameraDeviceSession(device, deviceInfo, callback);
    IF_ALOGV() {
        session->getInterface()->interfaceChain([](
            ::android::hardware::hidl_vec<::android::hardware::hidl_string> interfaceChain) {
                ALOGV("Session interface chain:");
                for (auto iface : interfaceChain) {
                    ALOGV("  %s", iface.c_str());
                }
            });
    }
    return session;
}

Return<void> CameraDevice::getPhysicalCameraCharacteristics(const hidl_string& physicalCameraId,
        V3_5::ICameraDevice::getPhysicalCameraCharacteristics_cb _hidl_cb) {
    Status status = initStatus();
    CameraMetadata cameraCharacteristics;
    if (status == Status::OK) {
        // Require module 2.5+ version.
        if (mModule->getModuleApiVersion() < CAMERA_MODULE_API_VERSION_2_5) {
            ALOGE("%s: get_physical_camera_info must be called on camera module 2.5 or newer",
                    __FUNCTION__);
            status = Status::INTERNAL_ERROR;
        } else {
            char *end;
            errno = 0;
            long id = strtol(physicalCameraId.c_str(), &end, 0);
            if (id > INT_MAX || (errno == ERANGE && id == LONG_MAX) ||
                    id < INT_MIN || (errno == ERANGE && id == LONG_MIN) ||
                    *end != '\0') {
                ALOGE("%s: Invalid physicalCameraId %s", __FUNCTION__, physicalCameraId.c_str());
                status = Status::ILLEGAL_ARGUMENT;
            } else {
                camera_metadata_t *physicalInfo = nullptr;
                int ret = mModule->getPhysicalCameraInfo((int)id, &physicalInfo);
                if (ret == OK) {
                    V3_2::implementation::convertToHidl(physicalInfo, &cameraCharacteristics);
                } else if (ret == -EINVAL) {
                    ALOGE("%s: %s is not a valid physical camera Id outside of getCameraIdList()",
                            __FUNCTION__, physicalCameraId.c_str());
                    status = Status::ILLEGAL_ARGUMENT;
                } else {
                    ALOGE("%s: Failed to get physical camera %s info: %s (%d)!", __FUNCTION__,
                            physicalCameraId.c_str(), strerror(-ret), ret);
                    status = Status::INTERNAL_ERROR;
                }
            }
        }
    }
    _hidl_cb(status, cameraCharacteristics);
    return Void();
}

Return<void> CameraDevice::isStreamCombinationSupported(const V3_4::StreamConfiguration& streams,
        V3_5::ICameraDevice::isStreamCombinationSupported_cb _hidl_cb) {
    Status status;
    bool streamsSupported = false;

    // Require module 2.5+ version.
    if (mModule->getModuleApiVersion() < CAMERA_MODULE_API_VERSION_2_5) {
        ALOGE("%s: is_stream_combination_supported must be called on camera module 2.5 or "\
                "newer", __FUNCTION__);
        status = Status::INTERNAL_ERROR;
    } else {
        camera_stream_combination_t streamComb{};
        streamComb.operation_mode = static_cast<uint32_t> (streams.operationMode);
        streamComb.num_streams = streams.streams.size();
        camera_stream_t *streamBuffer  = new camera_stream_t[streamComb.num_streams];

        size_t i = 0;
        for (const auto &it : streams.streams) {
            streamBuffer[i].stream_type = static_cast<int> (it.v3_2.streamType);
            streamBuffer[i].width = it.v3_2.width;
            streamBuffer[i].height = it.v3_2.height;
            streamBuffer[i].format = static_cast<int> (it.v3_2.format);
            streamBuffer[i].data_space = static_cast<android_dataspace_t> (it.v3_2.dataSpace);
            streamBuffer[i].usage = static_cast<uint32_t> (it.v3_2.usage);
            streamBuffer[i].physical_camera_id = it.physicalCameraId.c_str();
            streamBuffer[i++].rotation = static_cast<int> (it.v3_2.rotation);
        }
        streamComb.streams = streamBuffer;
        auto res = mModule->isStreamCombinationSupported(mCameraIdInt, &streamComb);
        switch (res) {
            case NO_ERROR:
                streamsSupported = true;
                status = Status::OK;
                break;
            case BAD_VALUE:
                status = Status::OK;
                break;
            case INVALID_OPERATION:
                status = Status::METHOD_NOT_SUPPORTED;
                break;
            default:
                ALOGE("%s: Unexpected error: %d", __FUNCTION__, res);
                status = Status::INTERNAL_ERROR;
        };
        delete [] streamBuffer;
    }

    _hidl_cb(status, streamsSupported);
    return Void();
}

// End of methods from ::android::hardware::camera::device::V3_2::ICameraDevice.

} // namespace implementation
}  // namespace V3_5
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

