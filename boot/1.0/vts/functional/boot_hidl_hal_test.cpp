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

using ::android::hardware::boot::V1_0::IBootControl;
using ::android::hardware::boot::V1_0::CommandResult;
using ::android::hardware::boot::V1_0::BoolResult;
using ::android::hardware::boot::V1_0::Slot;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::sp;

// The main test class for the Boot HIDL HAL.
class BootHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    // TODO(b/33385836) Delete copied code
    bool getStub = false;
    char getsubProperty[PROPERTY_VALUE_MAX];
    if (property_get("vts.hidl.get_stub", getsubProperty, "") > 0) {
      if (!strcmp(getsubProperty, "true") || !strcmp(getsubProperty, "True") ||
          !strcmp(getsubProperty, "1")) {
        getStub = true;
      }
    }
    boot = IBootControl::getService("bootctrl", getStub);
    ASSERT_NE(boot, nullptr);
    ASSERT_EQ(!getStub, boot->isRemote());
  }

  virtual void TearDown() override {}

  sp<IBootControl> boot;
};

auto generate_callback(CommandResult *dest) {
  return [=](CommandResult cr) { *dest = cr; };
}

// Sanity check Boot::getNumberSlots().
TEST_F(BootHidlTest, GetNumberSlots) {
  uint32_t slots = boot->getNumberSlots();
  EXPECT_LE((uint32_t)2, slots);
}

// Sanity check Boot::getCurrentSlot().
TEST_F(BootHidlTest, GetCurrentSlot) {
  Slot curSlot = boot->getCurrentSlot();
  uint32_t slots = boot->getNumberSlots();
  EXPECT_LT(curSlot, slots);
}

// Sanity check Boot::markBootSuccessful().
TEST_F(BootHidlTest, MarkBootSuccessful) {
  CommandResult cr;
  Return<void> result = boot->markBootSuccessful(generate_callback(&cr));
  ASSERT_TRUE(result.isOk());
  if (cr.success) {
    Slot curSlot = boot->getCurrentSlot();
    BoolResult ret = boot->isSlotMarkedSuccessful(curSlot);
    EXPECT_EQ(BoolResult::TRUE, ret);
  }
}

// Sanity check Boot::setActiveBootSlot() on good and bad inputs.
TEST_F(BootHidlTest, SetActiveBootSlot) {
  for (Slot s = 0; s < 2; s++) {
    CommandResult cr;
    Return<void> result = boot->setActiveBootSlot(s, generate_callback(&cr));
    EXPECT_TRUE(result.isOk());
  }
  {
    CommandResult cr;
    uint32_t slots = boot->getNumberSlots();
    Return<void> result =
        boot->setActiveBootSlot(slots, generate_callback(&cr));
    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(false, cr.success);
  }
}

// Sanity check Boot::setSlotAsUnbootable() on good and bad inputs.
TEST_F(BootHidlTest, SetSlotAsUnbootable) {
  {
    CommandResult cr;
    Slot curSlot = boot->getCurrentSlot();
    Slot otherSlot = curSlot ? 0 : 1;
    Return<void> result =
        boot->setSlotAsUnbootable(otherSlot, generate_callback(&cr));
    EXPECT_TRUE(result.isOk());
    if (cr.success) {
      EXPECT_EQ(BoolResult::FALSE, boot->isSlotBootable(otherSlot));
      boot->setActiveBootSlot(otherSlot, generate_callback(&cr));
      EXPECT_TRUE(cr.success);
    }
  }
  {
    CommandResult cr;
    uint32_t slots = boot->getNumberSlots();
    Return<void> result =
        boot->setSlotAsUnbootable(slots, generate_callback(&cr));
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(false, cr.success);
  }
}

// Sanity check Boot::isSlotBootable() on good and bad inputs.
TEST_F(BootHidlTest, IsSlotBootable) {
  for (Slot s = 0; s < 2; s++) {
    EXPECT_NE(BoolResult::INVALID_SLOT, boot->isSlotBootable(s));
  }
  uint32_t slots = boot->getNumberSlots();
  EXPECT_EQ(BoolResult::INVALID_SLOT, boot->isSlotBootable(slots));
}

// Sanity check Boot::isSlotMarkedSuccessful() on good and bad inputs.
TEST_F(BootHidlTest, IsSlotMarkedSuccessful) {
  for (Slot s = 0; s < 2; s++) {
    EXPECT_NE(BoolResult::INVALID_SLOT, boot->isSlotMarkedSuccessful(s));
  }
  uint32_t slots = boot->getNumberSlots();
  EXPECT_EQ(BoolResult::INVALID_SLOT, boot->isSlotMarkedSuccessful(slots));
}

// Sanity check Boot::getSuffix() on good and bad inputs.
TEST_F(BootHidlTest, GetSuffix) {
  const char *suffixPtr;
  auto cb = [&](hidl_string suffix) { suffixPtr = suffix.c_str(); };
  for (Slot i = 0; i < 2; i++) {
    CommandResult cr;
    Return<void> result = boot->getSuffix(i, cb);
    EXPECT_TRUE(result.isOk());
    char correctSuffix[3];
    snprintf(correctSuffix, sizeof(correctSuffix), "_%c", 'a' + i);
    ASSERT_EQ(0, strcmp(suffixPtr, correctSuffix));
  }
  {
    char emptySuffix[] = "";
    Return<void> result = boot->getSuffix(boot->getNumberSlots(), cb);
    EXPECT_TRUE(result.isOk());
    ASSERT_EQ(0, strcmp(emptySuffix, suffixPtr));
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;
  return status;
}
