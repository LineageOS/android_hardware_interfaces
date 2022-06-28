/*
 * Copyright (C) 2021 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_CAMERA_PROVIDER_V2_7_EXTCAMERAPROVIDER_H
#define ANDROID_HARDWARE_CAMERA_PROVIDER_V2_7_EXTCAMERAPROVIDER_H

#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "ExternalCameraUtils.h"

#include <android/hardware/camera/provider/2.6/ICameraProviderCallback.h>
#include <android/hardware/camera/provider/2.7/ICameraProvider.h>

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_7 {
namespace implementation {

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::camera::common::V1_0::CameraDeviceStatus;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::VendorTagSection;
using ::android::hardware::camera::external::common::ExternalCameraConfig;
using ::android::hardware::camera::provider::V2_4::ICameraProviderCallback;
using ::android::hardware::camera::provider::V2_5::DeviceState;
using ::android::hardware::camera::provider::V2_7::CameraIdAndStreamCombination;
using ::android::hardware::camera::provider::V2_7::ICameraProvider;
using ::android::hidl::base::V1_0::IBase;

/**
 * The implementation of external webcam CameraProvider 2.7, separated
 * from the HIDL interface layer to allow for implementation reuse by later
 * provider versions.
 *
 * This camera provider supports standard UVC webcameras via the Linux V4L2
 * UVC driver.
 */
struct ExternalCameraProviderImpl_2_7 {
    ExternalCameraProviderImpl_2_7();
    ~ExternalCameraProviderImpl_2_7();

    // Caller must use this method to check if CameraProvider ctor failed
    bool isInitFailed() { return false; }

    // Methods from ::android::hardware::camera::provider::V2_4::ICameraProvider follow.
    Return<Status> setCallback(const sp<ICameraProviderCallback>& callback);
    Return<void> getVendorTags(ICameraProvider::getVendorTags_cb _hidl_cb);
    Return<void> getCameraIdList(ICameraProvider::getCameraIdList_cb _hidl_cb);
    Return<void> isSetTorchModeSupported(ICameraProvider::isSetTorchModeSupported_cb _hidl_cb);
    Return<void> getCameraDeviceInterface_V1_x(const hidl_string&,
                                               ICameraProvider::getCameraDeviceInterface_V1_x_cb);
    Return<void> getCameraDeviceInterface_V3_x(const hidl_string&,
                                               ICameraProvider::getCameraDeviceInterface_V3_x_cb);

    // Methods from ::android::hardware::camera::provider::V2_5::ICameraProvider follow.
    Return<void> notifyDeviceStateChange(hidl_bitfield<DeviceState> newState);

    // Methods from ::android::hardware::camera::provider::V2_7::ICameraProvider follow.
    Return<void> getConcurrentStreamingCameraIds(
            ICameraProvider::getConcurrentStreamingCameraIds_cb _hidl_cb);

    Return<void> isConcurrentStreamCombinationSupported(
            const hidl_vec<
                    ::android::hardware::camera::provider::V2_6::CameraIdAndStreamCombination>&
                    configs,
            ICameraProvider::isConcurrentStreamCombinationSupported_cb _hidl_cb);

    Return<void> isConcurrentStreamCombinationSupported_2_7(
            const hidl_vec<CameraIdAndStreamCombination>& configs,
            ICameraProvider::isConcurrentStreamCombinationSupported_2_7_cb _hidl_cb);

  private:
    void addExternalCamera(const char* devName);

    void deviceAdded(const char* devName);

    void deviceRemoved(const char* devName);

    void updateAttachedCameras();

    class HotplugThread : public android::Thread {
      public:
        HotplugThread(ExternalCameraProviderImpl_2_7* parent);
        ~HotplugThread();

        virtual bool threadLoop() override;

      private:
        ExternalCameraProviderImpl_2_7* mParent = nullptr;
        const std::unordered_set<std::string> mInternalDevices;

        int mINotifyFD = -1;
        int mWd = -1;
    };

    Mutex mLock;
    sp<ICameraProviderCallback> mCallbacks = nullptr;
    std::unordered_map<std::string, CameraDeviceStatus> mCameraStatusMap;  // camera id -> status
    const ExternalCameraConfig mCfg;
    sp<HotplugThread> mHotPlugThread;
    int mPreferredHal3MinorVersion;
};

}  // namespace implementation
}  // namespace V2_7
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_PROVIDER_V2_7_EXTCAMERAPROVIDER_H
