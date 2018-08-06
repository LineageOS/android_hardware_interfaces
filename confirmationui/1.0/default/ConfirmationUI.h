/*
**
** Copyright 2017, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_HARDWARE_CONFIRMATIONUI_V1_0_CONFIRMATIONUI_H
#define ANDROID_HARDWARE_CONFIRMATIONUI_V1_0_CONFIRMATIONUI_H

#include <android/hardware/confirmationui/1.0/IConfirmationUI.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace confirmationui {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct ConfirmationUI : public IConfirmationUI {
    // Methods from ::android::hardware::confirmationui::V1_0::IConfirmationUI follow.
    Return<ResponseCode> promptUserConfirmation(const sp<IConfirmationResultCallback>& resultCB,
                                                const hidl_string& promptText,
                                                const hidl_vec<uint8_t>& extraData,
                                                const hidl_string& locale,
                                                const hidl_vec<UIOption>& uiOptions) override;
    Return<ResponseCode> deliverSecureInputEvent(
        const ::android::hardware::keymaster::V4_0::HardwareAuthToken& secureInputToken) override;
    Return<void> abort() override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace confirmationui
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_CONFIRMATIONUI_V1_0_CONFIRMATIONUI_H
