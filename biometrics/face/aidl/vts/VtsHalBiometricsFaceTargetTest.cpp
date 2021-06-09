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

constexpr int kSensorId = 0;
constexpr int kUserId = 0;

class SessionCallback : public BnSessionCallback {
  public:
    ndk::ScopedAStatus onChallengeGenerated(int64_t /*challenge*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onChallengeRevoked(int64_t /*challenge*/) override {
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
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onEnrollmentsRemoved(
            const std::vector<int32_t>& /*enrollmentIds*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onFeaturesRetrieved(const std::vector<Feature>& /*features*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onFeatureSet(Feature /*feature*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onAuthenticatorIdRetrieved(int64_t /*authenticatorId*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onAuthenticatorIdInvalidated(int64_t /*newAuthenticatorId*/) override {
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
    bool mOnErrorInvoked = false;
    bool mOnSessionClosedInvoked = false;
};

class Face : public testing::TestWithParam<std::string> {
  protected:
    void SetUp() override {
        AIBinder* binder = AServiceManager_waitForService(GetParam().c_str());
        ASSERT_NE(binder, nullptr);
        mHal = IFace::fromBinder(ndk::SpAIBinder(binder));
    }

    std::shared_ptr<IFace> mHal;
};

TEST_P(Face, AuthenticateTest) {
    // Prepare the callback.
    auto cb = ndk::SharedRefBase::make<SessionCallback>();

    // Create a session
    std::shared_ptr<ISession> session;
    ASSERT_TRUE(mHal->createSession(kSensorId, kUserId, cb, &session).isOk());

    // Call authenticate
    std::shared_ptr<common::ICancellationSignal> cancellationSignal;
    ASSERT_TRUE(session->authenticate(0 /* operationId */, &cancellationSignal).isOk());

    auto lock = std::unique_lock<std::mutex>(cb->mMutex);
    cb->mCv.wait(lock, [&cb] { return cb->mOnErrorInvoked; });
    // Get the results
    EXPECT_EQ(cb->mError, Error::UNABLE_TO_PROCESS);
    EXPECT_EQ(cb->mVendorCode, 0);
    lock.unlock();

    // Close the session
    ASSERT_TRUE(session->close().isOk());

    lock.lock();
    cb->mCv.wait(lock, [&cb] { return cb->mOnSessionClosedInvoked; });
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
