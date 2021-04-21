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

#ifndef ANDROID_HARDWARE_ATRACE_V1_0_ATRACEDEVICE_H
#define ANDROID_HARDWARE_ATRACE_V1_0_ATRACEDEVICE_H

#include <android/hardware/atrace/1.0/IAtraceDevice.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace atrace {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct AtraceDevice : public IAtraceDevice {
    AtraceDevice();
    // Methods from ::android::hardware::atrace::V1_0::IAtraceDevice follow.
    Return<void> listCategories(listCategories_cb _hidl_cb) override;
    Return<::android::hardware::atrace::V1_0::Status> enableCategories(
        const hidl_vec<hidl_string>& categories) override;
    Return<::android::hardware::atrace::V1_0::Status> disableAllCategories() override;

  private:
    std::string tracefs_event_root_;

    // Methods from ::android::hidl::base::V1_0::IBase follow.
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace atrace
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_ATRACE_V1_0_ATRACEDEVICE_H
