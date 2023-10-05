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

#include <android/binder_process.h>
#include <fingerprint.sysprop.h>
#include <gtest/gtest.h>

#include <android-base/logging.h>

#include <aidl/android/hardware/biometrics/fingerprint/BnSessionCallback.h>

#include "Session.h"
#include "thread/WorkerThread.h"
#include "util/Util.h"

using namespace ::android::fingerprint::virt;
using namespace ::aidl::android::hardware::biometrics::fingerprint;

namespace aidl::android::hardware::biometrics::fingerprint {

class TestSessionCallback : public BnSessionCallback {
  public:
    ndk::ScopedAStatus onChallengeGenerated(int64_t /*challenge*/) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onChallengeRevoked(int64_t /*challenge*/) override {
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onError(fingerprint::Error /*error*/, int32_t /*vendorCode*/) override {
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
    ndk::ScopedAStatus onAcquired(AcquiredInfo /*info*/, int32_t /*vendorCode*/) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onEnrollmentsEnumerated(
            const std::vector<int32_t>& /*enrollmentIds*/) override {
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
    ::ndk::ScopedAStatus onLockoutPermanent() override { return ndk::ScopedAStatus::ok(); };
    ndk::ScopedAStatus onLockoutTimed(int64_t /* timeout */) override {
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus onLockoutCleared() override { return ndk::ScopedAStatus::ok(); }
    ndk::ScopedAStatus onSessionClosed() override {
        mIsClosed = true;
        return ndk::ScopedAStatus::ok();
    }

    bool mIsClosed = false;
};

class SessionTest : public ::testing::Test {
  public:
    SessionTest() : mWorker(2) {}

  protected:
    void SetUp() override {
        mCb = ndk::SharedRefBase::make<TestSessionCallback>();
        mSession = ndk::SharedRefBase::make<Session>(1, 2, mCb, &mFakeFingerprintEngine, &mWorker);
        ASSERT_TRUE(mSession != nullptr);
        mSession->linkToDeath(mCb->asBinder().get());
    }

    void TearDown() override {}

    std::shared_ptr<Session> mSession;
    std::shared_ptr<TestSessionCallback> mCb;

  private:
    FakeFingerprintEngine mFakeFingerprintEngine;
    WorkerThread mWorker;
};

TEST_F(SessionTest, close) {
    ASSERT_TRUE(!mSession->isClosed());
    ASSERT_TRUE(!mCb->mIsClosed);
    onClientDeath(nullptr);
    ASSERT_TRUE(!mSession->isClosed());
    ASSERT_TRUE(!mCb->mIsClosed);
    onClientDeath(static_cast<void*>(mSession.get()));
    ASSERT_TRUE(mSession->isClosed());
    ASSERT_TRUE(mCb->mIsClosed);
}

}  //  namespace aidl::android::hardware::biometrics::fingerprint

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
