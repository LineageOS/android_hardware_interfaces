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

#include <aidl/android/hardware/biometrics/face/BnSessionCallback.h>
#include <android/binder_process.h>
#include <face.sysprop.h>
#include <gtest/gtest.h>

#include <android-base/logging.h>

#include "FakeLockoutTracker.h"
#include "util/Util.h"

using namespace ::android::face::virt;
using namespace ::aidl::android::hardware::biometrics::face;

namespace aidl::android::hardware::biometrics::face {

class TestSessionCallback : public BnSessionCallback {
  public:
    ndk::ScopedAStatus onChallengeGenerated(int64_t /*challenge*/) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onChallengeRevoked(int64_t /*challenge*/) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onError(face::Error, int32_t /*vendorCode*/) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onEnrollmentProgress(int32_t /*enrollmentId*/,
                                              int32_t /*remaining*/) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onAuthenticationSucceeded(int32_t /*enrollmentId*/,
                                                   const keymaster::HardwareAuthToken&) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onAuthenticationFailed() override { return ndk::ScopedAStatus::ok(); };
    ::ndk::ScopedAStatus onInteractionDetected() override { return ndk::ScopedAStatus::ok(); };
    ::ndk::ScopedAStatus onEnrollmentsEnumerated(const std::vector<int32_t>&) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onEnrollmentsRemoved(
            const std::vector<int32_t>& /*enrollmentIds*/) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onAuthenticatorIdRetrieved(int64_t /*authenticatorId*/) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onAuthenticatorIdInvalidated(int64_t /*authenticatorId*/) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onEnrollmentFrame(const EnrollmentFrame&) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onFeaturesRetrieved(const std::vector<Feature>&) {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onFeatureSet(Feature) override { return ndk::ScopedAStatus::ok(); }
    ::ndk::ScopedAStatus onSessionClosed() override { return ndk::ScopedAStatus::ok(); }
    ::ndk::ScopedAStatus onAuthenticationFrame(const AuthenticationFrame&) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onLockoutTimed(int64_t timeLeft) override {
        mLockoutTimed++;
        mTimeLeft = timeLeft;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onLockoutPermanent() override {
        mLockoutPermanent++;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onLockoutCleared() override {
        mTimeLeft = 0;
        mLockoutTimed = 0;
        mLockoutPermanent = 0;
        return ndk::ScopedAStatus::ok();
    };

    int64_t mTimeLeft = 0;
    int mLockoutTimed = 0;
    int mLockoutPermanent = 0;
};

class FakeLockoutTrackerTest : public ::testing::Test {
  protected:
    static constexpr int32_t LOCKOUT_TIMED_THRESHOLD = 3;
    static constexpr int32_t LOCKOUT_PERMANENT_THRESHOLD = 5;
    static constexpr int32_t LOCKOUT_TIMED_DURATION = 100;

    void SetUp() override {
        FaceHalProperties::lockout_timed_threshold(LOCKOUT_TIMED_THRESHOLD);
        FaceHalProperties::lockout_timed_duration(LOCKOUT_TIMED_DURATION);
        FaceHalProperties::lockout_permanent_threshold(LOCKOUT_PERMANENT_THRESHOLD);
        mCallback = ndk::SharedRefBase::make<TestSessionCallback>();
    }

    void TearDown() override {
        // reset to default
        FaceHalProperties::lockout_timed_threshold(5);
        FaceHalProperties::lockout_timed_duration(20);
        FaceHalProperties::lockout_permanent_threshold(10000);
        FaceHalProperties::lockout_enable(false);
        FaceHalProperties::lockout(false);
    }

    FakeLockoutTracker mLockoutTracker;
    std::shared_ptr<TestSessionCallback> mCallback;
};

TEST_F(FakeLockoutTrackerTest, addFailedAttemptDisable) {
    FaceHalProperties::lockout_enable(false);
    for (int i = 0; i < LOCKOUT_TIMED_THRESHOLD + 1; i++)
        mLockoutTracker.addFailedAttempt(mCallback.get());
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kNone);
    ASSERT_EQ(0, mCallback->mLockoutTimed);
}

TEST_F(FakeLockoutTrackerTest, addFailedAttemptPermanent) {
    FaceHalProperties::lockout_enable(true);
    ASSERT_FALSE(mLockoutTracker.checkIfLockout(mCallback.get()));
    for (int i = 0; i < LOCKOUT_PERMANENT_THRESHOLD - 1; i++)
        mLockoutTracker.addFailedAttempt(mCallback.get());
    ASSERT_NE(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kPermanent);
    ASSERT_EQ(0, mCallback->mLockoutPermanent);
    mLockoutTracker.addFailedAttempt(mCallback.get());
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kPermanent);
    ASSERT_EQ(1, mCallback->mLockoutPermanent);
    ASSERT_TRUE(mLockoutTracker.checkIfLockout(mCallback.get()));
    ASSERT_EQ(2, mCallback->mLockoutPermanent);
}

TEST_F(FakeLockoutTrackerTest, addFailedAttemptLockoutTimed) {
    FaceHalProperties::lockout_enable(true);
    FaceHalProperties::lockout_timed_enable(true);
    ASSERT_FALSE(mLockoutTracker.checkIfLockout(mCallback.get()));
    for (int i = 0; i < LOCKOUT_TIMED_THRESHOLD; i++)
        mLockoutTracker.addFailedAttempt(mCallback.get());
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kTimed);
    ASSERT_EQ(1, mCallback->mLockoutTimed);
    ASSERT_TRUE(mLockoutTracker.checkIfLockout(mCallback.get()));
    ASSERT_EQ(2, mCallback->mLockoutTimed);
    // time left
    int N = 5;
    int64_t prevTimeLeft = INT_MAX;
    for (int i = 0; i < N; i++) {
        SLEEP_MS(LOCKOUT_TIMED_DURATION / N + 1);
        int64_t currTimeLeft = mLockoutTracker.getLockoutTimeLeft();
        ASSERT_TRUE(currTimeLeft < prevTimeLeft);
        prevTimeLeft = currTimeLeft;
    }
    SLEEP_MS(LOCKOUT_TIMED_DURATION / N);
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kNone);
}

TEST_F(FakeLockoutTrackerTest, addFailedAttemptLockout_TimedThenPermanent) {
    FaceHalProperties::lockout_enable(true);
    FaceHalProperties::lockout_timed_enable(true);
    ASSERT_FALSE(mLockoutTracker.checkIfLockout(mCallback.get()));
    for (int i = 0; i < LOCKOUT_TIMED_THRESHOLD; i++)
        mLockoutTracker.addFailedAttempt(mCallback.get());
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kTimed);
    SLEEP_MS(LOCKOUT_TIMED_DURATION + 20);
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kNone);
    for (int i = 0; i < LOCKOUT_PERMANENT_THRESHOLD - LOCKOUT_TIMED_THRESHOLD; i++)
        mLockoutTracker.addFailedAttempt(mCallback.get());
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kPermanent);
}

TEST_F(FakeLockoutTrackerTest, addFailedAttemptLockoutTimedTwice) {
    FaceHalProperties::lockout_enable(true);
    FaceHalProperties::lockout_timed_enable(true);
    ASSERT_FALSE(mLockoutTracker.checkIfLockout(mCallback.get()));
    ASSERT_EQ(0, mCallback->mLockoutTimed);
    for (int i = 0; i < LOCKOUT_TIMED_THRESHOLD; i++)
        mLockoutTracker.addFailedAttempt(mCallback.get());
    SLEEP_MS(LOCKOUT_TIMED_DURATION / 2);
    mLockoutTracker.addFailedAttempt(mCallback.get());
    SLEEP_MS(LOCKOUT_TIMED_DURATION);
    ASSERT_EQ(2, mCallback->mLockoutTimed);
    ASSERT_TRUE(mLockoutTracker.checkIfLockout(mCallback.get()));
    SLEEP_MS(LOCKOUT_TIMED_DURATION);
    ASSERT_FALSE(mLockoutTracker.checkIfLockout(mCallback.get()));
}

TEST_F(FakeLockoutTrackerTest, resetLockout) {
    FaceHalProperties::lockout_enable(true);
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kNone);
    for (int i = 0; i < LOCKOUT_PERMANENT_THRESHOLD; i++)
        mLockoutTracker.addFailedAttempt(mCallback.get());
    ASSERT_EQ(mLockoutTracker.getMode(), FakeLockoutTracker::LockoutMode::kPermanent);
    mLockoutTracker.reset();
    ASSERT_FALSE(mLockoutTracker.checkIfLockout(mCallback.get()));
}

}  // namespace aidl::android::hardware::biometrics::face

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
