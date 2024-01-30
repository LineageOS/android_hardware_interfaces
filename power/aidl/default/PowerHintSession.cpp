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

#include "PowerHintSession.h"

#include <android-base/logging.h>

namespace aidl::android::hardware::power::impl::example {

using ndk::ScopedAStatus;

PowerHintSession::PowerHintSession() {}

ScopedAStatus PowerHintSession::updateTargetWorkDuration(int64_t targetDurationNanos) {
    LOG(VERBOSE) << __func__ << "target duration in nanoseconds: " << targetDurationNanos;
    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::reportActualWorkDuration(
        const std::vector<WorkDuration>& /* durations */) {
    LOG(VERBOSE) << __func__;
    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::pause() {
    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::resume() {
    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::close() {
    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::sendHint(SessionHint /* hint */) {
    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::setThreads(const std::vector<int32_t>& threadIds) {
    if (threadIds.size() == 0) {
        LOG(ERROR) << "Error: threadIds.size() shouldn't be " << threadIds.size();
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus PowerHintSession::setMode(SessionMode /* mode */, bool /* enabled */) {
    return ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::power::impl::example
