/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android/hardware/boot/1.2/IBootControl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <unistd.h>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::boot::V1_0::CommandResult;
using ::android::hardware::boot::V1_0::Slot;
using ::android::hardware::boot::V1_2::IBootControl;

class BootHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        boot = IBootControl::getService(GetParam());
        ASSERT_NE(boot, nullptr);

        LOG(INFO) << "Test is remote " << boot->isRemote();
    }

    sp<IBootControl> boot;
};

auto generate_callback(CommandResult* dest) {
    return [=](CommandResult cr) { *dest = cr; };
}

TEST_P(BootHidlTest, GetActiveBootSlot) {
    Slot curSlot = boot->getCurrentSlot();
    Slot otherSlot = curSlot ? 0 : 1;

    // Set the active slot, then check if the getter returns the correct slot.
    CommandResult cr;
    Return<void> result = boot->setActiveBootSlot(otherSlot, generate_callback(&cr));
    EXPECT_TRUE(result.isOk());
    Slot activeSlot = boot->getActiveBootSlot();
    EXPECT_EQ(otherSlot, activeSlot);

    result = boot->setActiveBootSlot(curSlot, generate_callback(&cr));
    EXPECT_TRUE(result.isOk());
    activeSlot = boot->getActiveBootSlot();
    EXPECT_EQ(curSlot, activeSlot);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BootHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, BootHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IBootControl::descriptor)),
        android::hardware::PrintInstanceNameToString);
