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

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/vibrator/1.0/types.h>
#include <android/hardware/vibrator/1.3/IVibrator.h>
#include <unistd.h>

using ::android::sp;
using ::android::hardware::vibrator::V1_0::Status;
using ::android::hardware::vibrator::V1_3::IVibrator;

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

// The main test class for VIBRATOR HIDL HAL 1.3.
class VibratorHidlTest_1_3 : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        vibrator = ::testing::VtsHalHidlTargetTestBase::getService<IVibrator>(
            VibratorHidlEnvironment::Instance()->getServiceName<IVibrator>());
        ASSERT_NE(vibrator, nullptr);
    }

    virtual void TearDown() override {}

    sp<IVibrator> vibrator;
};

TEST_F(VibratorHidlTest_1_3, ChangeVibrationalExternalControl) {
    if (vibrator->supportsExternalControl()) {
        EXPECT_EQ(Status::OK, vibrator->setExternalControl(true));
        sleep(1);
        EXPECT_EQ(Status::OK, vibrator->setExternalControl(false));
        sleep(1);
    }
}

TEST_F(VibratorHidlTest_1_3, SetExternalControlReturnUnsupportedOperationIfNotSupported) {
    if (!vibrator->supportsExternalControl()) {
        EXPECT_EQ(Status::UNSUPPORTED_OPERATION, vibrator->setExternalControl(true));
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
