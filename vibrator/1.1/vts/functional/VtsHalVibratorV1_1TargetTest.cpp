/*
 * Copyright (C) 2017 The Android Open Source Project
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
#include <android/hardware/vibrator/1.1/IVibrator.h>
#include <android/hardware/vibrator/1.1/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <unistd.h>

using ::android::sp;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::vibrator::V1_0::EffectStrength;
using ::android::hardware::vibrator::V1_0::Status;
using ::android::hardware::vibrator::V1_1::Effect_1_1;
using ::android::hardware::vibrator::V1_1::IVibrator;

#define EXPECT_OK(ret) EXPECT_TRUE((ret).isOk())

// The main test class for VIBRATOR HIDL HAL 1.1.
class VibratorHidlTest_1_1 : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        vibrator = IVibrator::getService(GetParam());
        ASSERT_NE(vibrator, nullptr);
    }

    virtual void TearDown() override {}

    sp<IVibrator> vibrator;
};

static void validatePerformEffect(Status status, uint32_t lengthMs) {
    ASSERT_TRUE(status == Status::OK || status == Status::UNSUPPORTED_OPERATION);
    if (status == Status::OK) {
        ASSERT_GT(lengthMs, static_cast<uint32_t>(0))
            << "Effects that return OK must return a non-zero duration";
    } else {
        ASSERT_EQ(lengthMs, static_cast<uint32_t>(0))
            << "Effects that return UNSUPPORTED_OPERATION must have a duration of zero";
    }
}

static void validatePerformEffectBadInput(Status status, uint32_t lengthMs) {
    ASSERT_EQ(Status::UNSUPPORTED_OPERATION, status);
    ASSERT_EQ(static_cast<uint32_t>(0), lengthMs)
            << "Effects that return UNSUPPORTED_OPERATION must have a duration of zero";
}

TEST_P(VibratorHidlTest_1_1, PerformEffect_1_1) {
    vibrator->perform_1_1(Effect_1_1::CLICK, EffectStrength::MEDIUM, validatePerformEffect);
    vibrator->perform_1_1(Effect_1_1::TICK, EffectStrength::STRONG, validatePerformEffect);
}

/*
 * Test to make sure effect values above the valid range are rejected.
 */
TEST_P(VibratorHidlTest_1_1, PerformEffect_1_1_BadEffects_AboveValidRange) {
    Effect_1_1 effect = *std::prev(hidl_enum_range<Effect_1_1>().end());
    Effect_1_1 badEffect = static_cast<Effect_1_1>(static_cast<int32_t>(effect) + 1);
    EXPECT_OK(
            vibrator->perform_1_1(badEffect, EffectStrength::LIGHT, validatePerformEffectBadInput));
}

/*
 * Test to make sure effect values below the valid range are rejected.
 */
TEST_P(VibratorHidlTest_1_1, PerformEffect_1_1_BadEffects_BelowValidRange) {
    Effect_1_1 effect = *hidl_enum_range<Effect_1_1>().begin();
    Effect_1_1 badEffect = static_cast<Effect_1_1>(static_cast<int32_t>(effect) - 1);
    EXPECT_OK(
            vibrator->perform_1_1(badEffect, EffectStrength::LIGHT, validatePerformEffectBadInput));
}

/*
 * Test to make sure strength values above the valid range are rejected.
 */
TEST_P(VibratorHidlTest_1_1, PerformEffect_1_1_BadStrength_AboveValidRange) {
    EffectStrength strength = *std::prev(hidl_enum_range<EffectStrength>().end());
    EffectStrength badStrength = static_cast<EffectStrength>(static_cast<int32_t>(strength) + 1);
    EXPECT_OK(vibrator->perform_1_1(Effect_1_1::CLICK, badStrength, validatePerformEffectBadInput));
}

/*
 * Test to make sure strength values below the valid range are rejected.
 */
TEST_P(VibratorHidlTest_1_1, PerformEffect_1_1_BadStrength_BelowValidRange) {
    EffectStrength strength = *hidl_enum_range<EffectStrength>().begin();
    EffectStrength badStrength = static_cast<EffectStrength>(static_cast<int32_t>(strength) - 1);
    EXPECT_OK(vibrator->perform_1_1(Effect_1_1::CLICK, badStrength, validatePerformEffectBadInput));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VibratorHidlTest_1_1);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, VibratorHidlTest_1_1,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IVibrator::descriptor)),
        android::hardware::PrintInstanceNameToString);
