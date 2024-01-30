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

#pragma once

#include <aidl/android/hardware/power/BnPowerHintSession.h>
#include <aidl/android/hardware/power/SessionHint.h>
#include <aidl/android/hardware/power/SessionMode.h>
#include <aidl/android/hardware/power/WorkDuration.h>

namespace aidl::android::hardware::power::impl::example {

class PowerHintSession : public BnPowerHintSession {
  public:
    explicit PowerHintSession();
    ndk::ScopedAStatus updateTargetWorkDuration(int64_t targetDurationNanos) override;
    ndk::ScopedAStatus reportActualWorkDuration(
            const std::vector<WorkDuration>& durations) override;
    ndk::ScopedAStatus pause() override;
    ndk::ScopedAStatus resume() override;
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus sendHint(SessionHint hint) override;
    ndk::ScopedAStatus setThreads(const std::vector<int32_t>& threadIds) override;
    ndk::ScopedAStatus setMode(SessionMode mode, bool enabled) override;
};

}  // namespace aidl::android::hardware::power::impl::example
