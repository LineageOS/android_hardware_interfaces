/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_CAMERA_PROVIDER_V2_5_EXTCAMERAPROVIDER_H
#define ANDROID_HARDWARE_CAMERA_PROVIDER_V2_5_EXTCAMERAPROVIDER_H

#include <ExternalCameraProviderImpl_2_4.h>

#include <android/hardware/camera/provider/2.5/ICameraProvider.h>
#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>

namespace android {
namespace hardware {
namespace camera {
namespace provider {
namespace V2_5 {
namespace implementation {

using namespace ::android::hardware::camera::provider;

using ::android::hardware::camera::common::V1_0::CameraDeviceStatus;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::VendorTagSection;
using ::android::hardware::camera::external::common::ExternalCameraConfig;
using ::android::hardware::camera::provider::V2_5::ICameraProvider;
using ::android::hardware::camera::provider::V2_4::ICameraProviderCallback;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_bitfield;

struct ExternalCameraProviderImpl_2_5 :
        public V2_4::implementation::ExternalCameraProviderImpl_2_4 {
    ExternalCameraProviderImpl_2_5();
    ~ExternalCameraProviderImpl_2_5();

    // Methods from ::android::hardware::camera::provider::V2_5::ICameraProvider follow.
    Return<void> notifyDeviceStateChange(hidl_bitfield<DeviceState> newState);
private:
};

}  // namespace implementation
}  // namespace V2_5
}  // namespace provider
}  // namespace camera
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CAMERA_PROVIDER_V2_5_EXTCAMERAPROVIDER_H
