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

#define LOG_TAG "boot_hidl_hal_test"
#include <android-base/logging.h>

#include <cutils/properties.h>

#include <android/hardware/boot/1.0/IBootControl.h>

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <unordered_set>

using ::android::hardware::boot::V1_0::IBootControl;
using ::android::hardware::boot::V1_0::CommandResult;
using ::android::hardware::boot::V1_0::BoolResult;
using ::android::hardware::boot::V1_0::Slot;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::sp;
using std::string;
using std::unordered_set;
using std::vector;

// The main test class for the Boot HIDL HAL.
class BootHidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        boot = IBootControl::getService(GetParam());
        ASSERT_NE(boot, nullptr);
    }

    virtual void TearDown() override {}

    sp<IBootControl> boot;
};

auto generate_callback(CommandResult *dest) {
  return [=](CommandResult cr) { *dest = cr; };
}

// Sanity check Boot::getNumberSlots().
TEST_P(BootHidlTest, GetNumberSlots) {
    uint32_t slots = boot->getNumberSlots();
    EXPECT_LE((uint32_t)2, slots);
}

// Sanity check Boot::getCurrentSlot().
TEST_P(BootHidlTest, GetCurrentSlot) {
    Slot curSlot = boot->getCurrentSlot();
    uint32_t slots = boot->getNumberSlots();
    EXPECT_LT(curSlot, slots);
}

// Sanity check Boot::markBootSuccessful().
TEST_P(BootHidlTest, MarkBootSuccessful) {
    CommandResult cr;
    Return<void> result = boot->markBootSuccessful(generate_callback(&cr));
    ASSERT_TRUE(result.isOk());
    if (cr.success) {
        Slot curSlot = boot->getCurrentSlot();
        BoolResult ret = boot->isSlotMarkedSuccessful(curSlot);
        EXPECT_EQ(BoolResult::TRUE, ret);
    }
}

TEST_P(BootHidlTest, SetActiveBootSlot) {
    Slot curSlot = boot->getCurrentSlot();
    Slot otherSlot = curSlot ? 0 : 1;
    auto otherBootable = boot->isSlotBootable(otherSlot);

    for (Slot s = 0; s < 2; s++) {
        CommandResult cr;
        Return<void> result = boot->setActiveBootSlot(s, generate_callback(&cr));
        EXPECT_TRUE(result.isOk());
    }
    {
        // Restore original flags to avoid problems on reboot
        CommandResult cr;
        auto result = boot->setActiveBootSlot(curSlot, generate_callback(&cr));
        EXPECT_TRUE(result.isOk());
        EXPECT_TRUE(cr.success);

        if (otherBootable == BoolResult::FALSE) {
            result = boot->setSlotAsUnbootable(otherSlot, generate_callback(&cr));
            EXPECT_TRUE(result.isOk());
            EXPECT_TRUE(cr.success);
        }

        result = boot->markBootSuccessful(generate_callback(&cr));
        EXPECT_TRUE(result.isOk());
        EXPECT_TRUE(cr.success);
    }
    {
        CommandResult cr;
        uint32_t slots = boot->getNumberSlots();
        Return<void> result = boot->setActiveBootSlot(slots, generate_callback(&cr));
        ASSERT_TRUE(result.isOk());
        EXPECT_EQ(false, cr.success);
    }
}

TEST_P(BootHidlTest, SetSlotAsUnbootable) {
    Slot curSlot = boot->getCurrentSlot();
    Slot otherSlot = curSlot ? 0 : 1;
    auto otherBootable = boot->isSlotBootable(otherSlot);
    {
        CommandResult cr;
        Return<void> result = boot->setSlotAsUnbootable(otherSlot, generate_callback(&cr));
        EXPECT_TRUE(result.isOk());
        if (cr.success) {
            EXPECT_EQ(BoolResult::FALSE, boot->isSlotBootable(otherSlot));

            // Restore original flags to avoid problems on reboot
            if (otherBootable == BoolResult::TRUE) {
                result = boot->setActiveBootSlot(otherSlot, generate_callback(&cr));
                EXPECT_TRUE(result.isOk());
                EXPECT_TRUE(cr.success);
            }
            result = boot->setActiveBootSlot(curSlot, generate_callback(&cr));
            EXPECT_TRUE(result.isOk());
            EXPECT_TRUE(cr.success);
            result = boot->markBootSuccessful(generate_callback(&cr));
            EXPECT_TRUE(result.isOk());
            EXPECT_TRUE(cr.success);
        }
    }
    {
        CommandResult cr;
        uint32_t slots = boot->getNumberSlots();
        Return<void> result = boot->setSlotAsUnbootable(slots, generate_callback(&cr));
        EXPECT_TRUE(result.isOk());
        EXPECT_EQ(false, cr.success);
    }
}

// Sanity check Boot::isSlotBootable() on good and bad inputs.
TEST_P(BootHidlTest, IsSlotBootable) {
    for (Slot s = 0; s < 2; s++) {
        EXPECT_NE(BoolResult::INVALID_SLOT, boot->isSlotBootable(s));
    }
    uint32_t slots = boot->getNumberSlots();
    EXPECT_EQ(BoolResult::INVALID_SLOT, boot->isSlotBootable(slots));
}

// Sanity check Boot::isSlotMarkedSuccessful() on good and bad inputs.
TEST_P(BootHidlTest, IsSlotMarkedSuccessful) {
    for (Slot s = 0; s < 2; s++) {
        EXPECT_NE(BoolResult::INVALID_SLOT, boot->isSlotMarkedSuccessful(s));
    }
    uint32_t slots = boot->getNumberSlots();
    EXPECT_EQ(BoolResult::INVALID_SLOT, boot->isSlotMarkedSuccessful(slots));
}

// Sanity check Boot::getSuffix() on good and bad inputs.
TEST_P(BootHidlTest, GetSuffix) {
    string suffixStr;
    unordered_set<string> suffixes;
    auto cb = [&](hidl_string suffix) { suffixStr = suffix.c_str(); };
    for (Slot i = 0; i < boot->getNumberSlots(); i++) {
        CommandResult cr;
        Return<void> result = boot->getSuffix(i, cb);
        EXPECT_TRUE(result.isOk());
        ASSERT_EQ('_', suffixStr[0]);
        ASSERT_LE((unsigned)2, suffixStr.size());
        suffixes.insert(suffixStr);
    }
    // All suffixes should be unique
    ASSERT_EQ(boot->getNumberSlots(), suffixes.size());
    {
        string emptySuffix = "";
        Return<void> result = boot->getSuffix(boot->getNumberSlots(), cb);
        EXPECT_TRUE(result.isOk());
        ASSERT_EQ(0, suffixStr.compare(emptySuffix));
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BootHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, BootHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IBootControl::descriptor)),
        android::hardware::PrintInstanceNameToString);
