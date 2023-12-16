/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/bluetooth/finder/IBluetoothFinder.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

#include <array>
#include <vector>

using ::aidl::android::hardware::bluetooth::finder::Eid;
using ::aidl::android::hardware::bluetooth::finder::IBluetoothFinder;
using ::ndk::ScopedAStatus;

class BluetoothFinderTest : public ::testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    ALOGI("SetUp Finder Test");
    bluetooth_finder = IBluetoothFinder::fromBinder(
        ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(bluetooth_finder, nullptr);
  }

  virtual void TearDown() override {
    ALOGI("TearDown Finder Test");
    bluetooth_finder = nullptr;
    ASSERT_EQ(bluetooth_finder, nullptr);
  }

  ScopedAStatus sendEids(uint8_t num);
  ScopedAStatus setPoweredOffFinderMode(bool enable);
  ScopedAStatus getPoweredOffFinderMode(bool* status);

 private:
  std::shared_ptr<IBluetoothFinder> bluetooth_finder;
};

ScopedAStatus BluetoothFinderTest::sendEids(uint8_t numKeys) {
  std::vector<Eid> keys(numKeys);
  for (uint_t i = 0; i < numKeys; i++) {
    std::array<uint8_t, 20> key;
    key.fill(i + 1);
    keys[i].bytes = key;
  }
  return bluetooth_finder->sendEids(keys);
}

ScopedAStatus BluetoothFinderTest::setPoweredOffFinderMode(bool enable) {
  return bluetooth_finder->setPoweredOffFinderMode(enable);
}

ScopedAStatus BluetoothFinderTest::getPoweredOffFinderMode(bool* status) {
  return bluetooth_finder->getPoweredOffFinderMode(status);
}

TEST_P(BluetoothFinderTest, SendEidsSingle) {
  ScopedAStatus status = sendEids(1);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothFinderTest, Send255Eids) {
  ScopedAStatus status = sendEids(255);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothFinderTest, setAndGetPoweredOffFinderModeEnable) {
  ScopedAStatus status = setPoweredOffFinderMode(true);
  ASSERT_TRUE(status.isOk());
  bool pof_status;
  status = getPoweredOffFinderMode(&pof_status);
  ASSERT_TRUE(status.isOk());
  ASSERT_TRUE(pof_status);
}

TEST_P(BluetoothFinderTest, setAndGetPoweredOffFinderModeDisable) {
  ScopedAStatus status = setPoweredOffFinderMode(false);
  ASSERT_TRUE(status.isOk());
  bool pof_status;
  status = getPoweredOffFinderMode(&pof_status);
  ASSERT_TRUE(status.isOk());
  ASSERT_TRUE(!pof_status);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BluetoothFinderTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, BluetoothFinderTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothFinder::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ABinderProcess_startThreadPool();
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
