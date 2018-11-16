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

#ifndef ANDROID_HARDWARE_CAMERA_PROVIDER_V2_4_EXTCAMERAPROVIDER_H
#define ANDROID_HARDWARE_CAMERA_PROVIDER_V2_4_EXTCAMERAPROVIDER_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include "utils/Mutex.h"
#include "utils/Thread.h"
#include <android/hardware/camera/provider/2.4/ICameraProvider.h>
#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>
#include "ExternalCameraUtils.h"

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_4 {
namespace implementation {

using ::android::hardware::camera::common::V1_0::CameraDeviceStatus;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::VendorTagSection;
using ::android::hardware::camera::external::common::ExternalCameraConfig;
using ::android::hardware::camera::provider::V2_4::ICameraProvider;
using ::android::hardware::camera::provider::V2_4::ICameraProviderCallback;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;
using ::android::Mutex;

struct ExternalCameraProvider : public ICameraProvider {
    ExternalCameraProvider();
    ~ExternalCameraProvider();

    // Methods from ::android::hardware::camera::provider::V2_4::ICameraProvider follow.
    Return<Status> setCallback(const sp<ICameraProviderCallback>& callback) override;

    Return<void> getVendorTags(getVendorTags_cb _hidl_cb) override;

    Return<void> getCameraIdList(getCameraIdList_cb _hidl_cb) override;

    Return<void> isSetTorchModeSupported(isSetTorchModeSupported_cb _hidl_cb) override;

    Return<void> getCameraDeviceInterface_V1_x(
            const hidl_string&,
            getCameraDeviceInterface_V1_x_cb) override;
    Return<void> getCameraDeviceInterface_V3_x(
            const hidl_string&,
            getCameraDeviceInterface_V3_x_cb) override;

private:

    void addExternalCamera(const char* devName);

    void deviceAdded(const char* devName);

    void deviceRemoved(const char* devName);

    class HotplugThread : public android::Thread {
    public:
        HotplugThread(ExternalCameraProvider* parent);
        ~HotplugThread();

        virtual bool threadLoop() override;

    private:
        ExternalCameraProvider* mParent = nullptr;
        const std::unordered_set<std::string> mInternalDevices;

        int mINotifyFD = -1;
        int mWd = -1;
    };

    Mutex mLock;
    sp<ICameraProviderCallback> mCallbacks = nullptr;
    std::unordered_map<std::string, CameraDeviceStatus> mCameraStatusMap; // camera id -> status
    const ExternalCameraConfig mCfg;
    HotplugThread mHotPlugThread;
    int mPreferredHal3MinorVersion;
};



}  // namespace implementation
}  // namespace V2_4
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_PROVIDER_V2_4_EXTCAMERAPROVIDER_H
