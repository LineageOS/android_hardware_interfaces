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

#include "ConfirmationUI.h"

#include "PlatformSpecifics.h"

#include <android/hardware/confirmationui/support/cbor.h>
#include <android/hardware/confirmationui/support/confirmationui_utils.h>

#include <android/hardware/confirmationui/1.0/generic/GenericOperation.h>

#include <time.h>

namespace android {
namespace hardware {
namespace confirmationui {
namespace V1_0 {
namespace implementation {

using ::android::hardware::confirmationui::V1_0::generic::Operation;
using ::android::hardware::keymaster::V4_0::HardwareAuthToken;

uint8_t hmacKey[32];

// Methods from ::android::hardware::confirmationui::V1_0::IConfirmationUI follow.
Return<ResponseCode> ConfirmationUI::promptUserConfirmation(
    const sp<IConfirmationResultCallback>& resultCB, const hidl_string& promptText,
    const hidl_vec<uint8_t>& extraData, const hidl_string& locale,
    const hidl_vec<UIOption>& uiOptions) {
    auto& operation = MyOperation::get();
    auto result = operation.init(resultCB, promptText, extraData, locale, uiOptions);
    if (result == ResponseCode::OK) {
        // This is where implementation start the UI and then call setPending on success.
        operation.setPending();
    }
    return result;
}

Return<ResponseCode> ConfirmationUI::deliverSecureInputEvent(
    const HardwareAuthToken& secureInputToken) {
    auto& operation = MyOperation::get();
    return operation.deliverSecureInputEvent(secureInputToken);
}

Return<void> ConfirmationUI::abort() {
    auto& operation = MyOperation::get();
    operation.abort();
    operation.finalize(hmacKey);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace confirmationui
}  // namespace hardware
}  // namespace android
