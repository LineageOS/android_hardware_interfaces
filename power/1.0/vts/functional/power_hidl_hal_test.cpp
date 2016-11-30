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

#define LOG_TAG "power_hidl_hal_test"
#include <android-base/logging.h>

#include <android/hardware/power/1.0/IPower.h>

#include <gtest/gtest.h>

using ::android::hardware::power::V1_0::IPower;
using ::android::hardware::power::V1_0::Feature;
using ::android::hardware::power::V1_0::PowerHint;
using ::android::hardware::power::V1_0::PowerStatePlatformSleepState;
using ::android::hardware::power::V1_0::Status;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::sp;

class PowerHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    power = IPower::getService("power");
    ASSERT_NE(power, nullptr);
  }

  virtual void TearDown() override {}

  sp<IPower> power;
};

// Sanity check Power::setInteractive.
TEST_F(PowerHidlTest, SetInteractive) {
  Return<void> ret;

  ret = power->setInteractive(true);
  ASSERT_TRUE(ret.getStatus().isOk());

  ret = power->setInteractive(false);
  ASSERT_TRUE(ret.getStatus().isOk());
}

// Sanity check Power::powerHint on good and bad inputs.
TEST_F(PowerHidlTest, PowerHint) {
  PowerHint badHint = static_cast<PowerHint>(0xA);
  auto hints = {PowerHint::VSYNC,         PowerHint::INTERACTION,
                PowerHint::VIDEO_ENCODE,  PowerHint::VIDEO_DECODE,
                PowerHint::LOW_POWER,     PowerHint::SUSTAINED_PERFORMANCE,
                PowerHint::VR_MODE,       PowerHint::LAUNCH,
                PowerHint::DISABLE_TOUCH, badHint};
  Return<void> ret;
  for (auto hint : hints) {
    ret = power->powerHint(hint, 1);
    ASSERT_TRUE(ret.getStatus().isOk());

    ret = power->powerHint(hint, 0);
    ASSERT_TRUE(ret.getStatus().isOk());
  }
}

// Sanity check Power::setFeature() on good and bad inputs.
TEST_F(PowerHidlTest, SetFeature) {
  Return<void> ret;
  ret = power->setFeature(Feature::POWER_FEATURE_DOUBLE_TAP_TO_WAKE, true);
  ASSERT_TRUE(ret.getStatus().isOk());
  ret = power->setFeature(Feature::POWER_FEATURE_DOUBLE_TAP_TO_WAKE, false);
  ASSERT_TRUE(ret.getStatus().isOk());

  Feature badFeature = static_cast<Feature>(0x2);
  ret = power->setFeature(badFeature, true);
  ASSERT_TRUE(ret.getStatus().isOk());
  ret = power->setFeature(badFeature, false);
  ASSERT_TRUE(ret.getStatus().isOk());
}

// Sanity check Power::getPlatformLowPowerStats().
TEST_F(PowerHidlTest, GetPlatformLowPowerStats) {
  hidl_vec<PowerStatePlatformSleepState> vec;
  Status s;
  auto cb = [&vec, &s](hidl_vec<PowerStatePlatformSleepState> states,
                       Status status) {
    vec = states;
    s = status;
  };
  Return<void> ret = power->getPlatformLowPowerStats(cb);
  ASSERT_TRUE(ret.getStatus().isOk());
  ASSERT_TRUE(s == Status::SUCCESS || s == Status::FILESYSTEM_ERROR);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;
  return status;
}
