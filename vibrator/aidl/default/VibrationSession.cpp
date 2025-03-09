/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "vibrator-impl/VibrationSession.h"

#include <android-base/logging.h>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

static constexpr int32_t SESSION_END_DELAY_MS = 20;

ndk::ScopedAStatus VibrationSession::close() {
    LOG(VERBOSE) << "Vibration Session close";
    mManager->closeSession(SESSION_END_DELAY_MS);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VibrationSession::abort() {
    LOG(VERBOSE) << "Vibration Session abort";
    mManager->abortSession();
    return ndk::ScopedAStatus::ok();
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
