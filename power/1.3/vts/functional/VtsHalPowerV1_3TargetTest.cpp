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
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::power::V1_3::IPower;
using ::android::hardware::power::V1_3::PowerHint;

class PowerHidlTest : public testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        power = IPower::getService(GetParam());
        ASSERT_NE(power, nullptr);
    }

    sp<IPower> power;
};

TEST_P(PowerHidlTest, PowerHintAsync_1_3) {
    ASSERT_TRUE(power->powerHintAsync_1_3(PowerHint::EXPENSIVE_RENDERING, 0).isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PowerHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, PowerHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IPower::descriptor)),
        android::hardware::PrintInstanceNameToString);
