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
#include <aidl/android/hardware/biometrics/fingerprint/BnFingerprint.h>
#include <aidl/android/hardware/biometrics/fingerprint/BnSessionCallback.h>

#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <chrono>
#include <future>

namespace aidl::android::hardware::biometrics::fingerprint {
namespace {

using namespace std::literals::chrono_literals;

constexpr int kSensorId = 0;
constexpr int kUserId = 0;

class SessionCallback : public BnSessionCallback {
  public:
    explicit SessionCallback(std::promise<void>&& promise) : mPromise(std::move(promise)) {}

    ndk::ScopedAStatus onChallengeGenerated(int64_t /*challenge*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onChallengeRevoked(int64_t /*challenge*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onAcquired(AcquiredInfo /*info*/, int32_t /*vendorCode*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onError(Error /*error*/, int32_t /*vendorCode*/) override {
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

    ndk::ScopedAStatus onAuthenticatorIdRetrieved(int64_t /*authenticatorId*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onAuthenticatorIdInvalidated(int64_t /*newAuthenticatorId*/) override {
        return ndk::ScopedAStatus::ok();
    }

    ndk::ScopedAStatus onSessionClosed() override {
        mPromise.set_value();
        return ndk::ScopedAStatus::ok();
    }

  private:
    std::promise<void> mPromise;
};

class Fingerprint : public testing::TestWithParam<std::string> {
  protected:
    void SetUp() override {
        AIBinder* binder = AServiceManager_waitForService(GetParam().c_str());
        ASSERT_NE(binder, nullptr);
        mHal = IFingerprint::fromBinder(ndk::SpAIBinder(binder));
    }

    std::shared_ptr<IFingerprint> mHal;
};

TEST_P(Fingerprint, AuthenticateTest) {
    auto promise = std::promise<void>{};
    auto future = promise.get_future();
    // Prepare the callback.
    auto cb = ndk::SharedRefBase::make<SessionCallback>(std::move(promise));

    // Create a session
    std::shared_ptr<ISession> session;
    ASSERT_TRUE(mHal->createSession(kSensorId, kUserId, cb, &session).isOk());

    // Call authenticate
    std::shared_ptr<common::ICancellationSignal> cancellationSignal;
    ASSERT_TRUE(session->authenticate(-1 /* operationId */, &cancellationSignal).isOk());

    // Get the results
    // TODO(b/166799066): test authenticate.

    // Close the session
    ASSERT_TRUE(session->close().isOk());
    auto status = future.wait_for(1s);
    ASSERT_EQ(status, std::future_status::ready);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(Fingerprint);
INSTANTIATE_TEST_SUITE_P(
        IFingerprint, Fingerprint,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IFingerprint::descriptor)),
        ::android::PrintInstanceNameToString);

}  // namespace
}  // namespace aidl::android::hardware::biometrics::fingerprint

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
