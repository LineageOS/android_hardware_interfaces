/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <android-base/logging.h>

#include <cutils/properties.h>

#include <aidl/android/hardware/boot/IBootControl.h>

#include <aidl/Vintf.h>
#include <android/binder_manager.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <unordered_set>

using aidl::android::hardware::boot::IBootControl;
using std::string;
using std::unordered_set;

// The main test class for the Boot HIDL HAL.
class BootAidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        const auto instance_name = GetParam();
        ASSERT_TRUE(AServiceManager_isDeclared(instance_name.c_str()))
                << " instance " << instance_name << " not declared.";
        boot = ::aidl::android::hardware::boot::IBootControl::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(instance_name.c_str())));
        ASSERT_NE(boot, nullptr);
    }

    std::shared_ptr<IBootControl> boot;
};

// validity check Boot::getNumberSlots().
TEST_P(BootAidlTest, GetNumberSlots) {
    int32_t slots{};
    boot->getNumberSlots(&slots);
    ASSERT_LE(2, slots);
}

// validity check Boot::getCurrentSlot().
TEST_P(BootAidlTest, GetCurrentSlot) {
    int curSlot = -1;
    boot->getCurrentSlot(&curSlot);
    int slots = 0;
    boot->getNumberSlots(&slots);
    ASSERT_LT(curSlot, slots);
}

// validity check Boot::markBootSuccessful().
TEST_P(BootAidlTest, MarkBootSuccessful) {
    const auto result = boot->markBootSuccessful();
    ASSERT_TRUE(result.isOk());
    int curSlot = 0;
    boot->getCurrentSlot(&curSlot);
    bool ret = false;
    boot->isSlotMarkedSuccessful(curSlot, &ret);
    ASSERT_TRUE(ret);
}

TEST_P(BootAidlTest, SetActiveBootSlot) {
    int curSlot = -1;
    boot->getCurrentSlot(&curSlot);
    ASSERT_GE(curSlot, 0);
    int otherSlot = curSlot ? 0 : 1;
    bool otherBootable = true;
    boot->isSlotBootable(otherSlot, &otherBootable);

    for (int s = 0; s < 2; s++) {
        const auto result = boot->setActiveBootSlot(s);
        ASSERT_TRUE(result.isOk());
    }
    {
        // Restore original flags to avoid problems on reboot
        auto result = boot->setActiveBootSlot(curSlot);
        ASSERT_TRUE(result.isOk());

        if (!otherBootable) {
            const auto result = boot->setSlotAsUnbootable(otherSlot);
            ASSERT_TRUE(result.isOk());
        }

        result = boot->markBootSuccessful();
        ASSERT_TRUE(result.isOk());
    }
    {
        int slots = 0;
        boot->getNumberSlots(&slots);
        const auto result = boot->setActiveBootSlot(slots);
        ASSERT_FALSE(result.isOk()) << "setActiveBootSlot on invalid slot should fail";
    }
}

TEST_P(BootAidlTest, SetSlotAsUnbootable) {
    int curSlot = -1;
    boot->getCurrentSlot(&curSlot);
    ASSERT_GE(curSlot, 0);
    int otherSlot = curSlot ? 0 : 1;
    bool otherBootable = false;
    boot->isSlotBootable(otherSlot, &otherBootable);
    {
        auto result = boot->setSlotAsUnbootable(otherSlot);
        ASSERT_TRUE(result.isOk());
        boot->isSlotBootable(otherSlot, &otherBootable);
        ASSERT_FALSE(otherBootable);

        // Restore original flags to avoid problems on reboot
        if (otherBootable) {
            result = boot->setActiveBootSlot(otherSlot);
            ASSERT_TRUE(result.isOk());
        }
        result = boot->setActiveBootSlot(curSlot);
        ASSERT_TRUE(result.isOk());
        result = boot->markBootSuccessful();
        ASSERT_TRUE(result.isOk());
    }
    {
        int32_t slots = 0;
        boot->getNumberSlots(&slots);
        const auto result = boot->setSlotAsUnbootable(slots);
        ASSERT_FALSE(result.isOk());
    }
}

// validity check Boot::isSlotBootable() on good and bad inputs.
TEST_P(BootAidlTest, IsSlotBootable) {
    for (int s = 0; s < 2; s++) {
        bool bootable = false;
        const auto res = boot->isSlotBootable(s, &bootable);
        ASSERT_TRUE(res.isOk()) << res.getMessage();
    }
    int32_t slots = 0;
    boot->getNumberSlots(&slots);
    bool bootable = false;
    const auto res = boot->isSlotBootable(slots, &bootable);
    ASSERT_FALSE(res.isOk());
}

// validity check Boot::isSlotMarkedSuccessful() on good and bad inputs.
TEST_P(BootAidlTest, IsSlotMarkedSuccessful) {
    for (int32_t s = 0; s < 2; s++) {
        bool isSuccess = false;
        const auto res = boot->isSlotMarkedSuccessful(s, &isSuccess);
    }
    int32_t slots = 0;
    boot->getNumberSlots(&slots);
    bool isSuccess = false;
    const auto res = boot->isSlotMarkedSuccessful(slots, &isSuccess);
    ASSERT_FALSE(res.isOk());
}

// validity check Boot::getSuffix() on good and bad inputs.
TEST_P(BootAidlTest, GetSuffix) {
    string suffixStr;
    unordered_set<string> suffixes;
    int numSlots = 0;
    boot->getNumberSlots(&numSlots);
    for (int32_t i = 0; i < numSlots; i++) {
        std::string suffix;
        const auto result = boot->getSuffix(i, &suffixStr);
        ASSERT_TRUE(result.isOk());
        ASSERT_EQ('_', suffixStr[0]);
        ASSERT_LE((unsigned)2, suffixStr.size());
        suffixes.insert(suffixStr);
    }
    // All suffixes should be unique
    ASSERT_EQ(numSlots, suffixes.size());
    {
        const string emptySuffix = "";
        const auto result = boot->getSuffix(numSlots, &suffixStr);
        ASSERT_TRUE(result.isOk());
        ASSERT_EQ(suffixStr, emptySuffix);
    }
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, BootAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IBootControl::descriptor)));
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BootAidlTest);
