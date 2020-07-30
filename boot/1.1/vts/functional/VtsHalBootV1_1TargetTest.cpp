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

#define LOG_TAG "boot_hidl_hal_test"

#include <vector>

#include <android-base/logging.h>
#include <android/hardware/boot/1.1/IBootControl.h>
#include <android/hardware/boot/1.1/types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <unistd.h>

using ::android::sp;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::boot::V1_1::IBootControl;
using ::android::hardware::boot::V1_1::MergeStatus;
using ::testing::Contains;

class BootHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        boot = IBootControl::getService(GetParam());
        ASSERT_NE(boot, nullptr);

        LOG(INFO) << "Test is remote " << boot->isRemote();
    }

    sp<IBootControl> boot;
};

static std::vector<MergeStatus> ValidMergeStatusValues() {
    std::vector<MergeStatus> values;
    for (const auto value : hidl_enum_range<MergeStatus>()) {
        if (value == MergeStatus::UNKNOWN) {
            continue;
        }
        values.push_back(value);
    }
    return values;
}

/**
 * Ensure merge status can be retrieved.
 */
TEST_P(BootHidlTest, GetSnapshotMergeStatus) {
    auto values = ValidMergeStatusValues();
    auto status = (MergeStatus)boot->getSnapshotMergeStatus();
    EXPECT_THAT(values, Contains(status));
}

/**
 * Ensure merge status can be set to arbitrary value.
 */
TEST_P(BootHidlTest, SetSnapshotMergeStatus) {
    for (const auto value : ValidMergeStatusValues()) {
        EXPECT_TRUE(boot->setSnapshotMergeStatus(value).withDefault(false));
        auto status = boot->getSnapshotMergeStatus();
        if (value == MergeStatus::SNAPSHOTTED) {
            EXPECT_TRUE(status == MergeStatus::SNAPSHOTTED || status == MergeStatus::NONE);
        } else {
            EXPECT_EQ(status, value);
        }
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BootHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, BootHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IBootControl::descriptor)),
        android::hardware::PrintInstanceNameToString);
