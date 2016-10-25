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

#define LOG_TAG "nfc_hidl_hal_test"
#include <android-base/logging.h>

#include <hardware/nfc.h>
#include <android/hardware/nfc/1.0/types.h>
#include <android/hardware/nfc/1.0/INfc.h>
#include <android/hardware/nfc/1.0/INfcClientCallback.h>

#include <gtest/gtest.h>

using ::android::hardware::nfc::V1_0::INfc;
using ::android::hardware::nfc::V1_0::INfcClientCallback;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

#define NFC_NCI_SERVICE_NAME "nfc_nci"


// Simple NfcClientCallback used as part of testing.
class NfcClientCallback : public INfcClientCallback {
  public:
   NfcClientCallback() {};

   virtual ~NfcClientCallback() = default;

   // sendEvent callback function - currently no-op.
   Return<void> sendEvent(
           ::android::hardware::nfc::V1_0::NfcEvent event,
           ::android::hardware::nfc::V1_0::NfcStatus event_status) override {
     return Void();
   };

   // sendData callback function - currently no-op.
   Return<void> sendData(const ::android::hardware::nfc::V1_0::NfcData &data ) override {
     ::android::hardware::nfc::V1_0::NfcData copy = data;
     return Void();
   };
};


// The main test class for NFC HIDL HAL.
class NfcHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    // currently test passthrough mode only
    nfc = INfc::getService(NFC_NCI_SERVICE_NAME, true);
    ASSERT_NE(nfc, nullptr);

    nfc_cb = new NfcClientCallback();
    ASSERT_NE(nfc_cb, nullptr);
  }

  virtual void TearDown() override {}

  sp<INfc> nfc;
  sp<INfcClientCallback> nfc_cb;
};


// A class for test environment setup (kept since this file is a template).
class NfcHidlEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}

 private:
};

TEST_F(NfcHidlTest, OpenAndClose) {
  EXPECT_EQ(0, (int)nfc->open(nfc_cb));
  EXPECT_EQ(0, (int)nfc->close());
}

int main(int argc, char **argv) {
  ::testing::AddGlobalTestEnvironment(new NfcHidlEnvironment);
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
