/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "tv_input_hidl_hal_test"
#include <android-base/logging.h>

#include <android/hardware/tv/input/1.0/types.h>
#include <android/hardware/tv/input/1.0/ITvInput.h>
#include <android/hardware/tv/input/1.0/ITvInputCallback.h>

#include <gtest/gtest.h>

using ::android::hardware::tv::input::V1_0::ITvInput;
using ::android::hardware::tv::input::V1_0::ITvInputCallback;
using ::android::hardware::tv::input::V1_0::Result;
using ::android::hardware::tv::input::V1_0::TvInputType;
using ::android::hardware::tv::input::V1_0::TvInputDeviceInfo;
using ::android::hardware::tv::input::V1_0::TvInputEventType;
using ::android::hardware::tv::input::V1_0::TvInputEvent;
using ::android::hardware::tv::input::V1_0::TvStreamConfig;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;


// Simple ITvInputCallback used as part of testing.
class TvInputCallback : public ITvInputCallback {
  public:
   TvInputCallback() {};

   virtual ~TvInputCallback() = default;

   // notify callback function - currently no-op.
   // TODO: modify it later.
   Return<void> notify(const TvInputEvent& event) override {
     return Void();
   };
};


// The main test class for TV Input HIDL HAL.
class TvInputHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    // currently test passthrough mode only
    tv_input = ITvInput::getService();
    ASSERT_NE(tv_input, nullptr);

    tv_input_callback = new TvInputCallback();
    ASSERT_NE(tv_input_callback, nullptr);
  }

  virtual void TearDown() override {}

  sp<ITvInput> tv_input;
  sp<ITvInputCallback> tv_input_callback;
};


// A class for test environment setup.
class TvInputHidlEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}

 private:
};

// TODO: remove this test and add meaningful tests.
TEST_F(TvInputHidlTest, DummyTest) {
  EXPECT_NE(tv_input, nullptr);
}

int main(int argc, char **argv) {
  ::testing::AddGlobalTestEnvironment(new TvInputHidlEnvironment);
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}

