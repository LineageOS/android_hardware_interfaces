/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <aidl/android/hardware/biometrics/fingerprint/BnSessionCallback.h>

#include "FakeFingerprintEngine.h"

using namespace ::android::fingerprint::virt;
using namespace ::aidl::android::hardware::biometrics::fingerprint;
using namespace ::aidl::android::hardware::keymaster;

namespace aidl::android::hardware::biometrics::fingerprint {

class TestSessionCallback : public BnSessionCallback {
  public:
    ndk::ScopedAStatus onChallengeGenerated(int64_t challenge) override {
        mLastChallenge = challenge;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onChallengeRevoked(int64_t challenge) override {
        mLastChallengeRevoked = challenge;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onError(fingerprint::Error error, int32_t) override {
        mError = error;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onEnrollmentProgress(int32_t enrollmentId, int32_t remaining) override {
        if (remaining == 0) mLastEnrolled = enrollmentId;
        return ndk::ScopedAStatus::ok();
    };

    ::ndk::ScopedAStatus onAuthenticationSucceeded(int32_t enrollmentId,
                                                   const keymaster::HardwareAuthToken&) override {
        mLastAuthenticated = enrollmentId;
        mAuthenticateFailed = false;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onAuthenticationFailed() override {
        mLastAuthenticated = 0;
        mAuthenticateFailed = true;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onInteractionDetected() override {
        mInteractionDetectedCount++;
        return ndk::ScopedAStatus::ok();
    };
    ndk::ScopedAStatus onAcquired(AcquiredInfo /*info*/, int32_t /*vendorCode*/) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onEnrollmentsEnumerated(
            const std::vector<int32_t>& enrollmentIds) override {
        mLastEnrollmentEnumerated = enrollmentIds;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onEnrollmentsRemoved(const std::vector<int32_t>& enrollmentIds) override {
        mLastEnrollmentRemoved = enrollmentIds;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onAuthenticatorIdRetrieved(int64_t authenticatorId) override {
        mLastAuthenticatorId = authenticatorId;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onAuthenticatorIdInvalidated(int64_t authenticatorId) override {
        mLastAuthenticatorId = authenticatorId;
        mAuthenticatorIdInvalidated = true;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onLockoutPermanent() override {
        mLockoutPermanent = true;
        return ndk::ScopedAStatus::ok();
    };
    ndk::ScopedAStatus onLockoutTimed(int64_t /* timeout */) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus onLockoutCleared() override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus onSessionClosed() override { return ndk::ScopedAStatus::ok(); }

    Error mError = Error::UNKNOWN;
    int64_t mLastChallenge = -1;
    int64_t mLastChallengeRevoked = -1;
    int32_t mLastEnrolled = -1;
    int32_t mLastAuthenticated = -1;
    int64_t mLastAuthenticatorId = -1;
    std::vector<int32_t> mLastEnrollmentEnumerated;
    std::vector<int32_t> mLastEnrollmentRemoved;
    bool mAuthenticateFailed = false;
    bool mAuthenticatorIdInvalidated = false;
    bool mLockoutPermanent = false;
    int mInteractionDetectedCount = 0;
};

class FakeFingerprintEngineTest : public ::testing::Test {
  protected:
    void SetUp() override {
        FingerprintHalProperties::operation_enroll_latency(0);
        FingerprintHalProperties::operation_authenticate_latency(0);
        FingerprintHalProperties::operation_detect_interaction_latency(0);
        mCallback = ndk::SharedRefBase::make<TestSessionCallback>();
    }

    FakeFingerprintEngine mEngine;
    std::shared_ptr<TestSessionCallback> mCallback;
    std::promise<void> mCancel;
};

TEST_F(FakeFingerprintEngineTest, GenerateChallenge) {
    mEngine.generateChallengeImpl(mCallback.get());
    ASSERT_EQ(FingerprintHalProperties::challenge().value(), mCallback->mLastChallenge);
}

TEST_F(FakeFingerprintEngineTest, RevokeChallenge) {
    auto challenge = FingerprintHalProperties::challenge().value_or(10);
    mEngine.revokeChallengeImpl(mCallback.get(), challenge);
    ASSERT_FALSE(FingerprintHalProperties::challenge().has_value());
    ASSERT_EQ(challenge, mCallback->mLastChallengeRevoked);
}

TEST_F(FakeFingerprintEngineTest, ResetLockout) {
    FingerprintHalProperties::lockout(true);
    mEngine.resetLockoutImpl(mCallback.get(), {});
    ASSERT_FALSE(FingerprintHalProperties::lockout().value_or(true));
}

TEST_F(FakeFingerprintEngineTest, AuthenticatorId) {
    FingerprintHalProperties::authenticator_id(50);
    mEngine.getAuthenticatorIdImpl(mCallback.get());
    ASSERT_EQ(50, mCallback->mLastAuthenticatorId);
    ASSERT_FALSE(mCallback->mAuthenticatorIdInvalidated);
}

TEST_F(FakeFingerprintEngineTest, AuthenticatorIdInvalidate) {
    FingerprintHalProperties::authenticator_id(500);
    mEngine.invalidateAuthenticatorIdImpl(mCallback.get());
    ASSERT_NE(500, FingerprintHalProperties::authenticator_id().value());
    ASSERT_TRUE(mCallback->mAuthenticatorIdInvalidated);
}

TEST_F(FakeFingerprintEngineTest, Enroll) {
    FingerprintHalProperties::enrollments({});
    FingerprintHalProperties::next_enrollment("4:0,0:true");
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.enrollImpl(mCallback.get(), hat, mCancel.get_future());
    ASSERT_FALSE(FingerprintHalProperties::next_enrollment().has_value());
    ASSERT_EQ(1, FingerprintHalProperties::enrollments().size());
    ASSERT_EQ(4, FingerprintHalProperties::enrollments()[0].value());
    ASSERT_EQ(4, mCallback->mLastEnrolled);
}

TEST_F(FakeFingerprintEngineTest, EnrollCancel) {
    FingerprintHalProperties::enrollments({});
    auto next = "4:0,0:true";
    FingerprintHalProperties::next_enrollment(next);
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mCancel.set_value();
    mEngine.enrollImpl(mCallback.get(), hat, mCancel.get_future());
    ASSERT_EQ(Error::CANCELED, mCallback->mError);
    ASSERT_EQ(-1, mCallback->mLastEnrolled);
    ASSERT_EQ(0, FingerprintHalProperties::enrollments().size());
    ASSERT_EQ(next, FingerprintHalProperties::next_enrollment().value_or(""));
}

TEST_F(FakeFingerprintEngineTest, EnrollFail) {
    FingerprintHalProperties::enrollments({});
    auto next = "2:0,0:false";
    FingerprintHalProperties::next_enrollment(next);
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.enrollImpl(mCallback.get(), hat, mCancel.get_future());
    ASSERT_EQ(Error::UNABLE_TO_PROCESS, mCallback->mError);
    ASSERT_EQ(-1, mCallback->mLastEnrolled);
    ASSERT_EQ(0, FingerprintHalProperties::enrollments().size());
    ASSERT_FALSE(FingerprintHalProperties::next_enrollment().has_value());
}

TEST_F(FakeFingerprintEngineTest, Authenticate) {
    FingerprintHalProperties::enrollments({1, 2});
    FingerprintHalProperties::enrollment_hit(2);
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    ASSERT_FALSE(mCallback->mAuthenticateFailed);
    ASSERT_EQ(2, mCallback->mLastAuthenticated);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateCancel) {
    FingerprintHalProperties::enrollments({2});
    FingerprintHalProperties::enrollment_hit(2);
    mCancel.set_value();
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    ASSERT_EQ(Error::CANCELED, mCallback->mError);
    ASSERT_EQ(-1, mCallback->mLastAuthenticated);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateNotSet) {
    FingerprintHalProperties::enrollments({1, 2});
    FingerprintHalProperties::enrollment_hit({});
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    ASSERT_TRUE(mCallback->mAuthenticateFailed);
    ASSERT_EQ(mCallback->mError, Error::UNABLE_TO_PROCESS);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateNotEnrolled) {
    FingerprintHalProperties::enrollments({1, 2});
    FingerprintHalProperties::enrollment_hit(3);
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    ASSERT_TRUE(mCallback->mAuthenticateFailed);
    ASSERT_EQ(mCallback->mError, Error::UNABLE_TO_PROCESS);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateLockout) {
    FingerprintHalProperties::enrollments({22, 2});
    FingerprintHalProperties::enrollment_hit(2);
    FingerprintHalProperties::lockout(true);
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    ASSERT_TRUE(mCallback->mLockoutPermanent);
    ASSERT_NE(mCallback->mError, Error::UNKNOWN);
}

TEST_F(FakeFingerprintEngineTest, InteractionDetect) {
    FingerprintHalProperties::enrollments({1, 2});
    FingerprintHalProperties::enrollment_hit(2);
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    ASSERT_EQ(1, mCallback->mInteractionDetectedCount);
}

TEST_F(FakeFingerprintEngineTest, InteractionDetectCancel) {
    FingerprintHalProperties::enrollments({1, 2});
    FingerprintHalProperties::enrollment_hit(2);
    mCancel.set_value();
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    ASSERT_EQ(Error::CANCELED, mCallback->mError);
    ASSERT_EQ(0, mCallback->mInteractionDetectedCount);
}

TEST_F(FakeFingerprintEngineTest, InteractionDetectNotSet) {
    FingerprintHalProperties::enrollments({1, 2});
    FingerprintHalProperties::enrollment_hit({});
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    ASSERT_EQ(0, mCallback->mInteractionDetectedCount);
}

TEST_F(FakeFingerprintEngineTest, InteractionDetectNotEnrolled) {
    FingerprintHalProperties::enrollments({1, 2});
    FingerprintHalProperties::enrollment_hit(25);
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    ASSERT_EQ(0, mCallback->mInteractionDetectedCount);
}

TEST_F(FakeFingerprintEngineTest, EnumerateEnrolled) {
    FingerprintHalProperties::enrollments({2, 4, 8});
    mEngine.enumerateEnrollmentsImpl(mCallback.get());
    ASSERT_EQ(
            4,
            mCallback->mLastEnrollmentEnumerated.size());  // Due to workaround. TODO (b/243129174)
    for (auto id : FingerprintHalProperties::enrollments()) {
        ASSERT_TRUE(std::find(mCallback->mLastEnrollmentEnumerated.begin(),
                              mCallback->mLastEnrollmentEnumerated.end(),
                              id) != mCallback->mLastEnrollmentEnumerated.end());
    }
}

TEST_F(FakeFingerprintEngineTest, RemoveEnrolled) {
    FingerprintHalProperties::enrollments({2, 4, 8, 1});
    mEngine.removeEnrollmentsImpl(mCallback.get(), {2, 8});
    auto enrolls = FingerprintHalProperties::enrollments();
    ASSERT_EQ(2, mCallback->mLastEnrollmentRemoved.size());
    for (auto id : {2, 8}) {
        ASSERT_TRUE(std::find(mCallback->mLastEnrollmentRemoved.begin(),
                              mCallback->mLastEnrollmentRemoved.end(),
                              id) != mCallback->mLastEnrollmentRemoved.end());
    }
    ASSERT_EQ(2, enrolls.size());
    for (auto id : {1, 4}) {
        ASSERT_TRUE(std::find(enrolls.begin(), enrolls.end(), id) != enrolls.end());
    }
}

}  // namespace aidl::android::hardware::biometrics::fingerprint

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
