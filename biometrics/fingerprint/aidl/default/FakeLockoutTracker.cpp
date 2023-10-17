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
#include "util/Util.h"

using namespace ::android::fingerprint::virt;

namespace aidl::android::hardware::biometrics::fingerprint {

void FakeLockoutTracker::reset() {
    mFailedCount = 0;
    mLockoutTimedStart = 0;
    mCurrentMode = LockoutMode::kNone;
}

void FakeLockoutTracker::addFailedAttempt() {
    bool enabled = FingerprintHalProperties::lockout_enable().value_or(false);
    if (enabled) {
        mFailedCount++;
        int32_t lockoutTimedThreshold =
                FingerprintHalProperties::lockout_timed_threshold().value_or(5);
        int32_t lockoutPermanetThreshold =
                FingerprintHalProperties::lockout_permanent_threshold().value_or(20);
        if (mFailedCount >= lockoutPermanetThreshold) {
            mCurrentMode = LockoutMode::kPermanent;
            FingerprintHalProperties::lockout(true);
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
                FingerprintHalProperties::lockout_timed_duration().value_or(10 * 100);
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
                FingerprintHalProperties::lockout_timed_duration().value_or(10 * 100);
        auto now = Util::getSystemNanoTime();
        auto elapsed = (now - mLockoutTimedStart) / 1000000LL;
        res = lockoutTimedDuration - elapsed;
        LOG(INFO) << "xxxxxx: elapsed=" << elapsed << " now = " << now
                  << " mLockoutTimedStart=" << mLockoutTimedStart << " res=" << res;
    }

    return res;
}
}  // namespace aidl::android::hardware::biometrics::fingerprint
