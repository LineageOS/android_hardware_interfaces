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

#define LOG_TAG "vibrator_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/vibrator/1.0/types.h>
#include <android/hardware/vibrator/1.4/IVibrator.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <getopt.h>
#include <unistd.h>

#include <future>

using ::android::sp;
using ::android::hardware::hidl_bitfield;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::vibrator::V1_0::EffectStrength;
using ::android::hardware::vibrator::V1_0::Status;
using ::android::hardware::vibrator::V1_3::Effect;
using ::android::hardware::vibrator::V1_4::Capabilities;
using ::android::hardware::vibrator::V1_4::IVibrator;
using ::android::hardware::vibrator::V1_4::IVibratorCallback;

static uint32_t sCompletionLimitMs = UINT32_MAX;

#define EXPECT_OK(ret) ASSERT_TRUE((ret).isOk())

class CompletionCallback : public IVibratorCallback {
  public:
    CompletionCallback(std::function<void()> callback) : mCallback(callback) {}
    Return<void> onComplete() override {
        mCallback();
        return Void();
    }

  private:
    std::function<void()> mCallback;
};

class VibratorHidlTest_1_4 : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        vibrator = IVibrator::getService(GetParam());
        ASSERT_NE(vibrator, nullptr);
        capabilities = vibrator->getCapabilities();
    }

    virtual void TearDown() override {}

    sp<IVibrator> vibrator;
    hidl_bitfield<Capabilities> capabilities;
};

TEST_P(VibratorHidlTest_1_4, OnWithCallback) {
    if (capabilities & Capabilities::ON_COMPLETION_CALLBACK) {
        std::promise<void> completionPromise;
        std::future<void> completionFuture{completionPromise.get_future()};
        sp<CompletionCallback> callback =
                new CompletionCallback([&completionPromise] { completionPromise.set_value(); });
        uint32_t duration = 250;
        std::chrono::milliseconds timeout{duration * 2};
        EXPECT_EQ(Status::OK, vibrator->on_1_4(duration, callback));
        EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
        vibrator->off();
    }
}

static void validatePerformEffectUnsupportedOperation(Status status, uint32_t lengthMs) {
    ASSERT_EQ(Status::UNSUPPORTED_OPERATION, status);
    ASSERT_EQ(static_cast<uint32_t>(0), lengthMs)
            << "Effects that return UNSUPPORTED_OPERATION must have a duration of zero";
}

static void validatePerformEffect(Status status, uint32_t lengthMs) {
    ASSERT_TRUE(status == Status::OK || status == Status::UNSUPPORTED_OPERATION);
    if (status == Status::OK) {
        ASSERT_LT(static_cast<uint32_t>(0), lengthMs)
                << "Effects that return OK must return a positive duration";
    } else {
        validatePerformEffectUnsupportedOperation(status, lengthMs);
    }
}

/*
 * Test to make sure effects within the valid range return are either supported and return OK with
 * a valid duration, or are unsupported and return UNSUPPORTED_OPERATION with a duration of 0.
 */
TEST_P(VibratorHidlTest_1_4, PerformEffect_1_4) {
    Status performStatus;
    uint32_t performLength;
    auto validateWrapper = [&](Status status, uint32_t lengthMs) {
        performStatus = status;
        performLength = lengthMs;
        validatePerformEffect(status, lengthMs);
    };
    for (const auto& effect : hidl_enum_range<Effect>()) {
        for (const auto& strength : hidl_enum_range<EffectStrength>()) {
            std::promise<void> completionPromise;
            std::future<void> completionFuture{completionPromise.get_future()};
            sp<CompletionCallback> callback =
                    new CompletionCallback([&completionPromise] { completionPromise.set_value(); });
            EXPECT_OK(vibrator->perform_1_4(effect, strength, callback, validateWrapper));
            if (performStatus == Status::OK && performLength < sCompletionLimitMs &&
                (capabilities & Capabilities::PERFORM_COMPLETION_CALLBACK)) {
                std::chrono::milliseconds timeout{performLength * 2};
                EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
            }
        }
    }
}

/*
 * Test to make sure effect values above the valid range are rejected.
 */
TEST_P(VibratorHidlTest_1_4, PerformEffect_1_4_BadEffects_AboveValidRange) {
    Effect effect = *std::prev(hidl_enum_range<Effect>().end());
    Effect badEffect = static_cast<Effect>(static_cast<int32_t>(effect) + 1);
    EXPECT_OK(vibrator->perform_1_4(badEffect, EffectStrength::LIGHT, nullptr,
                                    validatePerformEffectUnsupportedOperation));
}

/*
 * Test to make sure effect values below the valid range are rejected.
 */
TEST_P(VibratorHidlTest_1_4, PerformEffect_1_4_BadEffects_BelowValidRange) {
    Effect effect = *hidl_enum_range<Effect>().begin();
    Effect badEffect = static_cast<Effect>(static_cast<int32_t>(effect) - 1);
    EXPECT_OK(vibrator->perform_1_4(badEffect, EffectStrength::LIGHT, nullptr,
                                    validatePerformEffectUnsupportedOperation));
}

/*
 * Test to make sure strength values above the valid range are rejected.
 */
TEST_P(VibratorHidlTest_1_4, PerformEffect_1_4_BadStrength_AboveValidRange) {
    EffectStrength strength = *std::prev(hidl_enum_range<EffectStrength>().end());
    EffectStrength badStrength = static_cast<EffectStrength>(static_cast<int32_t>(strength) + 1);
    EXPECT_OK(vibrator->perform_1_4(Effect::THUD, badStrength, nullptr,
                                    validatePerformEffectUnsupportedOperation));
}

/*
 * Test to make sure strength values below the valid range are rejected.
 */
TEST_P(VibratorHidlTest_1_4, PerformEffect_1_4_BadStrength_BelowValidRange) {
    EffectStrength strength = *hidl_enum_range<EffectStrength>().begin();
    EffectStrength badStrength = static_cast<EffectStrength>(static_cast<int32_t>(strength) - 1);
    EXPECT_OK(vibrator->perform_1_4(Effect::THUD, badStrength, nullptr,
                                    validatePerformEffectUnsupportedOperation));
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, VibratorHidlTest_1_4,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IVibrator::descriptor)),
        android::hardware::PrintInstanceNameToString);

enum {
    OPTION_COMPLETION_LIMIT_MS,
};

int main(int argc, char** argv) {
    struct option options[] = {
            {"completion-limit-ms", required_argument, 0, OPTION_COMPLETION_LIMIT_MS}, {}};

    printf("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);

    while (true) {
        int opt = getopt_long(argc, argv, "", options, nullptr);
        if (opt == -1) {
            break;
        }
        switch (opt) {
            case OPTION_COMPLETION_LIMIT_MS:
                std::istringstream(optarg) >> sCompletionLimitMs;
                break;
            default:
                printf("Unrecognized option\n");
                return -EINVAL;
        }
    }

    return RUN_ALL_TESTS();
}
