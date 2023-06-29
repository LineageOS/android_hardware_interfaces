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

#include <android/binder_process.h>
#include <fingerprint.sysprop.h>
#include <gtest/gtest.h>

#include <android-base/logging.h>

#include "FakeLockoutTracker.h"
#include "util/Util.h"

using namespace ::android::fingerprint::virt;
using namespace ::aidl::android::hardware::biometrics::fingerprint;

namespace aidl::android::hardware::biometrics::fingerprint {

class FakeLockoutTrackerTest : public ::testing::Test {
  protected:
    static constexpr int32_t LOCKOUT_TIMED_THRESHOLD = 3;
    static constexpr int32_t LOCKOUT_PERMANENT_THRESHOLD = 5;
    static constexpr int32_t LOCKOUT_TIMED_DURATION = 100;

    void SetUp() override {
        FingerprintHalProperties::lockout_timed_threshold(LOCKOUT_TIMED_THRESHOLD);
        FingerprintHalProperties::lockout_timed_duration(LOCKOUT_TIMED_DURATION);
        FingerprintHalProperties::lockout_permanent_threshold(LOCKOUT_PERMANENT_THRESHOLD);
    }

    void TearDown() override {
        // reset to default
        FingerprintHalProperties::lockout_timed_threshold(5);
        FingerprintHalProperties::lockout_timed_duration(20);
        FingerprintHalProperties::lockout_permanent_threshold(10000);
        FingerprintHalProperties::lockout_enable(false);
        FingerprintHalProperties::lockout(false);
    }

    FakeLockoutTracker mLockoutTracker;
};

TEST_F(FakeLockoutTrackerTest, addFailedAttemptDisable) {
    FingerprintHalProperties::lockout_enable(false);
    for (int i = 0; i < LOCKOUT_TIMED_THRESHOLD + 1; i++) mLockoutTracker.addFailedAttempt();
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kNone);
    mLockoutTracker.reset();
}

TEST_F(FakeLockoutTrackerTest, addFailedAttemptLockoutTimed) {
    FingerprintHalProperties::lockout_enable(true);
    for (int i = 0; i < LOCKOUT_TIMED_THRESHOLD; i++) mLockoutTracker.addFailedAttempt();
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kTimed);
    // time left
    int N = 5;
    int64_t prevTimeLeft = INT_MAX;
    for (int i = 0; i < N; i++) {
        SLEEP_MS(LOCKOUT_TIMED_DURATION / N + 1);
        int64_t currTimeLeft = mLockoutTracker.getLockoutTimeLeft();
        ASSERT_TRUE(currTimeLeft < prevTimeLeft);
        prevTimeLeft = currTimeLeft;
    }
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kNone);
    mLockoutTracker.reset();
}

TEST_F(FakeLockoutTrackerTest, addFailedAttemptPermanent) {
    FingerprintHalProperties::lockout_enable(true);
    for (int i = 0; i < LOCKOUT_PERMANENT_THRESHOLD - 1; i++) mLockoutTracker.addFailedAttempt();
    ASSERT_NE(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kPermanent);
    mLockoutTracker.addFailedAttempt();
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kPermanent);
    ASSERT_TRUE(FingerprintHalProperties::lockout());
    mLockoutTracker.reset();
}

}  // namespace aidl::android::hardware::biometrics::fingerprint

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
