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

#ifndef HARDWARE_INTERFACES_CAMERA_PROVIDER_AIDL_VTS_TORCH_PROVIDER_CB_H_
#define HARDWARE_INTERFACES_CAMERA_PROVIDER_AIDL_VTS_TORCH_PROVIDER_CB_H_

#import <aidl/android/hardware/camera/provider/BnCameraProviderCallback.h>
#import <camera_aidl_test.h>

using ::aidl::android::hardware::camera::common::TorchModeStatus;
using ::aidl::android::hardware::camera::provider::BnCameraProviderCallback;

class TorchProviderCb : public BnCameraProviderCallback {
  public:
    TorchProviderCb(CameraAidlTest* parent);
    ndk::ScopedAStatus torchModeStatusChange(const std::string& cameraDeviceName,
                                             TorchModeStatus newStatus) override;

    ScopedAStatus cameraDeviceStatusChange(
            const std::string& in_cameraDeviceName,
            ::aidl::android::hardware::camera::common::CameraDeviceStatus in_newStatus) override;

    ScopedAStatus physicalCameraDeviceStatusChange(
            const std::string& in_cameraDeviceName, const std::string& in_physicalCameraDeviceName,
            ::aidl::android::hardware::camera::common::CameraDeviceStatus in_newStatus) override;

  private:
    CameraAidlTest* mParent;
};

#endif  // HARDWARE_INTERFACES_CAMERA_PROVIDER_AIDL_VTS_TORCH_PROVIDER_CB_H_
