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

#include <cutils/properties.h>

#include <android-base/file.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <android/hardware/power/1.0/IPower.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <fcntl.h>
#include <algorithm>

using ::android::hardware::power::V1_0::IPower;
using ::android::hardware::power::V1_0::Feature;
using ::android::hardware::power::V1_0::PowerHint;
using ::android::hardware::power::V1_0::PowerStatePlatformSleepState;
using ::android::hardware::power::V1_0::Status;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::sp;
using ::android::base::unique_fd;

using std::vector;

#define CPU_GOVERNOR_PATH \
  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define AVAILABLE_GOVERNORS_PATH \
  "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors"

class PowerHidlTest : public testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
      power = IPower::getService(GetParam());
      ASSERT_NE(power, nullptr);
  }

  virtual void TearDown() override {}

  sp<IPower> power;
};

// Validate Power::setInteractive.
TEST_P(PowerHidlTest, SetInteractive) {
  Return<void> ret;

  ret = power->setInteractive(true);
  ASSERT_TRUE(ret.isOk());

  ret = power->setInteractive(false);
  ASSERT_TRUE(ret.isOk());
}

// Test Power::setInteractive and Power::powerHint(Launch)
// with each available CPU governor, if available
TEST_P(PowerHidlTest, TryDifferentGovernors) {
  Return<void> ret;

  std::string old_governor, governors;
  if (!android::base::ReadFileToString(CPU_GOVERNOR_PATH, &old_governor) ||
      !android::base::ReadFileToString(AVAILABLE_GOVERNORS_PATH, &governors)) {
    // Files don't exist, so skip the rest of the test case
    SUCCEED();
    return;
  }
  auto all_governors = android::base::Split(governors, " \n");
  for (const auto &governor : all_governors) {
      ASSERT_TRUE(android::base::WriteStringToFile(governor, CPU_GOVERNOR_PATH));
    ret = power->setInteractive(true);
    ASSERT_TRUE(ret.isOk());

    ret = power->setInteractive(false);
    ASSERT_TRUE(ret.isOk());

    ret = power->setInteractive(false);
    ASSERT_TRUE(ret.isOk());

    power->powerHint(PowerHint::LAUNCH, 1);
    power->powerHint(PowerHint::LAUNCH, 0);
  }

  ASSERT_TRUE(android::base::WriteStringToFile(old_governor, CPU_GOVERNOR_PATH));
}

// Validate Power::powerHint on good and bad inputs.
TEST_P(PowerHidlTest, PowerHint) {
  PowerHint badHint = static_cast<PowerHint>(0xA);
  auto hints = {PowerHint::VSYNC,         PowerHint::INTERACTION,
                PowerHint::VIDEO_ENCODE,  PowerHint::VIDEO_DECODE,
                PowerHint::LOW_POWER,     PowerHint::SUSTAINED_PERFORMANCE,
                PowerHint::VR_MODE,       PowerHint::LAUNCH,
                badHint};
  Return<void> ret;
  for (auto hint : hints) {
    ret = power->powerHint(hint, 30000);
    ASSERT_TRUE(ret.isOk());

    ret = power->powerHint(hint, 0);
    ASSERT_TRUE(ret.isOk());
  }

  // Turning these hints on in different orders triggers different code paths,
  // so iterate over possible orderings.
  std::vector<PowerHint> hints2 = {PowerHint::LAUNCH, PowerHint::VR_MODE,
                                   PowerHint::SUSTAINED_PERFORMANCE,
                                   PowerHint::INTERACTION};
  auto compareHints = [](PowerHint l, PowerHint r) {
    return static_cast<uint32_t>(l) < static_cast<uint32_t>(r);
  };
  std::sort(hints2.begin(), hints2.end(), compareHints);
  do {
    for (auto iter = hints2.begin(); iter != hints2.end(); iter++) {
      ret = power->powerHint(*iter, 0);
      ASSERT_TRUE(ret.isOk());
    }
    for (auto iter = hints2.begin(); iter != hints2.end(); iter++) {
      ret = power->powerHint(*iter, 30000);
      ASSERT_TRUE(ret.isOk());
    }
  } while (std::next_permutation(hints2.begin(), hints2.end(), compareHints));
}

// Validate Power::setFeature() on good and bad inputs.
TEST_P(PowerHidlTest, SetFeature) {
  Return<void> ret;
  ret = power->setFeature(Feature::POWER_FEATURE_DOUBLE_TAP_TO_WAKE, true);
  ASSERT_TRUE(ret.isOk());
  ret = power->setFeature(Feature::POWER_FEATURE_DOUBLE_TAP_TO_WAKE, false);
  ASSERT_TRUE(ret.isOk());

  Feature badFeature = static_cast<Feature>(0x2);
  ret = power->setFeature(badFeature, true);
  ASSERT_TRUE(ret.isOk());
  ret = power->setFeature(badFeature, false);
  ASSERT_TRUE(ret.isOk());
}

// Validate Power::getPlatformLowPowerStats().
TEST_P(PowerHidlTest, GetPlatformLowPowerStats) {
  hidl_vec<PowerStatePlatformSleepState> vec;
  Status s;
  auto cb = [&vec, &s](hidl_vec<PowerStatePlatformSleepState> states,
                       Status status) {
    vec = states;
    s = status;
  };
  Return<void> ret = power->getPlatformLowPowerStats(cb);
  ASSERT_TRUE(ret.isOk());
  ASSERT_TRUE(s == Status::SUCCESS || s == Status::FILESYSTEM_ERROR);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PowerHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, PowerHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IPower::descriptor)),
        android::hardware::PrintInstanceNameToString);
