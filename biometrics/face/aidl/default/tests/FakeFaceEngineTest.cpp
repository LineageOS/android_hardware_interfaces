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
#include <face.sysprop.h>
#include <gtest/gtest.h>

#include <aidl/android/hardware/biometrics/face/BnSessionCallback.h>
#include <android-base/logging.h>

#include "FakeFaceEngine.h"

using namespace ::android::face::virt;
using namespace ::aidl::android::hardware::biometrics::face;
using namespace ::aidl::android::hardware::keymaster;

namespace aidl::android::hardware::biometrics::face {

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
    ::ndk::ScopedAStatus onError(Error error, int32_t) override {
        mError = error;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onEnrollmentProgress(int32_t enrollmentId, int32_t remaining) override {
        if (remaining == 0) mLastEnrolled = enrollmentId;
        mRemaining = remaining;
        return ndk::ScopedAStatus::ok();
    };

    ::ndk::ScopedAStatus onAuthenticationSucceeded(int32_t enrollmentId,
                                                   const HardwareAuthToken&) override {
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

    ::ndk::ScopedAStatus onEnrollmentFrame(const EnrollmentFrame& frame) override {
        mEnrollmentFrames.push_back(frame.data.vendorCode);
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onEnrollmentsEnumerated(
            const std::vector<int32_t>& enrollmentIds) override {
        mLastEnrollmentsEnumerated = enrollmentIds;
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
    ::ndk::ScopedAStatus onAuthenticationFrame(const AuthenticationFrame& /*authFrame*/) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onLockoutPermanent() override {
        mLockoutPermanent = true;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onLockoutTimed(int64_t /* timeout */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onLockoutCleared() override {
        mLockoutPermanent = false;
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onSessionClosed() override { return ndk::ScopedAStatus::ok(); }

    ::ndk::ScopedAStatus onFeaturesRetrieved(const std::vector<Feature>& features) override {
        mFeatures = features;
        return ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onFeatureSet(Feature feature) override {
        mLastFeatureSet = feature;
        return ndk::ScopedAStatus::ok();
    }

    Error mError = Error::UNKNOWN;
    int64_t mLastChallenge = -1;
    int64_t mLastChallengeRevoked = -1;
    int32_t mLastEnrolled = -1;
    int32_t mLastAuthenticated = -1;
    int64_t mLastAuthenticatorId = -1;
    std::vector<int32_t> mLastEnrollmentsEnumerated;
    std::vector<int32_t> mLastEnrollmentRemoved;
    std::vector<Feature> mFeatures;
    Feature mLastFeatureSet;
    std::vector<int32_t> mEnrollmentFrames;
    bool mAuthenticateFailed = false;
    bool mAuthenticatorIdInvalidated = false;
    bool mLockoutPermanent = false;
    int mInteractionDetectedCount = 0;
    int mRemaining = -1;
};

class FakeFaceEngineTest : public ::testing::Test {
  protected:
    void SetUp() override {
        LOG(ERROR) << "JRM SETUP";
        mCallback = ndk::SharedRefBase::make<TestSessionCallback>();
        FaceHalProperties::enrollments({});
        FaceHalProperties::challenge({});
        FaceHalProperties::features({});
        FaceHalProperties::authenticator_id({});
        FaceHalProperties::strength("");
    }

    FakeFaceEngine mEngine;
    std::shared_ptr<TestSessionCallback> mCallback;
    std::promise<void> mCancel;
};

TEST_F(FakeFaceEngineTest, one_eq_one) {
    ASSERT_EQ(1, 1);
}

TEST_F(FakeFaceEngineTest, GenerateChallenge) {
    mEngine.generateChallengeImpl(mCallback.get());
    ASSERT_EQ(FaceHalProperties::challenge().value(), mCallback->mLastChallenge);
}

TEST_F(FakeFaceEngineTest, RevokeChallenge) {
    auto challenge = FaceHalProperties::challenge().value_or(10);
    mEngine.revokeChallengeImpl(mCallback.get(), challenge);
    ASSERT_FALSE(FaceHalProperties::challenge().has_value());
    ASSERT_EQ(challenge, mCallback->mLastChallengeRevoked);
}

TEST_F(FakeFaceEngineTest, ResetLockout) {
    FaceHalProperties::lockout(true);
    mEngine.resetLockoutImpl(mCallback.get(), {});
    ASSERT_FALSE(mCallback->mLockoutPermanent);
    ASSERT_FALSE(FaceHalProperties::lockout().value_or(true));
}

TEST_F(FakeFaceEngineTest, AuthenticatorId) {
    FaceHalProperties::authenticator_id(50);
    mEngine.getAuthenticatorIdImpl(mCallback.get());
    ASSERT_EQ(50, mCallback->mLastAuthenticatorId);
    ASSERT_FALSE(mCallback->mAuthenticatorIdInvalidated);
}

TEST_F(FakeFaceEngineTest, GetAuthenticatorIdWeakReturnsZero) {
    FaceHalProperties::strength("weak");
    FaceHalProperties::authenticator_id(500);
    mEngine.getAuthenticatorIdImpl(mCallback.get());
    ASSERT_EQ(0, mCallback->mLastAuthenticatorId);
    ASSERT_FALSE(mCallback->mAuthenticatorIdInvalidated);
}

TEST_F(FakeFaceEngineTest, AuthenticatorIdInvalidate) {
    FaceHalProperties::authenticator_id(500);
    mEngine.invalidateAuthenticatorIdImpl(mCallback.get());
    ASSERT_NE(500, FaceHalProperties::authenticator_id().value());
    ASSERT_TRUE(mCallback->mAuthenticatorIdInvalidated);
}

TEST_F(FakeFaceEngineTest, Enroll) {
    FaceHalProperties::next_enrollment("1,0:1000-[21,5,6,7,1],1100-[1118,1108,1]:true");
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.enrollImpl(mCallback.get(), hat, {} /*enrollmentType*/, {} /*features*/,
                       mCancel.get_future());
    ASSERT_FALSE(FaceHalProperties::next_enrollment().has_value());
    ASSERT_EQ(1, FaceHalProperties::enrollments().size());
    ASSERT_EQ(1, FaceHalProperties::enrollments()[0].value());
    ASSERT_EQ(1, mCallback->mLastEnrolled);
    ASSERT_EQ(0, mCallback->mRemaining);
}

TEST_F(FakeFaceEngineTest, EnrollFails) {
    FaceHalProperties::next_enrollment("1,0:1000-[21,5,6,7,1],1100-[1118,1108,1]:false");
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.enrollImpl(mCallback.get(), hat, {} /*enrollmentType*/, {} /*features*/,
                       mCancel.get_future());
    ASSERT_FALSE(FaceHalProperties::next_enrollment().has_value());
    ASSERT_EQ(0, FaceHalProperties::enrollments().size());
}

TEST_F(FakeFaceEngineTest, EnrollCancel) {
    FaceHalProperties::next_enrollment("1:2000-[21,8,9],300:false");
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mCancel.set_value();
    mEngine.enrollImpl(mCallback.get(), hat, {} /*enrollmentType*/, {} /*features*/,
                       mCancel.get_future());
    ASSERT_EQ(Error::CANCELED, mCallback->mError);
    ASSERT_EQ(-1, mCallback->mLastEnrolled);
    ASSERT_EQ(0, FaceHalProperties::enrollments().size());
    ASSERT_TRUE(FaceHalProperties::next_enrollment().has_value());
}

TEST_F(FakeFaceEngineTest, Authenticate) {
    FaceHalProperties::enrollments({100});
    FaceHalProperties::enrollment_hit(100);
    mEngine.authenticateImpl(mCallback.get(), 0 /* operationId*/, mCancel.get_future());

    ASSERT_EQ(100, mCallback->mLastAuthenticated);
    ASSERT_FALSE(mCallback->mAuthenticateFailed);
}

TEST_F(FakeFaceEngineTest, AuthenticateCancel) {
    FaceHalProperties::enrollments({100});
    FaceHalProperties::enrollment_hit(100);
    mCancel.set_value();
    mEngine.authenticateImpl(mCallback.get(), 0 /* operationId*/, mCancel.get_future());
    ASSERT_EQ(Error::CANCELED, mCallback->mError);
}

TEST_F(FakeFaceEngineTest, AuthenticateFailedForUnEnrolled) {
    FaceHalProperties::enrollments({3});
    FaceHalProperties::enrollment_hit(100);
    mEngine.authenticateImpl(mCallback.get(), 0 /* operationId*/, mCancel.get_future());
    ASSERT_EQ(Error::TIMEOUT, mCallback->mError);
    ASSERT_TRUE(mCallback->mAuthenticateFailed);
}

TEST_F(FakeFaceEngineTest, DetectInteraction) {
    FaceHalProperties::enrollments({100});
    FaceHalProperties::enrollment_hit(100);
    ASSERT_EQ(0, mCallback->mInteractionDetectedCount);
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    ASSERT_EQ(1, mCallback->mInteractionDetectedCount);
}

TEST_F(FakeFaceEngineTest, DetectInteractionCancel) {
    FaceHalProperties::enrollments({100});
    FaceHalProperties::enrollment_hit(100);
    mCancel.set_value();
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    ASSERT_EQ(Error::CANCELED, mCallback->mError);
}

TEST_F(FakeFaceEngineTest, GetFeatureEmpty) {
    mEngine.getFeaturesImpl(mCallback.get());
    ASSERT_TRUE(mCallback->mFeatures.empty());
}

TEST_F(FakeFaceEngineTest, SetFeature) {
    FaceHalProperties::enrollments({1});
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::REQUIRE_ATTENTION, true);
    auto features = mCallback->mFeatures;
    ASSERT_TRUE(features.empty());
    ASSERT_EQ(Feature::REQUIRE_ATTENTION, mCallback->mLastFeatureSet);

    mEngine.getFeaturesImpl(mCallback.get());
    features = mCallback->mFeatures;
    ASSERT_FALSE(features.empty());
    ASSERT_NE(features.end(),
              std::find(features.begin(), features.end(), Feature::REQUIRE_ATTENTION));
}

TEST_F(FakeFaceEngineTest, ToggleFeature) {
    FaceHalProperties::enrollments({1});
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::REQUIRE_ATTENTION, true);
    mEngine.getFeaturesImpl(mCallback.get());
    auto features = mCallback->mFeatures;
    ASSERT_FALSE(features.empty());
    ASSERT_NE(features.end(),
              std::find(features.begin(), features.end(), Feature::REQUIRE_ATTENTION));

    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::REQUIRE_ATTENTION, false);
    mEngine.getFeaturesImpl(mCallback.get());
    features = mCallback->mFeatures;
    ASSERT_TRUE(features.empty());
}

TEST_F(FakeFaceEngineTest, TurningOffNonExistentFeatureDoesNothing) {
    FaceHalProperties::enrollments({1});
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::REQUIRE_ATTENTION, false);
    mEngine.getFeaturesImpl(mCallback.get());
    auto features = mCallback->mFeatures;
    ASSERT_TRUE(features.empty());
}

TEST_F(FakeFaceEngineTest, SetMultipleFeatures) {
    FaceHalProperties::enrollments({1});
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::REQUIRE_ATTENTION, true);
    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::REQUIRE_DIVERSE_POSES, true);
    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::DEBUG, true);
    mEngine.getFeaturesImpl(mCallback.get());
    auto features = mCallback->mFeatures;
    ASSERT_EQ(3, features.size());
    ASSERT_NE(features.end(),
              std::find(features.begin(), features.end(), Feature::REQUIRE_ATTENTION));
    ASSERT_NE(features.end(),
              std::find(features.begin(), features.end(), Feature::REQUIRE_DIVERSE_POSES));
    ASSERT_NE(features.end(), std::find(features.begin(), features.end(), Feature::DEBUG));
}

TEST_F(FakeFaceEngineTest, SetMultipleFeaturesAndTurnOffSome) {
    FaceHalProperties::enrollments({1});
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::REQUIRE_ATTENTION, true);
    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::REQUIRE_DIVERSE_POSES, true);
    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::DEBUG, true);
    mEngine.setFeatureImpl(mCallback.get(), hat, Feature::DEBUG, false);
    mEngine.getFeaturesImpl(mCallback.get());
    auto features = mCallback->mFeatures;
    ASSERT_EQ(2, features.size());
    ASSERT_NE(features.end(),
              std::find(features.begin(), features.end(), Feature::REQUIRE_ATTENTION));
    ASSERT_NE(features.end(),
              std::find(features.begin(), features.end(), Feature::REQUIRE_DIVERSE_POSES));
    ASSERT_EQ(features.end(), std::find(features.begin(), features.end(), Feature::DEBUG));
}

TEST_F(FakeFaceEngineTest, Enumerate) {
    FaceHalProperties::enrollments({120, 3});
    mEngine.enumerateEnrollmentsImpl(mCallback.get());
    auto enrolls = mCallback->mLastEnrollmentsEnumerated;
    ASSERT_FALSE(enrolls.empty());
    ASSERT_NE(enrolls.end(), std::find(enrolls.begin(), enrolls.end(), 120));
    ASSERT_NE(enrolls.end(), std::find(enrolls.begin(), enrolls.end(), 3));
}

TEST_F(FakeFaceEngineTest, RemoveEnrollments) {
    FaceHalProperties::enrollments({120, 3, 100});
    mEngine.removeEnrollmentsImpl(mCallback.get(), {120, 100});
    mEngine.enumerateEnrollmentsImpl(mCallback.get());
    auto enrolls = mCallback->mLastEnrollmentsEnumerated;
    ASSERT_FALSE(enrolls.empty());
    ASSERT_EQ(enrolls.end(), std::find(enrolls.begin(), enrolls.end(), 120));
    ASSERT_NE(enrolls.end(), std::find(enrolls.begin(), enrolls.end(), 3));
    ASSERT_EQ(enrolls.end(), std::find(enrolls.begin(), enrolls.end(), 100));
}

TEST_F(FakeFaceEngineTest, ResetLockoutWithAuth) {
    FaceHalProperties::lockout(true);
    FaceHalProperties::enrollments({33});
    FaceHalProperties::enrollment_hit(33);
    auto cancelFuture = mCancel.get_future();
    mEngine.authenticateImpl(mCallback.get(), 0 /* operationId*/, cancelFuture);

    ASSERT_TRUE(mCallback->mLockoutPermanent);

    mEngine.resetLockoutImpl(mCallback.get(), {} /* hat */);
    ASSERT_FALSE(mCallback->mLockoutPermanent);
    FaceHalProperties::enrollment_hit(33);
    mEngine.authenticateImpl(mCallback.get(), 0 /* operationId*/, cancelFuture);
    ASSERT_EQ(33, mCallback->mLastAuthenticated);
    ASSERT_FALSE(mCallback->mAuthenticateFailed);
}

}  // namespace aidl::android::hardware::biometrics::face
