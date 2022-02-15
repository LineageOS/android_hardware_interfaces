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

#include "inputprocessor-impl/InputProcessor.h"

namespace aidl {
namespace android {
namespace hardware {
namespace input {
namespace processor {

using ::aidl::android::hardware::input::common::Classification;
using aidl::android::hardware::input::common::MotionEvent;

::ndk::ScopedAStatus InputProcessor::classify(const MotionEvent& /*in_event*/,
                                              Classification* _aidl_return) {
    *_aidl_return = Classification::NONE;
    return ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus InputProcessor::reset() {
    return ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus InputProcessor::resetDevice(int32_t /*in_deviceId*/) {
    return ndk::ScopedAStatus::ok();
}

}  // namespace processor
}  // namespace input
}  // namespace hardware
}  // namespace android
}  // namespace aidl