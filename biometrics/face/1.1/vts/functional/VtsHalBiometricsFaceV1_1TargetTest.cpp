/*
 * Copyright 2020 The Android Open Source Project
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

#define LOG_TAG "biometrics_face_hidl_hal_test"

#include <android/hardware/biometrics/face/1.0/IBiometricsFaceClientCallback.h>
#include <android/hardware/biometrics/face/1.1/IBiometricsFace.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <android-base/logging.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <chrono>
#include <cstdint>
#include <random>

using android::sp;
using android::hardware::hidl_handle;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::biometrics::face::V1_0::FaceAcquiredInfo;
using android::hardware::biometrics::face::V1_0::FaceError;
using android::hardware::biometrics::face::V1_0::IBiometricsFaceClientCallback;
using android::hardware::biometrics::face::V1_0::OptionalUint64;
using android::hardware::biometrics::face::V1_0::Status;
using android::hardware::biometrics::face::V1_1::IBiometricsFace;

namespace {

// Arbitrary, nonexistent userId
constexpr uint32_t kUserId = 9;
constexpr uint32_t kTimeoutSec = 3;
constexpr auto kTimeout = std::chrono::seconds(kTimeoutSec);
constexpr char kFacedataDir[] = "/data/vendor_de/0/facedata";
constexpr char kCallbackNameOnError[] = "onError";

// Callback arguments that need to be captured for the tests.
struct FaceCallbackArgs {
    // The error passed to the last onError() callback.
    FaceError error;

    // The userId passed to the last callback.
    int32_t userId;
};

// Test callback class for the BiometricsFace HAL.
// The HAL will call these callback methods to notify about completed operations
// or encountered errors.
class FaceCallback : public ::testing::VtsHalHidlTargetCallbackBase<FaceCallbackArgs>,
                     public IBiometricsFaceClientCallback {
  public:
    Return<void> onEnrollResult(uint64_t, uint32_t, int32_t, uint32_t) override { return Void(); }

    Return<void> onAuthenticated(uint64_t, uint32_t, int32_t, const hidl_vec<uint8_t>&) override {
        return Void();
    }

    Return<void> onAcquired(uint64_t, int32_t, FaceAcquiredInfo, int32_t) override {
        return Void();
    }

    Return<void> onError(uint64_t, int32_t userId, FaceError error, int32_t) override {
        FaceCallbackArgs args = {};
        args.error = error;
        args.userId = userId;
        NotifyFromCallback(kCallbackNameOnError, args);
        return Void();
    }

    Return<void> onRemoved(uint64_t, const hidl_vec<uint32_t>&, int32_t) override { return Void(); }

    Return<void> onEnumerate(uint64_t, const hidl_vec<uint32_t>&, int32_t) override {
        return Void();
    }

    Return<void> onLockoutChanged(uint64_t) override { return Void(); }
};

// Test class for the BiometricsFace HAL.
class FaceHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        mService = IBiometricsFace::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        mCallback = new FaceCallback();
        mCallback->SetWaitTimeoutDefault(kTimeout);
        Return<void> ret1 = mService->setCallback(mCallback, [](const OptionalUint64& res) {
            ASSERT_EQ(Status::OK, res.status);
            // Makes sure the "deviceId" represented by "res.value" is not 0.
            // 0 would mean the HIDL is not available.
            ASSERT_NE(0UL, res.value);
        });
        ASSERT_TRUE(ret1.isOk());
        Return<Status> ret2 = mService->setActiveUser(kUserId, kFacedataDir);
        ASSERT_EQ(Status::OK, static_cast<Status>(ret2));
    }

    void TearDown() override {}

    sp<IBiometricsFace> mService;
    sp<FaceCallback> mCallback;
};

// enroll with an invalid (all zeroes) HAT should fail.
TEST_P(FaceHidlTest, Enroll2_2ZeroHatTest) {
    // Filling HAT with zeros
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; i++) {
        token[i] = 0;
    }

    hidl_handle windowId = nullptr;
    Return<Status> ret = mService->enroll_1_1(token, kTimeoutSec, {}, windowId);
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));

    // onError should be called with a meaningful (nonzero) error.
    auto res = mCallback->WaitForCallback(kCallbackNameOnError);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kUserId, res.args->userId);
    EXPECT_EQ(FaceError::UNABLE_TO_PROCESS, res.args->error);
}

// enroll with an invalid HAT should fail.
TEST_P(FaceHidlTest, Enroll2_2GarbageHatTest) {
    // Filling HAT with pseudorandom invalid data.
    // Using default seed to make the test reproducible.
    std::mt19937 gen(std::mt19937::default_seed);
    std::uniform_int_distribution<uint8_t> dist;
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; ++i) {
        token[i] = dist(gen);
    }

    hidl_handle windowId = nullptr;
    Return<Status> ret = mService->enroll_1_1(token, kTimeoutSec, {}, windowId);
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));

    // onError should be called with a meaningful (nonzero) error.
    auto res = mCallback->WaitForCallback(kCallbackNameOnError);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kUserId, res.args->userId);
    EXPECT_EQ(FaceError::UNABLE_TO_PROCESS, res.args->error);
}

// enroll with an invalid (all zeroes) HAT should fail.
TEST_P(FaceHidlTest, EnrollRemotelyZeroHatTest) {
    // Filling HAT with zeros
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; i++) {
        token[i] = 0;
    }

    Return<Status> ret = mService->enrollRemotely(token, kTimeoutSec, {});
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));

    // onError should be called with a meaningful (nonzero) error.
    auto res = mCallback->WaitForCallback(kCallbackNameOnError);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kUserId, res.args->userId);
    EXPECT_EQ(FaceError::UNABLE_TO_PROCESS, res.args->error);
}

// enroll with an invalid HAT should fail.
TEST_P(FaceHidlTest, EnrollRemotelyGarbageHatTest) {
    // Filling HAT with pseudorandom invalid data.
    // Using default seed to make the test reproducible.
    std::mt19937 gen(std::mt19937::default_seed);
    std::uniform_int_distribution<uint8_t> dist;
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; ++i) {
        token[i] = dist(gen);
    }

    Return<Status> ret = mService->enrollRemotely(token, kTimeoutSec, {});
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));

    // onError should be called with a meaningful (nonzero) error.
    auto res = mCallback->WaitForCallback(kCallbackNameOnError);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kUserId, res.args->userId);
    EXPECT_EQ(FaceError::UNABLE_TO_PROCESS, res.args->error);
}

}  // anonymous namespace

INSTANTIATE_TEST_SUITE_P(
        PerInstance, FaceHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IBiometricsFace::descriptor)),
        android::hardware::PrintInstanceNameToString);
