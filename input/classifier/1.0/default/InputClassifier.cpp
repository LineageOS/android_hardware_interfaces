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

#define LOG_TAG "InputClassifierHAL"

#include "InputClassifier.h"
#include <inttypes.h>
#include <log/log.h>
#include <utils/Timers.h>

using namespace android::hardware::input::common::V1_0;

namespace android {
namespace hardware {
namespace input {
namespace classifier {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::input::classifier::V1_0::IInputClassifier follow.
Return<Classification> InputClassifier::classify(const MotionEvent& /*event*/) {
    /**
     * The touchscreen data is highly device-dependent.
     * As a result, the implementation of this method will likely be hardware-specific.
     * Here we just report gesture as not having any classification, which means that the
     * default action will be taken in the framework.
     * This is equivalent to not having the InputClassifier HAL at all.
     */
    return Classification::NONE;
}

Return<void> InputClassifier::reset() {
    // We don't have any internal state, so no work needed here.
    return Void();
}

Return<void> InputClassifier::resetDevice(int32_t /*deviceId*/) {
    // We don't have any internal per-device state, so no work needed here.
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace classifier
}  // namespace input
}  // namespace hardware
}  // namespace android
