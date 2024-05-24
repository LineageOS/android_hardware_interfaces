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

#include "FakeLockoutTracker.h"
#include <fingerprint.sysprop.h>
#include "Fingerprint.h"
#include "util/Util.h"

using namespace ::android::fingerprint::virt;

namespace aidl::android::hardware::biometrics::fingerprint {

void FakeLockoutTracker::reset() {
    mFailedCount = 0;
    mLockoutTimedStart = 0;
    mCurrentMode = LockoutMode::kNone;
}

void FakeLockoutTracker::addFailedAttempt() {
    bool enabled = Fingerprint::cfg().get<bool>("lockout_enable");
    if (enabled) {
        mFailedCount++;
        int32_t lockoutTimedThreshold =
                Fingerprint::cfg().get<std::int32_t>("lockout_timed_threshold");
        int32_t lockoutPermanetThreshold =
                Fingerprint::cfg().get<std::int32_t>("lockout_permanent_threshold");
        if (mFailedCount >= lockoutPermanetThreshold) {
            mCurrentMode = LockoutMode::kPermanent;
            Fingerprint::cfg().set<bool>("lockout", true);
        } else if (mFailedCount >= lockoutTimedThreshold) {
            if (mCurrentMode == LockoutMode::kNone) {
                mCurrentMode = LockoutMode::kTimed;
                mLockoutTimedStart = Util::getSystemNanoTime();
            }
        }
    } else {
        reset();
    }
}

FakeLockoutTracker::LockoutMode FakeLockoutTracker::getMode() {
    if (mCurrentMode == LockoutMode::kTimed) {
        int32_t lockoutTimedDuration =
                Fingerprint::cfg().get<std::int32_t>("lockout_timed_duration");
        if (Util::hasElapsed(mLockoutTimedStart, lockoutTimedDuration)) {
            mCurrentMode = LockoutMode::kNone;
            mLockoutTimedStart = 0;
        }
    }

    return mCurrentMode;
}

int64_t FakeLockoutTracker::getLockoutTimeLeft() {
    int64_t res = 0;

    if (mLockoutTimedStart > 0) {
        int32_t lockoutTimedDuration =
                Fingerprint::cfg().get<std::int32_t>("lockout_timed_duration");
        auto now = Util::getSystemNanoTime();
        auto elapsed = (now - mLockoutTimedStart) / 1000000LL;
        res = lockoutTimedDuration - elapsed;
        LOG(INFO) << "elapsed=" << elapsed << " now = " << now
                  << " mLockoutTimedStart=" << mLockoutTimedStart << " res=" << res;
    }

    return res;
}
}  // namespace aidl::android::hardware::biometrics::fingerprint
