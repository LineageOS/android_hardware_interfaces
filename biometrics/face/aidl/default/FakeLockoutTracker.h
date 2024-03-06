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

#pragma once

#include <aidl/android/hardware/biometrics/face/ISessionCallback.h>
#include <android/binder_to_string.h>
#include <stdint.h>
#include <string>

namespace aidl::android::hardware::biometrics::face {

// Lockout implementation for Face Virtual HAL
class FakeLockoutTracker {
  public:
    FakeLockoutTracker()
        : mFailedCount(0),
          mLastFailedTime(0),
          mIsLockoutTimerStarted(false),
          mIsLockoutTimerAborted(false) {}
    ~FakeLockoutTracker() {}

    enum class LockoutMode : int8_t { kNone = 0, kTimed, kPermanent };

    bool checkIfLockout(ISessionCallback*);
    void addFailedAttempt(ISessionCallback*);
    int64_t getLockoutTimeLeft();
    LockoutMode getMode();
    void reset(bool dueToTimerExpire = false);
    inline std::string toString() const {
        std::ostringstream os;
        os << "----- FakeLockoutTracker:: -----" << std::endl;
        os << "mFailedCount:" << mFailedCount;
        os << ", mCurrentMode:" << (int)mCurrentMode;
        os << ", mLastFailedTime:" << (int)(mLastFailedTime / 1000000LL);
        os << ",  mIsLockoutTimerStarted:" << mIsLockoutTimerStarted;
        os << ", mIsLockoutTimerAborted:" << mIsLockoutTimerAborted;
        os << std::endl;
        return os.str();
    }

  private:
    void startLockoutTimer(int64_t timeout, ISessionCallback* cb);
    void lockoutTimerExpired(ISessionCallback* cb);
    int32_t getTimedLockoutDuration();
    void abortTimer();

  private:
    int32_t mFailedCount;
    int32_t mTimedFailedCount;
    int64_t mLastFailedTime;
    LockoutMode mCurrentMode;
    bool mIsLockoutTimerStarted;
    bool mIsLockoutTimerAborted;
};

}  // namespace aidl::android::hardware::biometrics::face
