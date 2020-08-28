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
#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprint.h>
#include <android/hardware/biometrics/fingerprint/2.2/IBiometricsFingerprintClientCallback.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>

#include <cinttypes>

namespace {

namespace hidl_interface = android::hardware::biometrics::fingerprint::V2_1;
namespace hidl_interface_2_2 = android::hardware::biometrics::fingerprint::V2_2;

using hidl_interface::FingerprintError;
using hidl_interface::IBiometricsFingerprint;
using hidl_interface::RequestStatus;

using android::sp;
using android::base::GetUintProperty;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;

constexpr uint32_t kTimeoutSec = 3;
constexpr auto kTimeout = std::chrono::seconds(kTimeoutSec);
constexpr uint32_t kGroupId = 99;
constexpr char kCallbackNameOnAcquired[] = "onAcquired";

// Callback arguments that need to be captured for the tests.
struct FingerprintCallbackArgs {
    // The info passed to the last onAcquired() callback.
    hidl_interface_2_2::FingerprintAcquiredInfo info;
};

// Test callback class for the BiometricsFingerprint HAL.
// The HAL will call these callback methods to notify about completed operations
// or encountered errors.
class FingerprintCallback : public ::testing::VtsHalHidlTargetCallbackBase<FingerprintCallbackArgs>,
                            public hidl_interface_2_2::IBiometricsFingerprintClientCallback {
  public:
    Return<void> onEnrollResult(uint64_t, uint32_t, uint32_t, uint32_t) override { return Void(); }

    Return<void> onAcquired(uint64_t, hidl_interface::FingerprintAcquiredInfo, int32_t) override {
        return Void();
    }

    Return<void> onAcquired_2_2(uint64_t, hidl_interface_2_2::FingerprintAcquiredInfo info,
                                int32_t) override {
        FingerprintCallbackArgs args = {};
        args.info = info;
        NotifyFromCallback(kCallbackNameOnAcquired, args);
        return Void();
    }

    Return<void> onAuthenticated(uint64_t, uint32_t, uint32_t, const hidl_vec<uint8_t>&) override {
        return Void();
    }

    Return<void> onError(uint64_t, FingerprintError, int32_t) override { return Void(); }

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

// The START message and onAcquired_2_2 method should exist and work together correctly.
// Note, this test doesn't use the HAL. It just makes sure that the newly added constant and
// callback compile. Unfortunately, there is no way to test the usage of the constant within the
// actual HAL.
TEST_P(FingerprintHidlTest, acquiredInfoStartTest) {
    mCallback->SetWaitTimeoutDefault(kTimeout);
    mCallback->onAcquired_2_2(0 /* deviceId */, hidl_interface_2_2::FingerprintAcquiredInfo::START,
                              0 /* vendorCode */);
    auto res = mCallback->WaitForCallback(kCallbackNameOnAcquired);
    ASSERT_EQ(hidl_interface_2_2::FingerprintAcquiredInfo::START, res.args->info);
}

}  // anonymous namespace

INSTANTIATE_TEST_SUITE_P(PerInstance, FingerprintHidlTest,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IBiometricsFingerprint::descriptor)),
                         android::hardware::PrintInstanceNameToString);
