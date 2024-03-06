/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "FaceVirtualHalLockoutTracker"

#include "FakeLockoutTracker.h"
#include <android-base/logging.h>
#include <face.sysprop.h>
#include "util/Util.h"

using namespace ::android::face::virt;

namespace aidl::android::hardware::biometrics::face {

void FakeLockoutTracker::reset(bool dueToTimerExpire) {
    if (!dueToTimerExpire) {
        mFailedCount = 0;
        mLastFailedTime = 0;
    }
    mTimedFailedCount = 0;
    mCurrentMode = LockoutMode::kNone;
    abortTimer();
}

void FakeLockoutTracker::addFailedAttempt(ISessionCallback* cb) {
    bool lockoutEnabled = FaceHalProperties::lockout_enable().value_or(false);
    bool timedLockoutenabled = FaceHalProperties::lockout_timed_enable().value_or(false);
    if (lockoutEnabled) {
        mFailedCount++;
        mTimedFailedCount++;
        mLastFailedTime = Util::getSystemNanoTime();
        int32_t lockoutTimedThreshold = FaceHalProperties::lockout_timed_threshold().value_or(3);
        int32_t lockoutPermanetThreshold =
                FaceHalProperties::lockout_permanent_threshold().value_or(5);
        if (mFailedCount >= lockoutPermanetThreshold) {
            mCurrentMode = LockoutMode::kPermanent;
            LOG(ERROR) << "FakeLockoutTracker: lockoutPermanent";
            cb->onLockoutPermanent();
            abortTimer();
        } else if (timedLockoutenabled && mTimedFailedCount >= lockoutTimedThreshold) {
            if (mCurrentMode == LockoutMode::kNone) {
                mCurrentMode = LockoutMode::kTimed;
                startLockoutTimer(getTimedLockoutDuration(), cb);
            }
            LOG(ERROR) << "FakeLockoutTracker: lockoutTimed";
            cb->onLockoutTimed(getLockoutTimeLeft());
        }
    } else {
        reset();
    }
}

FakeLockoutTracker::LockoutMode FakeLockoutTracker::getMode() {
    return mCurrentMode;
}

int32_t FakeLockoutTracker::getTimedLockoutDuration() {
    return FaceHalProperties::lockout_timed_duration().value_or(10 * 1000);
}

int64_t FakeLockoutTracker::getLockoutTimeLeft() {
    int64_t res = 0;

    if (mLastFailedTime > 0) {
        auto now = Util::getSystemNanoTime();
        auto elapsed = (now - mLastFailedTime) / 1000000LL;
        res = getTimedLockoutDuration() - elapsed;
        LOG(INFO) << "elapsed=" << elapsed << " now = " << now
                  << " mLastFailedTime=" << mLastFailedTime << " res=" << res;
    }

    return res;
}

bool FakeLockoutTracker::checkIfLockout(ISessionCallback* cb) {
    if (mCurrentMode == LockoutMode::kPermanent) {
        LOG(ERROR) << "Lockout permanent";
        cb->onLockoutPermanent();
        return true;
    } else if (mCurrentMode == LockoutMode::kTimed) {
        auto timeLeft = getLockoutTimeLeft();
        LOG(ERROR) << "Lockout timed " << timeLeft;
        cb->onLockoutTimed(timeLeft);
        return true;
    }
    return false;
}

void FakeLockoutTracker::startLockoutTimer(int64_t timeout, ISessionCallback* cb) {
    LOG(ERROR) << "startLockoutTimer: to=" << timeout;
    if (mIsLockoutTimerStarted) return;
    std::function<void(ISessionCallback*)> action =
            std::bind(&FakeLockoutTracker::lockoutTimerExpired, this, std::placeholders::_1);
    std::thread([timeout, action, cb]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        action(cb);
    }).detach();

    mIsLockoutTimerStarted = true;
}

void FakeLockoutTracker::lockoutTimerExpired(ISessionCallback* cb) {
    LOG(INFO) << "lockout timer expired";
    mIsLockoutTimerStarted = false;

    if (mIsLockoutTimerAborted) {
        mIsLockoutTimerAborted = false;
        return;
    }

    // if more failures seen since the timer started, need to restart timer again
    auto deltaTime = getLockoutTimeLeft();
    if (deltaTime <= 0) {
        cb->onLockoutCleared();
        reset(true);
    } else {
        startLockoutTimer(deltaTime, cb);
    }
}

void FakeLockoutTracker::abortTimer() {
    if (mIsLockoutTimerStarted) mIsLockoutTimerAborted = true;
}

}  // namespace aidl::android::hardware::biometrics::face
