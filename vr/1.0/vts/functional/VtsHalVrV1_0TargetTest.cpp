/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "vr_hidl_hal_test"
#include <android-base/logging.h>
#include <android/hardware/vr/1.0/IVr.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

using ::android::hardware::vr::V1_0::IVr;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

// The main test class for VR HIDL HAL.
class VrHidlTest : public ::testing::TestWithParam<std::string> {
 public:
  void SetUp() override {
    vr = IVr::getService(GetParam());
    ASSERT_NE(vr, nullptr);
  }

  void TearDown() override {}

  sp<IVr> vr;
};

// Sanity check that Vr::init does not crash.
TEST_P(VrHidlTest, Init) {
  EXPECT_TRUE(vr->init().isOk());
}

// Sanity check Vr::setVrMode is able to enable and disable VR mode.
TEST_P(VrHidlTest, SetVrMode) {
  EXPECT_TRUE(vr->init().isOk());
  EXPECT_TRUE(vr->setVrMode(true).isOk());
  EXPECT_TRUE(vr->setVrMode(false).isOk());
}

// Sanity check that Vr::init and Vr::setVrMode can be used in any order.
TEST_P(VrHidlTest, ReInit) {
  EXPECT_TRUE(vr->init().isOk());
  EXPECT_TRUE(vr->setVrMode(true).isOk());
  EXPECT_TRUE(vr->init().isOk());
  EXPECT_TRUE(vr->setVrMode(false).isOk());
  EXPECT_TRUE(vr->init().isOk());
  EXPECT_TRUE(vr->setVrMode(false).isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VrHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, VrHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IVr::descriptor)),
        android::hardware::PrintInstanceNameToString);

