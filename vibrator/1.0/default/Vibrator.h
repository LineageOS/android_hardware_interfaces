/*
 * Copyright (C) 2016 The Android Open Source Project
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
#ifndef HIDL_GENERATED_android_hardware_vibrator_V1_0_Vibrator_H_
#define HIDL_GENERATED_android_hardware_vibrator_V1_0_Vibrator_H_

#include <android/hardware/vibrator/1.0/IVibrator.h>
#include <hidl/Status.h>

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace vibrator {
namespace V1_0 {
namespace implementation {

using ::android::hardware::vibrator::V1_0::IVibrator;
using ::android::hardware::vibrator::V1_0::Status;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

struct Vibrator : public IVibrator {
  Vibrator(vibrator_device_t *device);

  // Methods from ::android::hardware::vibrator::V1_0::IVibrator follow.
  Return<Status> on(uint32_t timeoutMs)  override;
  Return<Status> off()  override;

  private:
    vibrator_device_t    *mDevice;
};

extern "C" IVibrator* HIDL_FETCH_IVibrator(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace vibrator
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_android_hardware_vibrator_V1_0_Vibrator_H_
