/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android/hardware/biometrics/face/1.0/IBiometricsFace.h>
#include <android/hardware/biometrics/face/1.0/IBiometricsFaceClientCallback.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <android-base/logging.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <chrono>
#include <cstdint>
#include <random>
#include <thread>

using android::sp;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::biometrics::face::V1_0::FaceAcquiredInfo;
using android::hardware::biometrics::face::V1_0::FaceError;
using android::hardware::biometrics::face::V1_0::Feature;
using android::hardware::biometrics::face::V1_0::IBiometricsFace;
using android::hardware::biometrics::face::V1_0::IBiometricsFaceClientCallback;
using android::hardware::biometrics::face::V1_0::OptionalBool;
using android::hardware::biometrics::face::V1_0::OptionalUint64;
using android::hardware::biometrics::face::V1_0::Status;

namespace {

// Arbitrary, nonexistent userId
constexpr uint32_t kUserId = 9;
// Arbitrary, nonexistent faceId
constexpr uint32_t kFaceId = 5;
constexpr uint32_t kTimeoutSec = 3;
constexpr auto kTimeout = std::chrono::seconds(kTimeoutSec);
constexpr int kGenerateChallengeIterations = 10;
constexpr char kFacedataDir[] = "/data/vendor_de/0/facedata";
constexpr char kCallbackNameOnEnrollResult[] = "onEnrollResult";
constexpr char kCallbackNameOnAuthenticated[] = "onAuthenticated";
constexpr char kCallbackNameOnAcquired[] = "onAcquired";
constexpr char kCallbackNameOnError[] = "onError";
constexpr char kCallbackNameOnRemoved[] = "onRemoved";
constexpr char kCallbackNameOnEnumerate[] = "onEnumerate";
constexpr char kCallbackNameOnLockoutChanged[] = "onLockoutChanged";

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
    Return<void> onEnrollResult(uint64_t, uint32_t, int32_t userId, uint32_t) override {
        FaceCallbackArgs args = {};
        args.userId = userId;
        NotifyFromCallback(kCallbackNameOnEnrollResult, args);
        return Void();
    }

    Return<void> onAuthenticated(uint64_t, uint32_t, int32_t userId,
                                 const hidl_vec<uint8_t>&) override {
        FaceCallbackArgs args = {};
        args.userId = userId;
        NotifyFromCallback(kCallbackNameOnAuthenticated, args);
        return Void();
    }

    Return<void> onAcquired(uint64_t, int32_t userId, FaceAcquiredInfo, int32_t) override {
        FaceCallbackArgs args = {};
        args.userId = userId;
        NotifyFromCallback(kCallbackNameOnAcquired, args);
        return Void();
    }

    Return<void> onError(uint64_t, int32_t userId, FaceError error, int32_t) override {
        FaceCallbackArgs args = {};
        args.error = error;
        args.userId = userId;
        NotifyFromCallback(kCallbackNameOnError, args);
        return Void();
    }

    Return<void> onRemoved(uint64_t, const hidl_vec<uint32_t>&, int32_t userId) override {
        FaceCallbackArgs args = {};
        args.userId = userId;
        NotifyFromCallback(kCallbackNameOnRemoved, args);
        return Void();
    }

    Return<void> onEnumerate(uint64_t, const hidl_vec<uint32_t>&, int32_t userId) override {
        FaceCallbackArgs args = {};
        args.userId = userId;
        NotifyFromCallback(kCallbackNameOnEnumerate, args);
        return Void();
    }

    Return<void> onLockoutChanged(uint64_t) override {
        NotifyFromCallback(kCallbackNameOnLockoutChanged);
        return Void();
    }
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

    void TearDown() override {
        // Hack to allow the asynchronous operations to finish on time.
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    sp<IBiometricsFace> mService;
    sp<FaceCallback> mCallback;
};

// generateChallenge should always return a unique, cryptographically secure,
// non-zero number.
TEST_P(FaceHidlTest, GenerateChallengeTest) {
    std::map<uint64_t, int> m;
    for (int i = 0; i < kGenerateChallengeIterations; ++i) {
        Return<void> ret =
                mService->generateChallenge(kTimeoutSec, [&m](const OptionalUint64& res) {
                    ASSERT_EQ(Status::OK, res.status);
                    EXPECT_NE(0UL, res.value);
                    m[res.value]++;
                    EXPECT_EQ(1UL, m[res.value]);
                });
        ASSERT_TRUE(ret.isOk());
    }
}

// enroll with an invalid (all zeroes) HAT should fail.
TEST_P(FaceHidlTest, EnrollZeroHatTest) {
    // Filling HAT with zeros
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; i++) {
        token[i] = 0;
    }

    Return<Status> ret = mService->enroll(token, kTimeoutSec, {});
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));

    // onError should be called with a meaningful (nonzero) error.
    auto res = mCallback->WaitForCallback(kCallbackNameOnError);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kUserId, res.args->userId);
    EXPECT_EQ(FaceError::UNABLE_TO_PROCESS, res.args->error);
}

// enroll with an invalid HAT should fail.
TEST_P(FaceHidlTest, EnrollGarbageHatTest) {
    // Filling HAT with pseudorandom invalid data.
    // Using default seed to make the test reproducible.
    std::mt19937 gen(std::mt19937::default_seed);
    std::uniform_int_distribution<uint8_t> dist;
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; ++i) {
        token[i] = dist(gen);
    }

    Return<Status> ret = mService->enroll(token, kTimeoutSec, {});
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));

    // onError should be called with a meaningful (nonzero) error.
    auto res = mCallback->WaitForCallback(kCallbackNameOnError);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kUserId, res.args->userId);
    EXPECT_EQ(FaceError::UNABLE_TO_PROCESS, res.args->error);
}

// setFeature with an invalid (all zeros) HAT should fail.
TEST_P(FaceHidlTest, SetFeatureZeroHatTest) {
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; i++) {
        token[i] = 0;
    }

    Return<Status> ret = mService->setFeature(Feature::REQUIRE_DIVERSITY, false, token, 0);
    ASSERT_EQ(Status::ILLEGAL_ARGUMENT, static_cast<Status>(ret));
}

// setFeature with an invalid HAT should fail.
TEST_P(FaceHidlTest, SetFeatureGarbageHatTest) {
    // Filling HAT with pseudorandom invalid data.
    // Using default seed to make the test reproducible.
    std::mt19937 gen(std::mt19937::default_seed);
    std::uniform_int_distribution<uint8_t> dist;
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; ++i) {
        token[i] = dist(gen);
    }

    Return<Status> ret = mService->setFeature(Feature::REQUIRE_DIVERSITY, false, token, 0);
    ASSERT_EQ(Status::ILLEGAL_ARGUMENT, static_cast<Status>(ret));
}

void assertGetFeatureFails(const sp<IBiometricsFace>& service, uint32_t faceId, Feature feature) {
    // Features cannot be retrieved for invalid faces.
    Return<void> res = service->getFeature(feature, faceId, [](const OptionalBool& result) {
        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, result.status);
    });
    ASSERT_TRUE(res.isOk());
}

TEST_P(FaceHidlTest, GetFeatureRequireAttentionTest) {
    assertGetFeatureFails(mService, 0 /* faceId */, Feature::REQUIRE_ATTENTION);
}

TEST_P(FaceHidlTest, GetFeatureRequireDiversityTest) {
    assertGetFeatureFails(mService, 0 /* faceId */, Feature::REQUIRE_DIVERSITY);
}

// revokeChallenge should always return within the timeout
TEST_P(FaceHidlTest, RevokeChallengeTest) {
    auto start = std::chrono::system_clock::now();
    Return<Status> ret = mService->revokeChallenge();
    auto elapsed = std::chrono::system_clock::now() - start;
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));
    ASSERT_GE(kTimeout, elapsed);
}

// The call to getAuthenticatorId should succeed.
TEST_P(FaceHidlTest, GetAuthenticatorIdTest) {
    Return<void> ret = mService->getAuthenticatorId(
            [](const OptionalUint64& res) { ASSERT_EQ(Status::OK, res.status); });
    ASSERT_TRUE(ret.isOk());
}

// The call to enumerate should succeed.
TEST_P(FaceHidlTest, EnumerateTest) {
    Return<Status> ret = mService->enumerate();
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));
    auto res = mCallback->WaitForCallback(kCallbackNameOnEnumerate);
    EXPECT_EQ(kUserId, res.args->userId);
    EXPECT_TRUE(res.no_timeout);
}

// The call to remove should succeed for any faceId
TEST_P(FaceHidlTest, RemoveFaceTest) {
    // Remove a face
    Return<Status> ret = mService->remove(kFaceId);
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));
}

// Remove should accept 0 to delete all faces
TEST_P(FaceHidlTest, RemoveAllFacesTest) {
    // Remove all faces
    Return<Status> ret = mService->remove(0);
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));
}

// Active user should successfully set to a writable location.
TEST_P(FaceHidlTest, SetActiveUserTest) {
    // Create an active user
    Return<Status> ret = mService->setActiveUser(2, kFacedataDir);
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));

    // Reset active user
    ret = mService->setActiveUser(kUserId, kFacedataDir);
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));
}

// Active user should fail to set to an unwritable location.
TEST_P(FaceHidlTest, SetActiveUserUnwritableTest) {
    // Create an active user to an unwritable location (device root dir)
    Return<Status> ret = mService->setActiveUser(3, "/");
    ASSERT_NE(Status::OK, static_cast<Status>(ret));

    // Reset active user
    ret = mService->setActiveUser(kUserId, kFacedataDir);
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));
}

// Active user should fail to set to a null location.
TEST_P(FaceHidlTest, SetActiveUserNullTest) {
    // Create an active user to a null location.
    Return<Status> ret = mService->setActiveUser(4, nullptr);
    ASSERT_NE(Status::OK, static_cast<Status>(ret));

    // Reset active user
    ret = mService->setActiveUser(kUserId, kFacedataDir);
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));
}

// Cancel should always return CANCELED from any starting state including
// the IDLE state.
TEST_P(FaceHidlTest, CancelTest) {
    Return<Status> ret = mService->cancel();
    // check that we were able to make an IPC request successfully
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));
    auto res = mCallback->WaitForCallback(kCallbackNameOnError);
    // make sure callback was invoked within kRevokeChallengeTimeout
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(kUserId, res.args->userId);
    EXPECT_EQ(FaceError::CANCELED, res.args->error);
}

TEST_P(FaceHidlTest, OnLockoutChangedTest) {
    // Update active user and ensure onLockoutChanged was called.
    Return<Status> ret = mService->setActiveUser(kUserId + 1, kFacedataDir);
    ASSERT_EQ(Status::OK, static_cast<Status>(ret));

    // Make sure callback was invoked
    auto res = mCallback->WaitForCallback(kCallbackNameOnLockoutChanged);
    EXPECT_TRUE(res.no_timeout);
}

}  // anonymous namespace

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(FaceHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, FaceHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IBiometricsFace::descriptor)),
        android::hardware::PrintInstanceNameToString);
