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

#include <aidl/android/hardware/biometrics/fingerprint/BnSessionCallback.h>
#include <android/binder_process.h>
#include <fingerprint.sysprop.h>
#include <gtest/gtest.h>

#include <android-base/logging.h>

#include "FakeFingerprintEngine.h"
#include "FakeFingerprintEngineUdfps.h"

using namespace ::android::fingerprint::virt;
using namespace ::aidl::android::hardware::biometrics::fingerprint;
using namespace ::aidl::android::hardware::keymaster;

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
        mEnrollmentProgress++;
        return ndk::ScopedAStatus::ok();
    };

    ::ndk::ScopedAStatus onAuthenticationSucceeded(int32_t /*enrollmentId*/,
                                                   const keymaster::HardwareAuthToken&) override {
        mAuthenticationSuccess++;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onAuthenticationFailed() override {
        mAuthenticationFailure++;
        return ndk::ScopedAStatus::ok();
    };
    ::ndk::ScopedAStatus onInteractionDetected() override {
        mDetectInteraction++;
        return ndk::ScopedAStatus::ok();
    };
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
    ndk::ScopedAStatus onSessionClosed() override { return ndk::ScopedAStatus::ok(); }

    int32_t getAuthenticationCount() { return mAuthenticationSuccess + mAuthenticationFailure; }
    int32_t getDetectInteractionCount() { return mDetectInteraction; }

    int32_t mAuthenticationSuccess = 0;
    int32_t mAuthenticationFailure = 0;
    int32_t mEnrollmentProgress = 0;
    int32_t mDetectInteraction = 0;
};

class FakeFingerprintEngineUdfpsTest : public ::testing::Test {
  protected:
    void SetUp() override {}

    void TearDown() override {
        // reset to default
        FingerprintHalProperties::sensor_location("");
    }

    FakeFingerprintEngineUdfps mEngine;
};

bool isDefaultLocation(SensorLocation& sc) {
    return (sc.sensorLocationX == FakeFingerprintEngineUdfps::defaultSensorLocationX &&
            sc.sensorLocationY == FakeFingerprintEngineUdfps::defaultSensorLocationY &&
            sc.sensorRadius == FakeFingerprintEngineUdfps::defaultSensorRadius && sc.display == "");
}

TEST_F(FakeFingerprintEngineUdfpsTest, getSensorLocationOk) {
    auto loc = "100:200:30";
    FingerprintHalProperties::sensor_location(loc);
    SensorLocation sc = mEngine.getSensorLocation();
    ASSERT_TRUE(sc.sensorLocationX == 100);
    ASSERT_TRUE(sc.sensorLocationY == 200);
    ASSERT_TRUE(sc.sensorRadius == 30);

    loc = "100:200:30:screen1";
    FingerprintHalProperties::sensor_location(loc);
    sc = mEngine.getSensorLocation();
    ASSERT_TRUE(sc.sensorLocationX == 100);
    ASSERT_TRUE(sc.sensorLocationY == 200);
    ASSERT_TRUE(sc.sensorRadius == 30);
    ASSERT_TRUE(sc.display == "screen1");
}

TEST_F(FakeFingerprintEngineUdfpsTest, getSensorLocationBad) {
    const std::vector<std::string> badStr{"", "100", "10:20", "10,20,5", "a:b:c"};
    SensorLocation sc;
    for (const auto& s : badStr) {
        FingerprintHalProperties::sensor_location(s);
        sc = mEngine.getSensorLocation();
        ASSERT_TRUE(isDefaultLocation(sc));
    }
}

TEST_F(FakeFingerprintEngineUdfpsTest, initialization) {
    ASSERT_TRUE(mEngine.getWorkMode() == FakeFingerprintEngineUdfps::WorkMode::kIdle);
}

TEST_F(FakeFingerprintEngineUdfpsTest, authenticate) {
    std::shared_ptr<TestSessionCallback> cb = ndk::SharedRefBase::make<TestSessionCallback>();
    std::promise<void> cancel;
    mEngine.authenticateImpl(cb.get(), 1, cancel.get_future());
    ASSERT_TRUE(mEngine.getWorkMode() == FakeFingerprintEngineUdfps::WorkMode::kAuthenticate);
    mEngine.onPointerDownImpl(1, 2, 3, 4.0, 5.0);
    ASSERT_EQ(cb->getAuthenticationCount(), 0);
    mEngine.onUiReadyImpl();
    ASSERT_EQ(cb->getAuthenticationCount(), 1);
}

TEST_F(FakeFingerprintEngineUdfpsTest, enroll) {
    std::shared_ptr<TestSessionCallback> cb = ndk::SharedRefBase::make<TestSessionCallback>();
    std::promise<void> cancel;
    keymaster::HardwareAuthToken hat{.mac = {5, 6}};
    FingerprintHalProperties::next_enrollment("5:0,0:true");
    mEngine.enrollImpl(cb.get(), hat, cancel.get_future());
    ASSERT_TRUE(mEngine.getWorkMode() == FakeFingerprintEngineUdfps::WorkMode::kEnroll);
    mEngine.onPointerDownImpl(1, 2, 3, 4.0, 5.0);
    ASSERT_EQ(cb->mEnrollmentProgress, 0);
    mEngine.onUiReadyImpl();
    ASSERT_TRUE(cb->mEnrollmentProgress > 0);
}

TEST_F(FakeFingerprintEngineUdfpsTest, detectInteraction) {
    FingerprintHalProperties::detect_interaction(true);
    FingerprintHalProperties::enrollments({1, 2});
    FingerprintHalProperties::enrollment_hit(2);
    FingerprintHalProperties::operation_detect_interaction_acquired("");
    std::shared_ptr<TestSessionCallback> cb = ndk::SharedRefBase::make<TestSessionCallback>();
    std::promise<void> cancel;
    mEngine.detectInteractionImpl(cb.get(), cancel.get_future());
    ASSERT_TRUE(mEngine.getWorkMode() == FakeFingerprintEngineUdfps::WorkMode::kDetectInteract);
    mEngine.onPointerDownImpl(1, 2, 3, 4.0, 5.0);
    ASSERT_EQ(cb->getDetectInteractionCount(), 0);
    mEngine.onUiReadyImpl();
    ASSERT_EQ(cb->getDetectInteractionCount(), 1);
}
// More
}  // namespace aidl::android::hardware::biometrics::fingerprint
