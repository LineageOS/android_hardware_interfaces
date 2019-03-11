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

#define LOG_TAG "face_hidl_test"

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/hardware/biometrics/face/1.0/IBiometricsFace.h>
#include <android/hardware/biometrics/face/1.0/IBiometricsFaceClientCallback.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/Condition.h>

#include <cinttypes>
#include <cstdint>
#include <future>
#include <utility>

using android::Condition;
using android::Mutex;
using android::sp;
using android::base::GetUintProperty;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::biometrics::face::V1_0::FaceAcquiredInfo;
using android::hardware::biometrics::face::V1_0::FaceError;
using android::hardware::biometrics::face::V1_0::Feature;
using android::hardware::biometrics::face::V1_0::IBiometricsFace;
using android::hardware::biometrics::face::V1_0::IBiometricsFaceClientCallback;
using android::hardware::biometrics::face::V1_0::OptionalBool;
using android::hardware::biometrics::face::V1_0::OptionalUint64;
using android::hardware::biometrics::face::V1_0::Status;

namespace {

const uint32_t kTimeout = 3;
const std::chrono::seconds kTimeoutInSeconds = std::chrono::seconds(kTimeout);
const uint32_t kUserId = 99;
const uint32_t kFaceId = 5;
const char kTmpDir[] = "/data/system/users/0/facedata";
const int kIterations = 1000;

const auto kAssertCallbackIsSet = [](const OptionalUint64& res) {
    ASSERT_EQ(Status::OK, res.status);
    // Makes sure the "deviceId" represented by "res.value" is not 0.
    // 0 would mean the HIDL is not available.
    ASSERT_NE(0UL, res.value);
};

// Wait for a callback to occur (signaled by the given future) up to the
// provided timeout. If the future is invalid or the callback does not come
// within the given time, returns false.
template <class ReturnType>
bool waitForCallback(std::future<ReturnType> future,
                     std::chrono::milliseconds timeout = kTimeoutInSeconds) {
    auto expiration = std::chrono::system_clock::now() + timeout;
    EXPECT_TRUE(future.valid());
    if (future.valid()) {
        std::future_status status = future.wait_until(expiration);
        EXPECT_NE(std::future_status::timeout, status) << "Timed out waiting for callback";
        if (status == std::future_status::ready) {
            return true;
        }
    }
    return false;
}

// Base callback implementation that just logs all callbacks by default
class FaceCallbackBase : public IBiometricsFaceClientCallback {
  public:
    Return<void> onEnrollResult(uint64_t, uint32_t, int32_t, uint32_t) override {
        ALOGD("Enroll callback called.");
        return Return<void>();
    }

    Return<void> onAuthenticated(uint64_t, uint32_t, int32_t, const hidl_vec<uint8_t>&) override {
        ALOGD("Authenticated callback called.");
        return Return<void>();
    }

    Return<void> onAcquired(uint64_t, int32_t, FaceAcquiredInfo, int32_t) override {
        ALOGD("Acquired callback called.");
        return Return<void>();
    }

    Return<void> onError(uint64_t, int32_t, FaceError, int32_t) override {
        ALOGD("Error callback called.");
        EXPECT_TRUE(false);  // fail any test that triggers an error
        return Return<void>();
    }

    Return<void> onRemoved(uint64_t, const hidl_vec<uint32_t>&, int32_t) override {
        ALOGD("Removed callback called.");
        return Return<void>();
    }

    Return<void> onEnumerate(uint64_t, const hidl_vec<uint32_t>&, int32_t /* userId */) override {
        ALOGD("Enumerate callback called.");
        return Return<void>();
    }

    Return<void> onLockoutChanged(uint64_t) override {
        ALOGD("LockoutChanged callback called.");
        return Return<void>();
    }
};

class EnumerateCallback : public FaceCallbackBase {
  public:
    Return<void> onEnumerate(uint64_t, const hidl_vec<uint32_t>&, int32_t) override {
        promise.set_value();
        return Return<void>();
    }

    std::promise<void> promise;
};

class ErrorCallback : public FaceCallbackBase {
  public:
    ErrorCallback(bool filterErrors = false, FaceError errorType = FaceError::HW_UNAVAILABLE)
        : filterErrors(filterErrors), errorType(errorType), hasError(false) {}

    Return<void> onError(uint64_t, int32_t, FaceError error, int32_t) override {
        if ((filterErrors && errorType == error) || !filterErrors) {
            hasError = true;
            this->error = error;
            promise.set_value();
        }
        return Return<void>();
    }

    bool filterErrors;
    FaceError errorType;
    bool hasError;
    FaceError error;
    std::promise<void> promise;
};

class RemoveCallback : public FaceCallbackBase {
  public:
    explicit RemoveCallback(int32_t userId) : removeUserId(userId) {}

    Return<void> onRemoved(uint64_t, const hidl_vec<uint32_t>&, int32_t userId) override {
        EXPECT_EQ(removeUserId, userId);
        promise.set_value();
        return Return<void>();
    }

    int32_t removeUserId;
    std::promise<void> promise;
};

class LockoutChangedCallback : public FaceCallbackBase {
  public:
    Return<void> onLockoutChanged(uint64_t duration) override {
        this->hasDuration = true;
        this->duration = duration;
        promise.set_value();
        return Return<void>();
    }
    bool hasDuration;
    uint64_t duration;
    std::promise<void> promise;
};

// Test environment for Face HIDL HAL.
class FaceHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
  public:
    // get the test environment singleton
    static FaceHidlEnvironment* Instance() {
        static FaceHidlEnvironment* instance = new FaceHidlEnvironment;
        return instance;
    }

    void registerTestServices() override { registerTestService<IBiometricsFace>(); }
};

class FaceHidlTest : public ::testing::VtsHalHidlTargetTestBase {
  public:
    void SetUp() override {
        mService = ::testing::VtsHalHidlTargetTestBase::getService<IBiometricsFace>(
                FaceHidlEnvironment::Instance()->getServiceName<IBiometricsFace>());
        ASSERT_FALSE(mService == nullptr);
        Return<Status> res = mService->setActiveUser(kUserId, kTmpDir);
        ASSERT_EQ(Status::OK, static_cast<Status>(res));
    }

    void TearDown() override {}

    sp<IBiometricsFace> mService;
};

// The service should be reachable.
TEST_F(FaceHidlTest, ConnectTest) {
    sp<FaceCallbackBase> cb = new FaceCallbackBase();
    mService->setCallback(cb, kAssertCallbackIsSet);
}

// Starting the service with null callback should succeed.
TEST_F(FaceHidlTest, ConnectNullTest) {
    mService->setCallback(nullptr, kAssertCallbackIsSet);
}

// generateChallenge should always return a unique, cryptographically secure,
// non-zero number.
TEST_F(FaceHidlTest, GenerateChallengeTest) {
    std::map<uint64_t, int> m;
    for (int i = 0; i < kIterations; ++i) {
        mService->generateChallenge(kTimeout, [&m](const OptionalUint64& res) {
            ASSERT_EQ(Status::OK, res.status);
            EXPECT_NE(0UL, res.value);
            m[res.value]++;
            EXPECT_EQ(1UL, m[res.value]);
        });
    }
}

// enroll with an invalid (all zeroes) HAT should fail.
TEST_F(FaceHidlTest, EnrollZeroHatTest) {
    sp<ErrorCallback> cb = new ErrorCallback();
    mService->setCallback(cb, kAssertCallbackIsSet);

    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; i++) {
        token[i] = 0;
    }

    Return<Status> res = mService->enroll(token, kTimeout, {});
    ASSERT_EQ(Status::OK, static_cast<Status>(res));

    // At least one call to onError should occur
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));
    ASSERT_TRUE(cb->hasError);
}

// enroll with an invalid HAT should fail.
TEST_F(FaceHidlTest, EnrollGarbageHatTest) {
    sp<ErrorCallback> cb = new ErrorCallback();
    mService->setCallback(cb, kAssertCallbackIsSet);

    // Filling HAT with invalid data
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; ++i) {
        token[i] = i;
    }

    Return<Status> res = mService->enroll(token, kTimeout, {});
    ASSERT_EQ(Status::OK, static_cast<Status>(res));

    // At least one call to onError should occur
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));
    ASSERT_TRUE(cb->hasError);
}

// setFeature with an invalid (all zeros) HAT should fail.
TEST_F(FaceHidlTest, SetFeatureZeroHatTest) {
    sp<ErrorCallback> cb = new ErrorCallback();
    mService->setCallback(cb, kAssertCallbackIsSet);

    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; i++) {
        token[i] = 0;
    }

    Return<Status> res = mService->setFeature(Feature::REQUIRE_DIVERSITY, false, token, 0);
    ASSERT_EQ(Status::ILLEGAL_ARGUMENT, static_cast<Status>(res));
}

// setFeature with an invalid HAT should fail.
TEST_F(FaceHidlTest, SetFeatureGarbageHatTest) {
    sp<ErrorCallback> cb = new ErrorCallback();
    mService->setCallback(cb, kAssertCallbackIsSet);

    // Filling HAT with invalid data
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; ++i) {
        token[i] = i;
    }

    Return<Status> res = mService->setFeature(Feature::REQUIRE_DIVERSITY, false, token, 0);
    ASSERT_EQ(Status::ILLEGAL_ARGUMENT, static_cast<Status>(res));
}

void assertGetFeatureFails(sp<IBiometricsFace> service, int faceId, Feature feature) {
    std::promise<void> promise;

    // Features cannot be retrieved for invalid faces.
    Return<void> res = service->getFeature(feature, faceId, [&promise](const OptionalBool& result) {
        ASSERT_EQ(Status::ILLEGAL_ARGUMENT, result.status);
        promise.set_value();
    });
    ASSERT_TRUE(waitForCallback(promise.get_future()));
}

TEST_F(FaceHidlTest, GetFeatureRequireAttentionTest) {
    assertGetFeatureFails(mService, 0 /* faceId */, Feature::REQUIRE_ATTENTION);
}

TEST_F(FaceHidlTest, GetFeatureRequireDiversityTest) {
    assertGetFeatureFails(mService, 0 /* faceId */, Feature::REQUIRE_DIVERSITY);
}

// revokeChallenge should always return within the timeout
TEST_F(FaceHidlTest, RevokeChallengeTest) {
    sp<FaceCallbackBase> cb = new FaceCallbackBase();
    mService->setCallback(cb, kAssertCallbackIsSet);

    auto start = std::chrono::system_clock::now();
    mService->revokeChallenge();
    auto elapsed = std::chrono::system_clock::now() - start;
    ASSERT_GE(kTimeoutInSeconds, elapsed);
}

// The call to getAuthenticatorId should succeed.
TEST_F(FaceHidlTest, GetAuthenticatorIdTest) {
    mService->getAuthenticatorId(
            [](const OptionalUint64& res) { ASSERT_EQ(Status::OK, res.status); });
}

// The call to enumerate should succeed.
TEST_F(FaceHidlTest, EnumerateTest) {
    sp<EnumerateCallback> cb = new EnumerateCallback();
    mService->setCallback(cb, kAssertCallbackIsSet);
    Return<Status> res = mService->enumerate();
    ASSERT_EQ(Status::OK, static_cast<Status>(res));
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));
}

// The call to remove should succeed for any faceId
TEST_F(FaceHidlTest, RemoveFaceTest) {
    sp<ErrorCallback> cb = new ErrorCallback();
    mService->setCallback(cb, kAssertCallbackIsSet);

    // Remove a face
    Return<Status> res = mService->remove(kFaceId);
    ASSERT_EQ(Status::OK, static_cast<Status>(res));
}

// Remove should accept 0 to delete all faces
TEST_F(FaceHidlTest, RemoveAllFacesTest) {
    sp<ErrorCallback> cb = new ErrorCallback();
    mService->setCallback(cb, kAssertCallbackIsSet);

    // Remove all faces
    Return<Status> res = mService->remove(0);
    ASSERT_EQ(Status::OK, static_cast<Status>(res));
}

// Active user should successfully set to a writable location.
TEST_F(FaceHidlTest, SetActiveUserTest) {
    // Create an active user
    Return<Status> res = mService->setActiveUser(2, kTmpDir);
    ASSERT_EQ(Status::OK, static_cast<Status>(res));

    // Reset active user
    res = mService->setActiveUser(kUserId, kTmpDir);
    ASSERT_EQ(Status::OK, static_cast<Status>(res));
}

// Active user should fail to set to an unwritable location.
TEST_F(FaceHidlTest, SetActiveUserUnwritableTest) {
    // Create an active user to an unwritable location (device root dir)
    Return<Status> res = mService->setActiveUser(3, "/");
    ASSERT_NE(Status::OK, static_cast<Status>(res));

    // Reset active user
    res = mService->setActiveUser(kUserId, kTmpDir);
    ASSERT_EQ(Status::OK, static_cast<Status>(res));
}

// Active user should fail to set to a null location.
TEST_F(FaceHidlTest, SetActiveUserNullTest) {
    // Create an active user to a null location.
    Return<Status> res = mService->setActiveUser(4, nullptr);
    ASSERT_NE(Status::OK, static_cast<Status>(res));

    // Reset active user
    res = mService->setActiveUser(kUserId, kTmpDir);
    ASSERT_EQ(Status::OK, static_cast<Status>(res));
}

// Cancel should always return CANCELED from any starting state including
// the IDLE state.
TEST_F(FaceHidlTest, CancelTest) {
    sp<ErrorCallback> cb = new ErrorCallback(true, FaceError::CANCELED);
    mService->setCallback(cb, kAssertCallbackIsSet);

    Return<Status> res = mService->cancel();
    // check that we were able to make an IPC request successfully
    ASSERT_EQ(Status::OK, static_cast<Status>(res));

    // make sure callback was invoked within kTimeoutInSeconds
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));
    // check error should be CANCELED
    ASSERT_EQ(FaceError::CANCELED, cb->error);
}

TEST_F(FaceHidlTest, OnLockoutChangedTest) {
    sp<LockoutChangedCallback> cb = new LockoutChangedCallback();
    mService->setCallback(cb, kAssertCallbackIsSet);

    // Update active user and ensure lockout duration 0 is received
    mService->setActiveUser(5, kTmpDir);

    // Make sure callback was invoked
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));

    // Check that duration 0 was received
    ASSERT_EQ(0, cb->duration);
}

}  // anonymous namespace

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(FaceHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    FaceHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
