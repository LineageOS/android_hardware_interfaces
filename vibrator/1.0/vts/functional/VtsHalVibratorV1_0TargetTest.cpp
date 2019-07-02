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

#define LOG_TAG "vibrator_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/vibrator/1.0/IVibrator.h>
#include <android/hardware/vibrator/1.0/types.h>
#include <unistd.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::sp;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::vibrator::V1_0::Effect;
using ::android::hardware::vibrator::V1_0::EffectStrength;
using ::android::hardware::vibrator::V1_0::IVibrator;
using ::android::hardware::vibrator::V1_0::Status;

#define EXPECT_OK(ret) EXPECT_TRUE((ret).isOk())

// Test environment for Vibrator HIDL HAL.
class VibratorHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
 public:
  // get the test environment singleton
  static VibratorHidlEnvironment* Instance() {
      static VibratorHidlEnvironment* instance = new VibratorHidlEnvironment;
      return instance;
  }

  virtual void registerTestServices() override { registerTestService<IVibrator>(); }

 private:
  VibratorHidlEnvironment() {}
};

// The main test class for VIBRATOR HIDL HAL.
class VibratorHidlTest : public ::testing::VtsHalHidlTargetTestBase {
 public:
  virtual void SetUp() override {
    vibrator = ::testing::VtsHalHidlTargetTestBase::getService<IVibrator>(
        VibratorHidlEnvironment::Instance()->getServiceName<IVibrator>());
    ASSERT_NE(vibrator, nullptr);
  }

  virtual void TearDown() override {}

  sp<IVibrator> vibrator;
};

static void validatePerformEffect(Status status, uint32_t lengthMs) {
  ASSERT_TRUE(status == Status::OK || status == Status::UNSUPPORTED_OPERATION);
  if (status == Status::OK) {
      ASSERT_GT(lengthMs, static_cast<uint32_t>(0));
  } else {
      ASSERT_EQ(lengthMs, static_cast<uint32_t>(0));
  }
}

static void validatePerformEffectBadInput(Status status, uint32_t lengthMs) {
    ASSERT_EQ(Status::UNSUPPORTED_OPERATION, status);
    ASSERT_EQ(static_cast<uint32_t>(0), lengthMs)
            << "Effects that return UNSUPPORTED_OPERATION must have a duration of zero";
}

TEST_F(VibratorHidlTest, OnThenOffBeforeTimeout) {
  EXPECT_EQ(Status::OK, vibrator->on(2000));
  sleep(1);
  EXPECT_EQ(Status::OK, vibrator->off());
}

TEST_F(VibratorHidlTest, PerformEffect) {
  vibrator->perform(Effect::CLICK, EffectStrength::MEDIUM, validatePerformEffect);
  vibrator->perform(Effect::DOUBLE_CLICK, EffectStrength::LIGHT, validatePerformEffect);
}

/*
 * Test to make sure effect values above the valid range are rejected.
 */
TEST_F(VibratorHidlTest, PerformEffect_BadEffects_AboveValidRange) {
    Effect effect = *std::prev(hidl_enum_range<Effect>().end());
    Effect badEffect = static_cast<Effect>(static_cast<int32_t>(effect) + 1);
    EXPECT_OK(vibrator->perform(badEffect, EffectStrength::LIGHT, validatePerformEffectBadInput));
}

/*
 * Test to make sure effect values below the valid range are rejected.
 */
TEST_F(VibratorHidlTest, PerformEffect_BadEffects_BelowValidRange) {
    Effect effect = *hidl_enum_range<Effect>().begin();
    Effect badEffect = static_cast<Effect>(static_cast<int32_t>(effect) - 1);
    EXPECT_OK(vibrator->perform(badEffect, EffectStrength::LIGHT, validatePerformEffectBadInput));
}

/*
 * Test to make sure strength values above the valid range are rejected.
 */
TEST_F(VibratorHidlTest, PerformEffect_BadStrength_AboveValidRange) {
    EffectStrength strength = *std::prev(hidl_enum_range<EffectStrength>().end());
    EffectStrength badStrength = static_cast<EffectStrength>(static_cast<int32_t>(strength) + 1);
    EXPECT_OK(vibrator->perform(Effect::CLICK, badStrength, validatePerformEffectBadInput));
}

/*
 * Test to make sure strength values below the valid range are rejected.
 */
TEST_F(VibratorHidlTest, PerformEffect_BadStrength_BelowValidRange) {
    EffectStrength strength = *hidl_enum_range<EffectStrength>().begin();
    EffectStrength badStrength = static_cast<EffectStrength>(static_cast<int32_t>(strength) - 1);
    EXPECT_OK(vibrator->perform(Effect::CLICK, badStrength, validatePerformEffectBadInput));
}

TEST_F(VibratorHidlTest, ChangeVibrationalAmplitude) {
  if (vibrator->supportsAmplitudeControl()) {
    EXPECT_EQ(Status::OK, vibrator->setAmplitude(1));
    EXPECT_EQ(Status::OK, vibrator->on(2000));
    EXPECT_EQ(Status::OK, vibrator->setAmplitude(128));
    sleep(1);
    EXPECT_EQ(Status::OK, vibrator->setAmplitude(255));
    sleep(1);
  }
}

TEST_F(VibratorHidlTest, AmplitudeOutsideRangeFails) {
  if (vibrator->supportsAmplitudeControl()) {
    EXPECT_EQ(Status::BAD_VALUE, vibrator->setAmplitude(0));
  }
}

TEST_F(VibratorHidlTest, SetAmplitudeReturnUnsupportedOperationIfNotSupported) {
  if (!vibrator->supportsAmplitudeControl()) {
    EXPECT_EQ(Status::UNSUPPORTED_OPERATION, vibrator->setAmplitude(1));
  }
}

int main(int argc, char **argv) {
  ::testing::AddGlobalTestEnvironment(VibratorHidlEnvironment::Instance());
  ::testing::InitGoogleTest(&argc, argv);
  VibratorHidlEnvironment::Instance()->init(&argc, argv);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;
  return status;
}
