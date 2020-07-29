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

#define ASSERT_OK(v) ASSERT_TRUE(v.isOk())

#include <android/hardware/biometrics/fingerprint/2.3/IBiometricsFingerprint.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/HidlSupport.h>
#include <hidl/ServiceManagement.h>

namespace {

namespace hidl_interface_2_3 = android::hardware::biometrics::fingerprint::V2_3;

using hidl_interface_2_3::IBiometricsFingerprint;

using android::sp;

// Callback arguments that need to be captured for the tests.
struct FingerprintCallbackArgs {};

class FingerprintHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        mService = IBiometricsFingerprint::getService(GetParam());
        ASSERT_NE(mService, nullptr);
    }

    sp<IBiometricsFingerprint> mService;
};

// This method returns true or false depending on the implementation.
TEST_P(FingerprintHidlTest, isUdfpsTest) {
    // Arbitrary ID
    uint32_t sensorId = 1234;
    ASSERT_OK(mService->isUdfps(sensorId));
}

// This method that doesn't return anything.
TEST_P(FingerprintHidlTest, onFingerDownTest) {
    ASSERT_OK(mService->onFingerDown(1, 2, 3.0f, 4.0f));
}

// This method that doesn't return anything.
TEST_P(FingerprintHidlTest, onFingerUp) {
    ASSERT_OK(mService->onFingerUp());
}

}  // anonymous namespace

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(FingerprintHidlTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, FingerprintHidlTest,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IBiometricsFingerprint::descriptor)),
                         android::hardware::PrintInstanceNameToString);
