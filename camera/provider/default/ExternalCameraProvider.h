/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef HARDWARE_INTERFACES_CAMERA_PROVIDER_DEFAULT_EXTERNALCAMERAPROVIDER_H_
#define HARDWARE_INTERFACES_CAMERA_PROVIDER_DEFAULT_EXTERNALCAMERAPROVIDER_H_

#include <ExternalCameraUtils.h>
#include <SimpleThread.h>
#include <aidl/android/hardware/camera/common/CameraDeviceStatus.h>
#include <aidl/android/hardware/camera/common/VendorTagSection.h>
#include <aidl/android/hardware/camera/device/ICameraDevice.h>
#include <aidl/android/hardware/camera/provider/BnCameraProvider.h>
#include <aidl/android/hardware/camera/provider/CameraIdAndStreamCombination.h>
#include <aidl/android/hardware/camera/provider/ConcurrentCameraIdCombination.h>
#include <aidl/android/hardware/camera/provider/ICameraProviderCallback.h>
#include <poll.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace implementation {

using ::aidl::android::hardware::camera::common::CameraDeviceStatus;
using ::aidl::android::hardware::camera::common::VendorTagSection;
using ::aidl::android::hardware::camera::device::ICameraDevice;
using ::aidl::android::hardware::camera::provider::BnCameraProvider;
using ::aidl::android::hardware::camera::provider::CameraIdAndStreamCombination;
using ::aidl::android::hardware::camera::provider::ConcurrentCameraIdCombination;
using ::aidl::android::hardware::camera::provider::ICameraProviderCallback;
using ::android::hardware::camera::common::helper::SimpleThread;
using ::android::hardware::camera::external::common::ExternalCameraConfig;

class ExternalCameraProvider : public BnCameraProvider {
  public:
    ExternalCameraProvider();
    ~ExternalCameraProvider() override;
    ndk::ScopedAStatus setCallback(
            const std::shared_ptr<ICameraProviderCallback>& in_callback) override;
    ndk::ScopedAStatus getVendorTags(std::vector<VendorTagSection>* _aidl_return) override;
    ndk::ScopedAStatus getCameraIdList(std::vector<std::string>* _aidl_return) override;
    ndk::ScopedAStatus getCameraDeviceInterface(
            const std::string& in_cameraDeviceName,
            std::shared_ptr<ICameraDevice>* _aidl_return) override;
    ndk::ScopedAStatus notifyDeviceStateChange(int64_t in_deviceState) override;
    ndk::ScopedAStatus getConcurrentCameraIds(
            std::vector<ConcurrentCameraIdCombination>* _aidl_return) override;
    ndk::ScopedAStatus isConcurrentStreamCombinationSupported(
            const std::vector<CameraIdAndStreamCombination>& in_configs,
            bool* _aidl_return) override;

  private:
    void addExternalCamera(const char* devName);
    void deviceAdded(const char* devName);
    void deviceRemoved(const char* devName);
    void updateAttachedCameras();

    // A separate thread to monitor '/dev' directory for '/dev/video*' entries
    // This thread calls back into ExternalCameraProvider when an actionable change is detected.
    class HotplugThread : public SimpleThread {
      public:
        explicit HotplugThread(ExternalCameraProvider* parent);
        ~HotplugThread() override;

      protected:
        bool threadLoop() override;

      private:
        // Returns true if thread initialization succeeded, and false if thread initialization
        // failed.
        bool initialize();

        ExternalCameraProvider* mParent = nullptr;
        const std::unordered_set<std::string> mInternalDevices;

        bool mIsInitialized = false;

        int mINotifyFD = -1;
        int mWd = -1;

        // struct to wrap mINotifyFD and poll it with timeout
        struct pollfd mPollFd = {};
        char mEventBuf[512] = {0};
    };

    Mutex mLock;
    std::shared_ptr<ICameraProviderCallback> mCallback = nullptr;
    std::unordered_map<std::string, CameraDeviceStatus> mCameraStatusMap;  // camera id -> status
    const ExternalCameraConfig mCfg;
    std::shared_ptr<HotplugThread> mHotPlugThread;
};

}  // namespace implementation
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // HARDWARE_INTERFACES_CAMERA_PROVIDER_DEFAULT_EXTERNALCAMERAPROVIDER_H_
