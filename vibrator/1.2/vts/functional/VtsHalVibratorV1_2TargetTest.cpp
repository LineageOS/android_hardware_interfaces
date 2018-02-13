/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/vibrator/1.0/types.h>
#include <android/hardware/vibrator/1.2/IVibrator.h>
#include <android/hardware/vibrator/1.2/types.h>
#include <unistd.h>

using ::android::hardware::vibrator::V1_0::Status;
using ::android::hardware::vibrator::V1_0::EffectStrength;
using ::android::hardware::vibrator::V1_2::Effect;
using ::android::hardware::vibrator::V1_2::IVibrator;
using ::android::hardware::hidl_enum_iterator;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

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

// The main test class for VIBRATOR HIDL HAL 1.2.
class VibratorHidlTest_1_2 : public ::testing::VtsHalHidlTargetTestBase {
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
        ASSERT_GT(lengthMs, static_cast<uint32_t>(0))
            << "Effects that return OK must return a non-zero duration";
    } else {
        ASSERT_EQ(lengthMs, static_cast<uint32_t>(0))
            << "Effects that return UNSUPPORTED_OPERATION must have a duration of zero";
    }
}

TEST_F(VibratorHidlTest_1_2, PerformEffect_1_2) {
    for (const auto& effect : hidl_enum_iterator<Effect>()) {
        for (const auto& strength : hidl_enum_iterator<EffectStrength>()) {
            vibrator->perform_1_2(effect, strength, validatePerformEffect);
        }
    }
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(VibratorHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    VibratorHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
