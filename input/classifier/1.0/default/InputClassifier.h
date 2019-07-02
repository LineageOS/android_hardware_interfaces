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

#ifndef ANDROID_HARDWARE_INPUT_CLASSIFIER_V1_0_INPUTCLASSIFIER_H
#define ANDROID_HARDWARE_INPUT_CLASSIFIER_V1_0_INPUTCLASSIFIER_H

#include <android/hardware/input/classifier/1.0/IInputClassifier.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace input {
namespace classifier {
namespace V1_0 {
namespace implementation {

using ::android::hardware::Return;

struct InputClassifier : public IInputClassifier {
    // Methods from ::android::hardware::input::classifier::V1_0::IInputClassifier follow.

    Return<android::hardware::input::common::V1_0::Classification> classify(
            const android::hardware::input::common::V1_0::MotionEvent& event) override;

    Return<void> reset() override;
    Return<void> resetDevice(int32_t deviceId) override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace classifier
}  // namespace input
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_INPUT_CLASSIFIER_V1_0_INPUTCLASSIFIER_H
