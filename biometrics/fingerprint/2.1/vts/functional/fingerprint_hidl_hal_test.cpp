/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "fingerprint_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprint.h>
#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprintClientCallback.h>
#include <chrono>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>

using android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint;
using android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprintClientCallback;
using android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo;
using android::hardware::biometrics::fingerprint::V2_1::FingerprintError;
using android::hardware::biometrics::fingerprint::V2_1::RequestStatus;
using android::hardware::hidl_vec;
using android::hardware::Return;

#define SERVICE_NAME "fingerprint_hal"

class FingerprintHidlTest : public ::testing::Test,
    public IBiometricsFingerprintClientCallback {

protected:
    android::sp<IBiometricsFingerprint> service;
    FingerprintError err;
    // State changes should occur within this threshold, otherwise the
    // framework' will assume things have broken.
    std::chrono::seconds threshold;

public:
    FingerprintHidlTest ():
        err(FingerprintError::ERROR_NO_ERROR), threshold(1) {}

    virtual void SetUp() override {
        service = IBiometricsFingerprint::getService(SERVICE_NAME);

        ASSERT_NE(service, nullptr);
        clearErr();

        // TODO: instantly fail any test that receives a death notification
    }

    virtual void TearDown() override {}

    // implement methods of IBiometricsFingerprintClientCallback
    virtual Return<void> onEnrollResult(uint64_t, uint32_t, uint32_t, uint32_t)
            override {
        return Return<void>();
    }
    virtual Return<void> onAcquired(uint64_t, FingerprintAcquiredInfo, int32_t)
            override {
        return Return<void>();
    }

    virtual Return<void> onAuthenticated(uint64_t, uint32_t, uint32_t, const
            hidl_vec<uint8_t>&) override {
        return Return<void>();
    }

    virtual Return<void> onError(uint64_t, FingerprintError error, int32_t)
            override {
        err = error;
        return Return<void>();
    }

    virtual Return<void> onRemoved(uint64_t, uint32_t, uint32_t, uint32_t)
            override {
        return Return<void>();
    }

    virtual Return<void> onEnumerate(uint64_t, uint32_t, uint32_t, uint32_t)
            override {
        return Return<void>();
    }

    void clearErr () {
        err = FingerprintError::ERROR_NO_ERROR;
    }
};

class FingerprintHidlEnvironment : public ::testing::Environment {
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

// The service should be reachable.
TEST_F(FingerprintHidlTest, ConnectTest) {
    Return<uint64_t> rc = service->setNotify(this);
    EXPECT_NE(rc, 0UL);
}

// Cancel should always return ERROR_CANCELED from any starting state including
// the IDLE state.
TEST_F(FingerprintHidlTest, CancelTest) {
    Return<uint64_t> rc = service->setNotify(this);
    EXPECT_NE(rc, 0UL);

    auto start = std::chrono::system_clock::now();
    Return<RequestStatus> res = service->cancel();
    auto end = std::chrono::system_clock::now();
    auto diff = end - start;

    // check that we were able to make an IPC request successfully
    EXPECT_EQ(RequestStatus::SYS_OK, res);
    // check error should be ERROR_CANCELED
    EXPECT_EQ(FingerprintError::ERROR_CANCELED, err);
    // check that this did not take longer than a threshold
    EXPECT_TRUE(diff <= threshold);
}

// A call to cancel should after any other method call should set the error
// state to canceled.
TEST_F(FingerprintHidlTest, AuthTest) {
    Return<uint64_t> rc = service->setNotify(this);
    EXPECT_NE(rc, 0UL);

    Return<RequestStatus> res = service->authenticate(0, 0);
    // check that we were able to make an IPC request successfully
    EXPECT_EQ(RequestStatus::SYS_OK, res);

    auto start = std::chrono::system_clock::now();
    res = service->cancel();
    auto end = std::chrono::system_clock::now();
    auto diff = end - start;

    // check that we were able to make an IPC request successfully
    EXPECT_EQ(RequestStatus::SYS_OK, res);
    // check error should be ERROR_CANCELED
    EXPECT_EQ(FingerprintError::ERROR_CANCELED, err);
    // check that this did not take longer than a threshold
    EXPECT_TRUE(diff <= threshold);
}

int main(int argc, char **argv) {
    ::testing::AddGlobalTestEnvironment(new FingerprintHidlEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
