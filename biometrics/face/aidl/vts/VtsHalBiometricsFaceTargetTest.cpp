/*
 * Copyright (C) 2020 The Android Open Source Project
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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/biometrics/face/BnFace.h>
#include <aidl/android/hardware/biometrics/face/BnSessionCallback.h>

#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <chrono>
#include <future>

namespace aidl::android::hardware::biometrics::face {
namespace {

using namespace std::literals::chrono_literals;

using aidl::android::hardware::common::NativeHandle;

constexpr int kSensorId = 0;
constexpr int kUserId = 0;

class SessionCallback : public BnSessionCallback {
  public:
    ndk::ScopedAStatus onChallengeGenerated(int64_t challenge) override {
        auto lock = std::lock_guard{mMutex};
        mOnChallengeGeneratedInvoked = true;
        mGeneratedChallenge = challenge;
        mCv.notify_one();
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onChallengeRevoked(int64_t challenge) override {
        auto lock = std::lock_guard{mMutex};
        mOnChallengeRevokedInvoked = true;
        mRevokedChallenge = challenge;
        mCv.notify_one();
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onAuthenticationFrame(const AuthenticationFrame& /*frame*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onEnrollmentFrame(const EnrollmentFrame& /*frame*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onError(Error error, int32_t vendorCode) override {
        auto lock = std::lock_guard<std::mutex>{mMutex};
        mError = error;
        mVendorCode = vendorCode;
        mOnErrorInvoked = true;
        mCv.notify_one();
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onEnrollmentProgress(int32_t /*enrollmentId*/,
                                            int32_t /*remaining*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onAuthenticationSucceeded(
            int32_t /*enrollmentId*/, const keymaster::HardwareAuthToken& /*hat*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onAuthenticationFailed() override { return ndk::ScopedAStatus::ok(); }

    ndk::ScopedAStatus onLockoutTimed(int64_t /*durationMillis*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onLockoutPermanent() override { return ndk::ScopedAStatus::ok(); }

    ndk::ScopedAStatus onLockoutCleared() override { return ndk::ScopedAStatus::ok(); }

    ndk::ScopedAStatus onInteractionDetected() override { return ndk::ScopedAStatus::ok(); }

    ndk::ScopedAStatus onEnrollmentsEnumerated(
            const std::vector<int32_t>& /*enrollmentIds*/) override {
        auto lock = std::lock_guard{mMutex};
        mOnEnrollmentsEnumeratedInvoked = true;
        mCv.notify_one();
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onEnrollmentsRemoved(
            const std::vector<int32_t>& /*enrollmentIds*/) override {
        auto lock = std::lock_guard{mMutex};
        mOnEnrollmentsRemovedInvoked = true;
        mCv.notify_one();
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onFeaturesRetrieved(const std::vector<Feature>& /*features*/) override {
        auto lock = std::lock_guard{mMutex};
        mOnFeaturesRetrievedInvoked = true;
        mCv.notify_one();
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onFeatureSet(Feature /*feature*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onAuthenticatorIdRetrieved(int64_t /*authenticatorId*/) override {
        auto lock = std::lock_guard{mMutex};
        mOnAuthenticatorIdRetrievedInvoked = true;
        mCv.notify_one();
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onAuthenticatorIdInvalidated(int64_t /*newAuthenticatorId*/) override {
        auto lock = std::lock_guard{mMutex};
        mOnAuthenticatorIdInvalidatedInvoked = true;
        mCv.notify_one();
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onSessionClosed() override {
        auto lock = std::lock_guard<std::mutex>{mMutex};
        mOnSessionClosedInvoked = true;
        mCv.notify_one();
        return ndk::ScopedAStatus::ok();
    }

    std::mutex mMutex;
    std::condition_variable mCv;
    Error mError = Error::UNKNOWN;
    int32_t mVendorCode = 0;
    int64_t mGeneratedChallenge = 0;
    int64_t mRevokedChallenge = 0;
    bool mOnChallengeGeneratedInvoked = false;
    bool mOnChallengeRevokedInvoked = false;
    bool mOnErrorInvoked = false;
    bool mOnEnrollmentsEnumeratedInvoked = false;
    bool mOnEnrollmentsRemovedInvoked = false;
    bool mOnFeaturesRetrievedInvoked = false;
    bool mOnAuthenticatorIdRetrievedInvoked = false;
    bool mOnAuthenticatorIdInvalidatedInvoked = false;
    bool mOnSessionClosedInvoked = false;
};

class Face : public testing::TestWithParam<std::string> {
  protected:
    void SetUp() override {
        // Prepare the callback.
        mCb = ndk::SharedRefBase::make<SessionCallback>();

        int retries = 0;
        bool isOk = false;
        // If the first attempt to create a session fails, we try to create a session again. The
        // first attempt might fail if the framework already has an active session. The AIDL
        // contract doesn't allow to create a new session without closing the old one. However, we
        // can't close the framework's session from VTS. The expectation here is that the HAL will
        // crash after the first illegal attempt to create a session, then it will restart, and then
        // we'll be able to create a session.
        do {
            // Get an instance of the HAL.
            AIBinder* binder = AServiceManager_waitForService(GetParam().c_str());
            ASSERT_NE(binder, nullptr);
            mHal = IFace::fromBinder(ndk::SpAIBinder(binder));

            // Create a session.
            isOk = mHal->createSession(kSensorId, kUserId, mCb, &mSession).isOk();
            ++retries;
        } while (!isOk && retries < 2);

        ASSERT_TRUE(isOk);
    }

    void TearDown() override {
        // Close the mSession.
        ASSERT_TRUE(mSession->close().isOk());

        // Make sure the mSession is closed.
        auto lock = std::unique_lock<std::mutex>(mCb->mMutex);
        mCb->mCv.wait(lock, [this] { return mCb->mOnSessionClosedInvoked; });
    }

    std::shared_ptr<IFace> mHal;
    std::shared_ptr<SessionCallback> mCb;
    std::shared_ptr<ISession> mSession;
};

TEST_P(Face, GetSensorPropsWorksTest) {
    std::vector<SensorProps> sensorProps;

    // Call the method.
    ASSERT_TRUE(mHal->getSensorProps(&sensorProps).isOk());

    // Make sure the sensorProps aren't empty.
    ASSERT_FALSE(sensorProps.empty());
    ASSERT_FALSE(sensorProps[0].commonProps.componentInfo.empty());
}

TEST_P(Face, EnrollWithBadHatResultsInErrorTest) {
    // Call the method.
    auto hat = keymaster::HardwareAuthToken{};
    std::shared_ptr<common::ICancellationSignal> cancellationSignal;
    ASSERT_TRUE(
            mSession->enroll(hat, EnrollmentType::DEFAULT, {}, std::nullopt, &cancellationSignal)
                    .isOk());

    // Make sure an error is returned.
    auto lock = std::unique_lock{mCb->mMutex};
    mCb->mCv.wait(lock, [this] { return mCb->mOnErrorInvoked; });
    EXPECT_EQ(mCb->mError, Error::UNABLE_TO_PROCESS);
    EXPECT_EQ(mCb->mVendorCode, 0);
}

TEST_P(Face, GenerateChallengeProducesUniqueChallengesTest) {
    static constexpr int kIterations = 100;

    auto challenges = std::set<int>{};
    for (unsigned int i = 0; i < kIterations; ++i) {
        // Call the method.
        ASSERT_TRUE(mSession->generateChallenge().isOk());

        // Check that the generated challenge is unique and not 0.
        auto lock = std::unique_lock{mCb->mMutex};
        mCb->mCv.wait(lock, [this] { return mCb->mOnChallengeGeneratedInvoked; });
        ASSERT_NE(mCb->mGeneratedChallenge, 0);
        ASSERT_EQ(challenges.find(mCb->mGeneratedChallenge), challenges.end());

        challenges.insert(mCb->mGeneratedChallenge);
        mCb->mOnChallengeGeneratedInvoked = false;
    }
}

TEST_P(Face, RevokeChallengeWorksForNonexistentChallengeTest) {
    const int64_t nonexistentChallenge = 123;

    // Call the method.
    ASSERT_TRUE(mSession->revokeChallenge(nonexistentChallenge).isOk());

    // Check that the challenge is revoked and matches the requested challenge.
    auto lock = std::unique_lock{mCb->mMutex};
    mCb->mCv.wait(lock, [this] { return mCb->mOnChallengeRevokedInvoked; });
    ASSERT_EQ(mCb->mRevokedChallenge, nonexistentChallenge);
}

TEST_P(Face, RevokeChallengeWorksForExistentChallengeTest) {
    // Generate a challenge.
    ASSERT_TRUE(mSession->generateChallenge().isOk());

    // Wait for the result.
    auto lock = std::unique_lock{mCb->mMutex};
    mCb->mCv.wait(lock, [this] { return mCb->mOnChallengeGeneratedInvoked; });
    lock.unlock();

    // Revoke the challenge.
    ASSERT_TRUE(mSession->revokeChallenge(mCb->mGeneratedChallenge).isOk());

    // Check that the challenge is revoked and matches the requested challenge.
    lock.lock();
    mCb->mCv.wait(lock, [this] { return mCb->mOnChallengeRevokedInvoked; });
    ASSERT_EQ(mCb->mRevokedChallenge, mCb->mGeneratedChallenge);
}

TEST_P(Face, EnumerateEnrollmentsWorksTest) {
    // Call the method.
    ASSERT_TRUE(mSession->enumerateEnrollments().isOk());

    // Wait for the result.
    auto lock = std::unique_lock{mCb->mMutex};
    mCb->mCv.wait(lock, [this] { return mCb->mOnEnrollmentsEnumeratedInvoked; });
}

TEST_P(Face, RemoveEnrollmentsWorksTest) {
    // Call the method.
    ASSERT_TRUE(mSession->removeEnrollments({}).isOk());

    // Wait for the result.
    auto lock = std::unique_lock{mCb->mMutex};
    mCb->mCv.wait(lock, [this] { return mCb->mOnEnrollmentsRemovedInvoked; });
}

TEST_P(Face, GetFeaturesWithoutEnrollmentsResultsInUnableToProcess) {
    // Call the method.
    ASSERT_TRUE(mSession->getFeatures().isOk());

    // Wait for the result.
    auto lock = std::unique_lock{mCb->mMutex};
    mCb->mCv.wait(lock, [this] { return mCb->mOnErrorInvoked; });
    EXPECT_EQ(mCb->mError, Error::UNABLE_TO_PROCESS);
    EXPECT_EQ(mCb->mVendorCode, 0);
}

TEST_P(Face, GetAuthenticatorIdWorksTest) {
    // Call the method.
    ASSERT_TRUE(mSession->getAuthenticatorId().isOk());

    // Wait for the result.
    auto lock = std::unique_lock{mCb->mMutex};
    mCb->mCv.wait(lock, [this] { return mCb->mOnAuthenticatorIdRetrievedInvoked; });
}

TEST_P(Face, InvalidateAuthenticatorIdWorksTest) {
    // Call the method.
    ASSERT_TRUE(mSession->invalidateAuthenticatorId().isOk());

    // Wait for the result.
    auto lock = std::unique_lock{mCb->mMutex};
    mCb->mCv.wait(lock, [this] { return mCb->mOnAuthenticatorIdInvalidatedInvoked; });
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(Face);
INSTANTIATE_TEST_SUITE_P(IFace, Face,
                         testing::ValuesIn(::android::getAidlHalInstanceNames(IFace::descriptor)),
                         ::android::PrintInstanceNameToString);

}  // namespace
}  // namespace aidl::android::hardware::biometrics::face

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
