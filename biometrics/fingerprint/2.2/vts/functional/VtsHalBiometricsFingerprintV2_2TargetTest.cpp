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

#define LOG_TAG "fingerprint_hidl_hal_test"

#include <VtsHalHidlTargetCallbackBase.h>
#include <android-base/properties.h>
#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprintClientCallback.h>
#include <android/hardware/biometrics/fingerprint/2.2/IBiometricsFingerprint.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>

#include <cinttypes>
#include <random>

using android::sp;
using android::base::GetUintProperty;
using android::hardware::hidl_handle;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo;
using android::hardware::biometrics::fingerprint::V2_1::FingerprintError;
using android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprintClientCallback;
using android::hardware::biometrics::fingerprint::V2_1::RequestStatus;
using android::hardware::biometrics::fingerprint::V2_2::IBiometricsFingerprint;

namespace {

constexpr uint32_t kTimeoutSec = 3;
constexpr auto kTimeout = std::chrono::seconds(kTimeoutSec);
constexpr uint32_t kGroupId = 99;
constexpr char kCallbackNameOnError[] = "onError";

// Callback arguments that need to be captured for the tests.
struct FingerprintCallbackArgs {
    // The error passed to the last onError() callback.
    FingerprintError error;

    // The deviceId passed to the last callback.
    uint64_t deviceId;
};

// Test callback class for the BiometricsFingerprint HAL.
// The HAL will call these callback methods to notify about completed operations
// or encountered errors.
class FingerprintCallback : public ::testing::VtsHalHidlTargetCallbackBase<FingerprintCallbackArgs>,
                            public IBiometricsFingerprintClientCallback {
  public:
    Return<void> onEnrollResult(uint64_t, uint32_t, uint32_t, uint32_t) override { return Void(); }

    Return<void> onAcquired(uint64_t, FingerprintAcquiredInfo, int32_t) override { return Void(); }

    Return<void> onAuthenticated(uint64_t, uint32_t, uint32_t, const hidl_vec<uint8_t>&) override {
        return Void();
    }

    Return<void> onError(uint64_t deviceId, FingerprintError error, int32_t) override {
        FingerprintCallbackArgs args = {};
        args.error = error;
        args.deviceId = deviceId;
        NotifyFromCallback(kCallbackNameOnError, args);
        return Void();
    }

    Return<void> onRemoved(uint64_t, uint32_t, uint32_t, uint32_t) override { return Void(); }

    Return<void> onEnumerate(uint64_t, uint32_t, uint32_t, uint32_t) override { return Void(); }
};

class FingerprintHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        mService = IBiometricsFingerprint::getService(GetParam());
        ASSERT_NE(mService, nullptr);
        mCallback = new FingerprintCallback();
        mCallback->SetWaitTimeoutDefault(kTimeout);
        Return<uint64_t> ret1 = mService->setNotify(mCallback);
        ASSERT_NE(0UL, static_cast<uint64_t>(ret1));

        /*
         * Devices shipped from now on will instead store
         * fingerprint data under /data/vendor_de/<user-id>/fpdata.
         * Support for /data/vendor_de and /data/vendor_ce has been added to vold.
         */

        auto api_level = GetUintProperty<uint64_t>("ro.product.first_api_level", 0);
        if (api_level == 0) {
            api_level = GetUintProperty<uint64_t>("ro.build.version.sdk", 0);
        }
        ASSERT_NE(api_level, 0);

        // 27 is the API number for O-MR1
        string tmpDir;
        if (api_level <= 27) {
            tmpDir = "/data/system/users/0/fpdata/";
        } else {
            tmpDir = "/data/vendor_de/0/fpdata/";
        }

        Return<RequestStatus> res = mService->setActiveGroup(kGroupId, tmpDir);
        ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));
    }

    sp<IBiometricsFingerprint> mService;
    sp<FingerprintCallback> mCallback;
};

// Enroll with an invalid (all zeroes) HAT should fail.
TEST_P(FingerprintHidlTest, EnrollZeroHatTest) {
    // Filling HAT with zeros
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; i++) {
        token[i] = 0;
    }

    hidl_handle windowId = nullptr;
    Return<RequestStatus> ret = mService->enroll_2_2(token, kGroupId, kTimeoutSec, windowId);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(ret));

    // At least one call to onError should occur
    auto res = mCallback->WaitForCallback(kCallbackNameOnError);
    ASSERT_NE(FingerprintError::ERROR_NO_ERROR, res.args->error);
}

// Enroll with an invalid (null) HAT should fail.
TEST_P(FingerprintHidlTest, EnrollGarbageHatTest) {
    // Filling HAT with pseudorandom invalid data.
    // Using default seed to make the test reproducible.
    std::mt19937 gen(std::mt19937::default_seed);
    std::uniform_int_distribution<uint8_t> dist;
    hidl_vec<uint8_t> token(69);
    for (size_t i = 0; i < 69; ++i) {
        token[i] = dist(gen);
    }

    hidl_handle windowId = nullptr;
    Return<RequestStatus> ret = mService->enroll_2_2(token, kGroupId, kTimeoutSec, windowId);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(ret));

    // At least one call to onError should occur
    auto res = mCallback->WaitForCallback(kCallbackNameOnError);
    ASSERT_NE(FingerprintError::ERROR_NO_ERROR, res.args->error);
}

}  // anonymous namespace

INSTANTIATE_TEST_SUITE_P(PerInstance, FingerprintHidlTest,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IBiometricsFingerprint::descriptor)),
                         android::hardware::PrintInstanceNameToString);
