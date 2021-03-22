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

#include <future>

namespace aidl::android::hardware::biometrics::fingerprint {
namespace {

constexpr int kSensorId = 0;
constexpr int kUserId = 0;
constexpr auto kCallbackTimeout = std::chrono::seconds(1);

enum class MethodName {
    kOnStateChanged,
};

struct Invocation {
    MethodName methodName;
    int32_t cookie;
    SessionState state;
};

class SessionCallback : public BnSessionCallback {
  public:
    explicit SessionCallback() : mIsPromiseValid(false) {}

    void setPromise(std::promise<std::vector<Invocation>>&& promise) {
        mPromise = std::move(promise);
        mIsPromiseValid = true;
    }

    ndk::ScopedAStatus onStateChanged(int32_t cookie, SessionState state) override {
        Invocation invocation = {};
        invocation.methodName = MethodName::kOnStateChanged;
        invocation.cookie = cookie;
        invocation.state = state;
        mInvocations.push_back(invocation);
        if (state == SessionState::IDLING) {
            assert(mIsPromiseValid);
            mPromise.set_value(mInvocations);
        }
        return ndk::ScopedAStatus::ok();
    }

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

    ndk::ScopedAStatus onSessionClosed() override { return ndk::ScopedAStatus::ok(); }

  private:
    bool mIsPromiseValid;
    std::vector<Invocation> mInvocations;
    std::promise<std::vector<Invocation>> mPromise;
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
    // Prepare the callback
    std::promise<std::vector<Invocation>> promise;
    auto future = promise.get_future();
    std::shared_ptr<SessionCallback> cb = ndk::SharedRefBase::make<SessionCallback>();
    cb->setPromise(std::move(promise));

    // Create a session
    std::shared_ptr<ISession> session;
    ASSERT_TRUE(mHal->createSession(kSensorId, kUserId, cb, &session).isOk());

    // Call authenticate
    int32_t cookie = 123;
    std::shared_ptr<common::ICancellationSignal> cancellationSignal;
    ASSERT_TRUE(session->authenticate(cookie, 0, &cancellationSignal).isOk());

    // Get the results
    ASSERT_TRUE(future.wait_for(kCallbackTimeout) == std::future_status::ready);
    std::vector<Invocation> invocations = future.get();

    // Close the session
    ASSERT_TRUE(session->close(0).isOk());

    ASSERT_FALSE(invocations.empty());
    EXPECT_EQ(invocations.front().methodName, MethodName::kOnStateChanged);
    EXPECT_EQ(invocations.front().state, SessionState::AUTHENTICATING);
    EXPECT_EQ(invocations.back().methodName, MethodName::kOnStateChanged);
    EXPECT_EQ(invocations.back().state, SessionState::IDLING);
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
