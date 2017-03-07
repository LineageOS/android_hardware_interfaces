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
#define SERVICE_NAME "fingerprint_hal"

#include <android-base/logging.h>
#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprint.h>
#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprintClientCallback.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <VtsHalHidlTargetBaseTest.h>

using android::Condition;
using android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint;
using android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprintClientCallback;
using android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo;
using android::hardware::biometrics::fingerprint::V2_1::FingerprintError;
using android::hardware::biometrics::fingerprint::V2_1::RequestStatus;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::Mutex;
using android::sp;

class FingerprintHidlTest : public ::testing::VtsHalHidlTargetBaseTest {

protected:
    class MyCallback : public IBiometricsFingerprintClientCallback {

        // implement methods of IBiometricsFingerprintClientCallback
        virtual Return<void> onEnrollResult(uint64_t, uint32_t, uint32_t,
                uint32_t) override {
            callBackCalled();
            return Return<void>();
        }

        virtual Return<void> onAcquired(uint64_t, FingerprintAcquiredInfo,
                int32_t) override {
            callBackCalled();
            return Return<void>();
        }

        virtual Return<void> onAuthenticated(uint64_t, uint32_t, uint32_t,
                const hidl_vec<uint8_t>&) override {
            callBackCalled();
            return Return<void>();
        }

        virtual Return<void> onError(uint64_t, FingerprintError error, int32_t)
                override {
            mTestCase->mErr = error;
            callBackCalled();
            return Return<void>();
        }

        virtual Return<void> onRemoved(uint64_t, uint32_t, uint32_t, uint32_t)
                override {
            callBackCalled();
            return Return<void>();
        }

        virtual Return<void> onEnumerate(uint64_t, uint32_t, uint32_t,
                uint32_t) override {
            callBackCalled();
            return Return<void>();
        }

        void callBackCalled () {
            mTestCase->mCallbackCalled = true;
            mTestCase->mCallbackCond.broadcast();
        }

        FingerprintHidlTest* mTestCase;
    public:
        MyCallback(FingerprintHidlTest* aTest) : mTestCase(aTest) {}
    };

    sp<MyCallback> mCallback;
    bool mCallbackCalled;
    Condition mCallbackCond;
    FingerprintError mErr;
    Mutex mLock;
    sp<IBiometricsFingerprint> mService;
    static const unsigned int kThresholdInSeconds = 3;

    void clearErr () {
        mErr = FingerprintError::ERROR_NO_ERROR;
    }

    // Timed callback mechanism.  Will block up to kThresholdInSeconds,
    // returning true if callback was invoked in that time frame.
    bool waitForCallback(const unsigned int seconds = kThresholdInSeconds) {
        nsecs_t reltime = seconds_to_nanoseconds(seconds);
        Mutex::Autolock _l(mLock);
        nsecs_t endTime = systemTime() + reltime;
        while (!mCallbackCalled) {
            if (reltime == 0) {
                mCallbackCond.wait(mLock);
            } else {
                nsecs_t now = systemTime();
                if (now > endTime) {
                    return false;
                }
                mCallbackCond.waitRelative(mLock, endTime - now);
            }
        }
        return true;
    }
public:
    FingerprintHidlTest (): mCallbackCalled(false) {}

    virtual void SetUp() override {
        mService = ::testing::VtsHalHidlTargetBaseTest::getService<IBiometricsFingerprint>(SERVICE_NAME);

        ASSERT_NE(mService, nullptr);
        clearErr();

        mCallback = new MyCallback(this);
        // TODO: instantly fail any test that receives a death notification
    }

    virtual void TearDown() override {}
};

class FingerprintHidlEnvironment : public ::testing::Environment {
public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

// The service should be reachable.
TEST_F(FingerprintHidlTest, ConnectTest) {
    Return<uint64_t> rc = mService->setNotify(mCallback);
    EXPECT_NE(rc, 0UL);
}

// Cancel should always return ERROR_CANCELED from any starting state including
// the IDLE state.
TEST_F(FingerprintHidlTest, CancelTest) {
    Return<uint64_t> rc = mService->setNotify(mCallback);
    EXPECT_NE(rc, 0UL);

    Return<RequestStatus> res = mService->cancel();
    // make sure callback was invoked within kThresholdInSeconds
    EXPECT_EQ(true, waitForCallback());
    // check that we were able to make an IPC request successfully
    EXPECT_EQ(RequestStatus::SYS_OK, res);
    // check error should be ERROR_CANCELED
    EXPECT_EQ(FingerprintError::ERROR_CANCELED, mErr);
}

// A call to cancel should after any other method call should set the error
// state to canceled.
TEST_F(FingerprintHidlTest, AuthTest) {
    Return<uint64_t> rc = mService->setNotify(mCallback);
    EXPECT_NE(rc, 0UL);

    Return<RequestStatus> res = mService->authenticate(0, 0);
    // check that we were able to make an IPC request successfully
    EXPECT_EQ(RequestStatus::SYS_OK, res);

    res = mService->cancel();
    // make sure callback was invoked within kThresholdInSeconds
    EXPECT_EQ(true, waitForCallback());
    // check that we were able to make an IPC request successfully
    EXPECT_EQ(RequestStatus::SYS_OK, res);
    // check error should be ERROR_CANCELED
    EXPECT_EQ(FingerprintError::ERROR_CANCELED, mErr);
}

int main(int argc, char **argv) {
    ::testing::AddGlobalTestEnvironment(new FingerprintHidlEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
