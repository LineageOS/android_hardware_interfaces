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

#ifndef ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_EXTCAMERADEVICE_H
#define ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_EXTCAMERADEVICE_H

#include "utils/Mutex.h"
#include "CameraMetadata.h"

#include <android/hardware/camera/device/3.2/ICameraDevice.h>
#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>
#include "ExternalCameraDeviceSession.h"

namespace android {
namespace hardware {
namespace camera {
namespace device {
namespace V3_4 {
namespace implementation {

using namespace ::android::hardware::camera::device;
using ::android::hardware::camera::device::V3_2::ICameraDevice;
using ::android::hardware::camera::device::V3_2::ICameraDeviceCallback;
using ::android::hardware::camera::common::V1_0::CameraResourceCost;
using ::android::hardware::camera::common::V1_0::TorchMode;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::external::common::ExternalCameraConfig;
using ::android::hardware::camera::external::common::Size;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

/*
 * The camera device HAL implementation is opened lazily (via the open call)
 */
struct ExternalCameraDevice : public ICameraDevice {

    // Called by external camera provider HAL.
    // Provider HAL must ensure the uniqueness of CameraDevice object per cameraId, or there could
    // be multiple CameraDevice trying to access the same physical camera.  Also, provider will have
    // to keep track of all CameraDevice objects in order to notify CameraDevice when the underlying
    // camera is detached.
    ExternalCameraDevice(const std::string& cameraId, const ExternalCameraConfig& cfg);
    ~ExternalCameraDevice();

    // Caller must use this method to check if CameraDevice ctor failed
    bool isInitFailed();

    /* Methods from ::android::hardware::camera::device::V3_2::ICameraDevice follow. */
    // The following method can be called without opening the actual camera device
    Return<void> getResourceCost(getResourceCost_cb _hidl_cb) override;

    Return<void> getCameraCharacteristics(getCameraCharacteristics_cb _hidl_cb) override;

    Return<Status> setTorchMode(TorchMode) override;

    // Open the device HAL and also return a default capture session
    Return<void> open(const sp<ICameraDeviceCallback>&, open_cb) override;

    // Forward the dump call to the opened session, or do nothing
    Return<void> dumpState(const ::android::hardware::hidl_handle&) override;
    /* End of Methods from ::android::hardware::camera::device::V3_2::ICameraDevice */

protected:
    void getFrameRateList(int fd, float fpsUpperBound, SupportedV4L2Format* format);
    // Init supported w/h/format/fps in mSupportedFormats. Caller still owns fd
    void initSupportedFormatsLocked(int fd);

    status_t initCameraCharacteristics();
    // Init non-device dependent keys
    status_t initDefaultCharsKeys(::android::hardware::camera::common::V1_0::helper::CameraMetadata*);
    // Init camera control chars keys. Caller still owns fd
    status_t initCameraControlsCharsKeys(int fd,
            ::android::hardware::camera::common::V1_0::helper::CameraMetadata*);
    // Init camera output configuration related keys.  Caller still owns fd
    status_t initOutputCharsKeys(int fd,
            ::android::hardware::camera::common::V1_0::helper::CameraMetadata*);

    static CroppingType initCroppingType(/*inout*/std::vector<SupportedV4L2Format>*);

    Mutex mLock;
    bool mInitFailed = false;
    std::string mCameraId;
    const ExternalCameraConfig& mCfg;
    std::vector<SupportedV4L2Format> mSupportedFormats;
    CroppingType mCroppingType;

    wp<ExternalCameraDeviceSession> mSession = nullptr;

    ::android::hardware::camera::common::V1_0::helper::CameraMetadata mCameraCharacteristics;
};

}  // namespace implementation
}  // namespace V3_4
}  // namespace device
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_DEVICE_V3_4_EXTCAMERADEVICE_H
