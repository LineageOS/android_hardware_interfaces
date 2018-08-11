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

#define LOG_TAG "power_hidl_hal_test"
#include <android-base/logging.h>
#include <android/hardware/power/1.3/IPower.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::power::V1_3::IPower;
using ::android::hardware::power::V1_3::PowerHint;

// Test environment for Power HIDL HAL.
class PowerHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static PowerHidlEnvironment* Instance() {
        static PowerHidlEnvironment* instance = new PowerHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IPower>(); }
};

class PowerHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        power = ::testing::VtsHalHidlTargetTestBase::getService<IPower>(
            PowerHidlEnvironment::Instance()->getServiceName<IPower>());
        ASSERT_NE(power, nullptr);
    }

    sp<IPower> power;
};

TEST_F(PowerHidlTest, PowerHintAsync_1_3) {
    ASSERT_TRUE(power->powerHintAsync_1_3(PowerHint::EXPENSIVE_RENDERING, 0).isOk());
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(PowerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    PowerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
