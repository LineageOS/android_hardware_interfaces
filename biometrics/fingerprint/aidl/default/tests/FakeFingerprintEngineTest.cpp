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

#include <aidl/android/hardware/biometrics/fingerprint/BnSessionCallback.h>

#include "FakeFingerprintEngine.h"
#include "Fingerprint.h"
#include "util/Util.h"

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
    ::ndk::ScopedAStatus onError(fingerprint::Error error, int32_t vendorCode) override {
        mError = error;
        mErrorVendorCode = vendorCode;
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
    ndk::ScopedAStatus onAcquired(AcquiredInfo info, int32_t vendorCode) override {
        mLastAcquiredInfo = (int32_t)info;
        mLastAcquiredVendorCode = vendorCode;
        mLastAcquiredCount++;
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
        mLockoutTimed = true;
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus onLockoutCleared() override {
        mLockoutCleared = true;
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus onSessionClosed() override { return ndk::ScopedAStatus::ok(); }

    Error mError = Error::UNKNOWN;
    int32_t mErrorVendorCode = 0;
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
    bool mLockoutTimed = false;
    bool mLockoutCleared = false;
    int mInteractionDetectedCount = 0;
    int32_t mLastAcquiredInfo = -1;
    int32_t mLastAcquiredVendorCode = -1;
    int32_t mLastAcquiredCount = 0;
};

class FakeFingerprintEngineTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Fingerprint::cfg().setopt<OptIntVec>("operation_enroll_latency", {0});
        Fingerprint::cfg().setopt<OptIntVec>("operation_authenticate_latency", {0});
        Fingerprint::cfg().setopt<OptIntVec>("operation_detect_interaction_latency", {0});
        mCallback = ndk::SharedRefBase::make<TestSessionCallback>();
    }

    void TearDown() override {
        Fingerprint::cfg().set<std::int32_t>("operation_authenticate_error", 0);
        Fingerprint::cfg().set<std::int32_t>("operation_detect_interaction_error", 0);
        Fingerprint::cfg().set<std::string>("operation_authenticate_acquired", "");
        Fingerprint::cfg().setopt<OptIntVec>("operation_enroll_latency", {});
        Fingerprint::cfg().setopt<OptIntVec>("operation_authenticate_latency", {});
        Fingerprint::cfg().setopt<OptIntVec>("operation_detect_interaction_latency", {});
        Fingerprint::cfg().set<bool>("operation_authenticate_fails", false);
    }

    FakeFingerprintEngine mEngine;
    std::shared_ptr<TestSessionCallback> mCallback;
    std::promise<void> mCancel;
};

TEST_F(FakeFingerprintEngineTest, GenerateChallenge) {
    mEngine.generateChallengeImpl(mCallback.get());
    ASSERT_EQ(Fingerprint::cfg().get<std::int64_t>("challenge"), mCallback->mLastChallenge);
}

TEST_F(FakeFingerprintEngineTest, RevokeChallenge) {
    auto challenge = Fingerprint::cfg().get<std::int64_t>("challenge");
    mEngine.revokeChallengeImpl(mCallback.get(), challenge);
    ASSERT_FALSE((Fingerprint::cfg().getopt<OptInt64>("challenge")).has_value());
    ASSERT_EQ(challenge, mCallback->mLastChallengeRevoked);
}

TEST_F(FakeFingerprintEngineTest, ResetLockout) {
    Fingerprint::cfg().get<bool>("lockout");
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.resetLockoutImpl(mCallback.get(), hat);
    ASSERT_FALSE(Fingerprint::cfg().get<bool>("lockout"));
}

TEST_F(FakeFingerprintEngineTest, AuthenticatorId) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {1});
    Fingerprint::cfg().set<std::int64_t>("authenticator_id", 50);
    mEngine.getAuthenticatorIdImpl(mCallback.get());
    ASSERT_EQ(50, mCallback->mLastAuthenticatorId);
    ASSERT_FALSE(mCallback->mAuthenticatorIdInvalidated);
}

TEST_F(FakeFingerprintEngineTest, AuthenticatorIdInvalidate) {
    Fingerprint::cfg().set<std::int64_t>("authenticator_id", 500);
    mEngine.invalidateAuthenticatorIdImpl(mCallback.get());
    ASSERT_NE(500, Fingerprint::cfg().get<std::int64_t>("authenticator_id"));
    ASSERT_TRUE(mCallback->mAuthenticatorIdInvalidated);
}

TEST_F(FakeFingerprintEngineTest, Enroll) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {});
    Fingerprint::cfg().set<std::string>("next_enrollment", "4:0,0:true");
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.notifyFingerdown();
    mEngine.enrollImpl(mCallback.get(), hat, mCancel.get_future());
    ASSERT_EQ(mEngine.getWorkMode(), FakeFingerprintEngine::WorkMode::kEnroll);
    mEngine.fingerDownAction();
    ASSERT_FALSE(Fingerprint::cfg().getopt<OptString>("next_enrollment").has_value());
    ASSERT_EQ(1, Fingerprint::cfg().getopt<OptIntVec>("enrollments").size());
    ASSERT_EQ(4, Fingerprint::cfg().getopt<OptIntVec>("enrollments")[0].value());
    ASSERT_EQ(4, mCallback->mLastEnrolled);
    ASSERT_EQ(1, mCallback->mLastAcquiredInfo);
    ASSERT_EQ(mEngine.getWorkMode(), FakeFingerprintEngine::WorkMode::kIdle);
}

TEST_F(FakeFingerprintEngineTest, EnrollCancel) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {});
    auto next = "4:0,0:true";
    Fingerprint::cfg().set<std::string>("next_enrollment", next);
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mCancel.set_value();
    mEngine.notifyFingerdown();
    mEngine.enrollImpl(mCallback.get(), hat, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_EQ(Error::CANCELED, mCallback->mError);
    ASSERT_EQ(-1, mCallback->mLastEnrolled);
    ASSERT_EQ(0, Fingerprint::cfg().getopt<OptIntVec>("enrollments").size());
    ASSERT_EQ(next, Fingerprint::cfg().get<std::string>("next_enrollment"));
}

TEST_F(FakeFingerprintEngineTest, EnrollFail) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {});
    auto next = "2:0,0:false";
    Fingerprint::cfg().set<std::string>("next_enrollment", next);
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    mEngine.notifyFingerdown();
    mEngine.enrollImpl(mCallback.get(), hat, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_EQ(Error::UNABLE_TO_PROCESS, mCallback->mError);
    ASSERT_EQ(-1, mCallback->mLastEnrolled);
    ASSERT_EQ(0, Fingerprint::cfg().getopt<OptIntVec>("enrollments").size());
    ASSERT_FALSE(Fingerprint::cfg().getopt<OptString>("next_enrollment").has_value());
}

TEST_F(FakeFingerprintEngineTest, EnrollAcquired) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {});
    Fingerprint::cfg().set<std::string>("next_enrollment", "4:0,5-[12,1013]:true");
    keymaster::HardwareAuthToken hat{.mac = {2, 4}};
    int32_t prevCnt = mCallback->mLastAcquiredCount;
    mEngine.notifyFingerdown();
    mEngine.enrollImpl(mCallback.get(), hat, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_FALSE(Fingerprint::cfg().getopt<OptString>("next_enrollment").has_value());
    ASSERT_EQ(1, Fingerprint::cfg().getopt<OptIntVec>("enrollments").size());
    ASSERT_EQ(4, Fingerprint::cfg().getopt<OptIntVec>("enrollments")[0].value());
    ASSERT_EQ(4, mCallback->mLastEnrolled);
    ASSERT_EQ(prevCnt + 3, mCallback->mLastAcquiredCount);
    ASSERT_EQ(7, mCallback->mLastAcquiredInfo);
    ASSERT_EQ(13, mCallback->mLastAcquiredVendorCode);
}

TEST_F(FakeFingerprintEngineTest, Authenticate) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {1, 2});
    Fingerprint::cfg().set<std::int32_t>("enrollment_hit", 2);
    mEngine.notifyFingerdown();
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    ASSERT_EQ(mEngine.getWorkMode(), FakeFingerprintEngine::WorkMode::kAuthenticate);
    mEngine.fingerDownAction();
    ASSERT_FALSE(mCallback->mAuthenticateFailed);
    ASSERT_EQ(2, mCallback->mLastAuthenticated);
    ASSERT_EQ(1, mCallback->mLastAcquiredInfo);
    ASSERT_EQ(mEngine.getWorkMode(), FakeFingerprintEngine::WorkMode::kIdle);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateCancel) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {2});
    Fingerprint::cfg().set<std::int32_t>("enrollment_hit", 2);
    mCancel.set_value();
    mEngine.notifyFingerdown();
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_EQ(Error::CANCELED, mCallback->mError);
    ASSERT_EQ(-1, mCallback->mLastAuthenticated);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateNotSet) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {1, 2});
    Fingerprint::cfg().setopt<OptInt32>("enrollment_hit", std::nullopt);
    mEngine.notifyFingerdown();
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_TRUE(mCallback->mAuthenticateFailed);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateNotEnrolled) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {1, 2});
    Fingerprint::cfg().set<std::int32_t>("enrollment_hit", 3);
    mEngine.notifyFingerdown();
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_TRUE(mCallback->mAuthenticateFailed);
    ASSERT_EQ(mEngine.getWorkMode(), FakeFingerprintEngine::WorkMode::kAuthenticate);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateLockout) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {22, 2});
    Fingerprint::cfg().set<std::int32_t>("enrollment_hit", 2);
    Fingerprint::cfg().set<bool>("lockout", true);
    mEngine.notifyFingerdown();
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_TRUE(mCallback->mLockoutPermanent);
    ASSERT_NE(mCallback->mError, Error::UNKNOWN);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateError8) {
    Fingerprint::cfg().set<std::int32_t>("operation_authenticate_error", 8);
    mEngine.notifyFingerdown();
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_EQ(mCallback->mError, (Error)8);
    ASSERT_EQ(mCallback->mErrorVendorCode, 0);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateError9) {
    Fingerprint::cfg().set<std::int32_t>("operation_authenticate_error", 1009);
    mEngine.notifyFingerdown();
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_EQ(mCallback->mError, (Error)7);
    ASSERT_EQ(mCallback->mErrorVendorCode, 9);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateFails) {
    Fingerprint::cfg().set<bool>("operation_authenticate_fails", true);
    mEngine.notifyFingerdown();
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_TRUE(mCallback->mAuthenticateFailed);
    ASSERT_EQ(mEngine.getWorkMode(), FakeFingerprintEngine::WorkMode::kAuthenticate);
}

TEST_F(FakeFingerprintEngineTest, AuthenticateAcquired) {
    Fingerprint::cfg().set<bool>("lockout", false);
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {1, 2});
    Fingerprint::cfg().set<std::int32_t>("enrollment_hit", 2);
    Fingerprint::cfg().set<std::string>("operation_authenticate_acquired", "4,1009");
    int32_t prevCount = mCallback->mLastAcquiredCount;
    mEngine.notifyFingerdown();
    mEngine.authenticateImpl(mCallback.get(), 0, mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_FALSE(mCallback->mAuthenticateFailed);
    ASSERT_EQ(2, mCallback->mLastAuthenticated);
    ASSERT_EQ(prevCount + 2, mCallback->mLastAcquiredCount);
    ASSERT_EQ(7, mCallback->mLastAcquiredInfo);
    ASSERT_EQ(9, mCallback->mLastAcquiredVendorCode);
}

TEST_F(FakeFingerprintEngineTest, InteractionDetect) {
    Fingerprint::cfg().set<bool>("detect_interaction", true);
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {1, 2});
    Fingerprint::cfg().set<std::int32_t>("enrollment_hit", 2);
    Fingerprint::cfg().set<std::string>("operation_detect_interaction_acquired", "");
    mEngine.notifyFingerdown();
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    ASSERT_EQ(mEngine.getWorkMode(), FakeFingerprintEngine::WorkMode::kDetectInteract);
    mEngine.fingerDownAction();
    ASSERT_EQ(1, mCallback->mInteractionDetectedCount);
    ASSERT_EQ(1, mCallback->mLastAcquiredInfo);
    ASSERT_EQ(mEngine.getWorkMode(), FakeFingerprintEngine::WorkMode::kIdle);
}

TEST_F(FakeFingerprintEngineTest, InteractionDetectCancel) {
    Fingerprint::cfg().set<bool>("detect_interaction", true);
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {1, 2});
    Fingerprint::cfg().set<std::int32_t>("enrollment_hit", 2);
    mCancel.set_value();
    mEngine.notifyFingerdown();
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_EQ(Error::CANCELED, mCallback->mError);
    ASSERT_EQ(0, mCallback->mInteractionDetectedCount);
}

TEST_F(FakeFingerprintEngineTest, InteractionDetectNotSet) {
    Fingerprint::cfg().set<bool>("detect_interaction", true);
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {1, 2});
    Fingerprint::cfg().setopt<OptInt32>("enrollment_hit", std::nullopt);
    mEngine.notifyFingerdown();
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_EQ(1, mCallback->mInteractionDetectedCount);
}

TEST_F(FakeFingerprintEngineTest, InteractionDetectNotEnrolled) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {1, 2});
    Fingerprint::cfg().set<std::int32_t>("enrollment_hit", 25);
    mEngine.notifyFingerdown();
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_EQ(1, mCallback->mInteractionDetectedCount);
}

TEST_F(FakeFingerprintEngineTest, InteractionDetectError) {
    Fingerprint::cfg().set<bool>("detect_interaction", true);
    Fingerprint::cfg().set<std::int32_t>("operation_detect_interaction_error", 8);
    mEngine.notifyFingerdown();
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_EQ(0, mCallback->mInteractionDetectedCount);
    ASSERT_EQ(mCallback->mError, (Error)8);
    ASSERT_EQ(mCallback->mErrorVendorCode, 0);
}

TEST_F(FakeFingerprintEngineTest, InteractionDetectAcquired) {
    Fingerprint::cfg().set<bool>("detect_interaction", true);
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {1, 2});
    Fingerprint::cfg().set<std::int32_t>("enrollment_hit", 2);
    Fingerprint::cfg().set<std::string>("operation_detect_interaction_acquired", "4,1013");
    int32_t prevCount = mCallback->mLastAcquiredCount;
    mEngine.notifyFingerdown();
    mEngine.detectInteractionImpl(mCallback.get(), mCancel.get_future());
    mEngine.fingerDownAction();
    ASSERT_EQ(1, mCallback->mInteractionDetectedCount);
    ASSERT_EQ(prevCount + 2, mCallback->mLastAcquiredCount);
    ASSERT_EQ(7, mCallback->mLastAcquiredInfo);
    ASSERT_EQ(13, mCallback->mLastAcquiredVendorCode);
}

TEST_F(FakeFingerprintEngineTest, EnumerateEnrolled) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {2, 4, 8});
    mEngine.enumerateEnrollmentsImpl(mCallback.get());
    ASSERT_EQ(3, mCallback->mLastEnrollmentEnumerated.size());
    for (auto id : Fingerprint::cfg().getopt<OptIntVec>("enrollments")) {
        ASSERT_TRUE(std::find(mCallback->mLastEnrollmentEnumerated.begin(),
                              mCallback->mLastEnrollmentEnumerated.end(),
                              id) != mCallback->mLastEnrollmentEnumerated.end());
    }
}

TEST_F(FakeFingerprintEngineTest, RemoveEnrolled) {
    Fingerprint::cfg().setopt<OptIntVec>("enrollments", {2, 4, 8, 1});
    mEngine.removeEnrollmentsImpl(mCallback.get(), {2, 8});
    auto enrolls = Fingerprint::cfg().getopt<OptIntVec>("enrollments");
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

TEST_F(FakeFingerprintEngineTest, parseIntSequence) {
    std::vector<int32_t> seqV;
    seqV = Util::parseIntSequence("");
    ASSERT_EQ(0, seqV.size());
    seqV = Util::parseIntSequence("2");
    ASSERT_EQ(1, seqV.size());
    ASSERT_EQ(2, seqV[0]);
    seqV = Util::parseIntSequence("2,3,4");
    std::vector<int32_t> expV{2, 3, 4};
    ASSERT_EQ(expV, seqV);
    seqV = Util::parseIntSequence("2,3,a");
    ASSERT_EQ(0, seqV.size());
    seqV = Util::parseIntSequence("2, 3, 4");
    ASSERT_EQ(expV, seqV);
    seqV = Util::parseIntSequence("123,456");
    ASSERT_EQ(2, seqV.size());
    std::vector<int32_t> expV1{123, 456};
    ASSERT_EQ(expV1, seqV);
    seqV = Util::parseIntSequence("12f3,456");
    ASSERT_EQ(0, seqV.size());
}

TEST_F(FakeFingerprintEngineTest, parseEnrollmentCaptureOk) {
    std::vector<std::vector<int32_t>> ecV;
    ecV = Util::parseEnrollmentCapture("100,200,300");
    ASSERT_EQ(6, ecV.size());
    std::vector<std::vector<int32_t>> expE{{100}, {200}, {300}};
    std::vector<int32_t> defC{1};
    for (int i = 0; i < ecV.size(); i += 2) {
        ASSERT_EQ(expE[i / 2], ecV[i]);
        ASSERT_EQ(defC, ecV[i + 1]);
    }
    ecV = Util::parseEnrollmentCapture("100");
    ASSERT_EQ(2, ecV.size());
    ASSERT_EQ(expE[0], ecV[0]);
    ASSERT_EQ(defC, ecV[1]);

    ecV = Util::parseEnrollmentCapture("100-[5,6,7]");
    std::vector<int32_t> expC{5, 6, 7};
    ASSERT_EQ(2, ecV.size());
    for (int i = 0; i < ecV.size(); i += 2) {
        ASSERT_EQ(expE[i / 2], ecV[i]);
        ASSERT_EQ(expC, ecV[i + 1]);
    }
    ecV = Util::parseEnrollmentCapture("100-[5,6,7], 200, 300-[9,10]");
    std::vector<std::vector<int32_t>> expC1{{5, 6, 7}, {1}, {9, 10}};
    ASSERT_EQ(6, ecV.size());
    for (int i = 0; i < ecV.size(); i += 2) {
        ASSERT_EQ(expE[i / 2], ecV[i]);
        ASSERT_EQ(expC1[i / 2], ecV[i + 1]);
    }
    ecV = Util::parseEnrollmentCapture("100-[5,6,7], 200-[2,1], 300-[9]");
    std::vector<std::vector<int32_t>> expC2{{5, 6, 7}, {2, 1}, {9}};
    ASSERT_EQ(ecV.size(), 6);
    for (int i = 0; i < ecV.size(); i += 2) {
        ASSERT_EQ(expE[i / 2], ecV[i]);
        ASSERT_EQ(expC2[i / 2], ecV[i + 1]);
    }
}

TEST_F(FakeFingerprintEngineTest, parseEnrollmentCaptureFail) {
    std::vector<std::string> badStr{"10c",         "100-5",   "100-[5,6,7", "100-5,6,7]",
                                    "100,2x0,300", "200-[f]", "a,b"};
    std::vector<std::vector<int32_t>> ecV;
    for (const auto& s : badStr) {
        ecV = Util::parseEnrollmentCapture(s);
        ASSERT_EQ(ecV.size(), 0);
    }
}

TEST_F(FakeFingerprintEngineTest, randomLatency) {
    Fingerprint::cfg().setopt<OptIntVec>("operation_detect_interaction_latency", {});
    ASSERT_EQ(DEFAULT_LATENCY, mEngine.getLatency(Fingerprint::cfg().getopt<OptIntVec>(
                                       "operation_detect_interaction_latency")));
    Fingerprint::cfg().setopt<OptIntVec>("operation_detect_interaction_latency", {10});
    ASSERT_EQ(10, mEngine.getLatency(Fingerprint::cfg().getopt<OptIntVec>(
                          "operation_detect_interaction_latency")));
    Fingerprint::cfg().setopt<OptIntVec>("operation_detect_interaction_latency", {1, 1000});
    std::set<int32_t> latencySet;
    for (int i = 0; i < 100; i++) {
        latencySet.insert(mEngine.getLatency(
                Fingerprint::cfg().getopt<OptIntVec>("operation_detect_interaction_latency")));
    }
    ASSERT_TRUE(latencySet.size() > 95);
}

TEST_F(FakeFingerprintEngineTest, lockoutTimer) {
    mEngine.startLockoutTimer(200, mCallback.get());
    ASSERT_TRUE(mEngine.getLockoutTimerStarted());
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    ASSERT_FALSE(mEngine.getLockoutTimerStarted());
    ASSERT_TRUE(mCallback->mLockoutCleared);
}
}  // namespace aidl::android::hardware::biometrics::fingerprint

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
