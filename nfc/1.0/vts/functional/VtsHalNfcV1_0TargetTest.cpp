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

#include <android/hardware/nfc/1.0/INfc.h>
#include <android/hardware/nfc/1.0/INfcClientCallback.h>
#include <android/hardware/nfc/1.0/types.h>
#include <hardware/nfc.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>

using ::android::hardware::nfc::V1_0::INfc;
using ::android::hardware::nfc::V1_0::INfcClientCallback;
using ::android::hardware::nfc::V1_0::NfcEvent;
using ::android::hardware::nfc::V1_0::NfcStatus;
using ::android::hardware::nfc::V1_0::NfcData;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::sp;

/* NCI Commands */
#define CORE_RESET_CMD \
  { 0x20, 0x00, 0x01, 0x00 }
#define CORE_RESET_CMD_CONFIG_RESET \
  { 0x20, 0x00, 0x01, 0x01 }
#define CORE_CONN_CREATE_CMD \
  { 0x20, 0x04, 0x02, 0x01, 0x00 }
#define INVALID_COMMAND \
  { 0x20, 0x00, 0x00 }
#define FAULTY_DATA_PACKET \
  { 0x00, 0x00, 0xFF }

#define LOOP_BACK_HEADER_SIZE 3
#define SYNTAX_ERROR 5
#define NUMBER_LOOPS 3922
#define VERSION 0x11
#define TIMEOUT_PERIOD 5

constexpr char kCallbackNameSendEvent[] = "sendEvent";
constexpr char kCallbackNameSendData[] = "sendData";

class NfcClientCallbackArgs {
   public:
    NfcEvent last_event_;
    NfcStatus last_status_;
    NfcData last_data_;
};

/* Callback class for data & Event. */
class NfcClientCallback
    : public ::testing::VtsHalHidlTargetCallbackBase<NfcClientCallbackArgs>,
      public INfcClientCallback {
   public:
    virtual ~NfcClientCallback() = default;

    /* sendEvent callback function - Records the Event & Status
     * and notifies the TEST
     **/
    Return<void> sendEvent(NfcEvent event, NfcStatus event_status) override {
        NfcClientCallbackArgs args;
        args.last_event_ = event;
        args.last_status_ = event_status;
        NotifyFromCallback(kCallbackNameSendEvent, args);
        return Void();
    };

    /* sendData callback function. Records the data and notifies the TEST*/
    Return<void> sendData(const NfcData& data) override {
        NfcClientCallbackArgs args;
        args.last_data_ = data;
        NotifyFromCallback(kCallbackNameSendData, args);
        return Void();
    };
};

// The main test class for NFC HIDL HAL.
class NfcHidlTest : public ::testing::VtsHalHidlTargetTestBase {
 public:
  virtual void SetUp() override {
    nfc_ = ::testing::VtsHalHidlTargetTestBase::getService<INfc>();
    ASSERT_NE(nfc_, nullptr);

    nfc_cb_ = new NfcClientCallback();
    ASSERT_NE(nfc_cb_, nullptr);

    EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
    // Wait for OPEN_CPLT event
    auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
    EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
  }

  virtual void TearDown() override {
    EXPECT_EQ(NfcStatus::OK, nfc_->close());
    // Wait for CLOSE_CPLT event
    auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(NfcEvent::CLOSE_CPLT, res.args->last_event_);
    EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
  }

  sp<INfc> nfc_;
  sp<NfcClientCallback> nfc_cb_;
};

// A class for test environment setup (kept since this file is a template).
class NfcHidlEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}

 private:
};

/*
 * OpenAndClose:
 * Makes an open call, waits for NfcEvent.OPEN_CPLT
 * Immediately calls close() and waits for NfcEvent.CLOSE_CPLT
 * Since open and close calls are a part of SetUp() and TearDown(),
 * the function definition is intentionally kept empty
 */
TEST_F(NfcHidlTest, OpenAndClose) {}

/*
 * WriteCoreReset:
 * Sends CORE_RESET_CMD
 * Waits for CORE_RESET_RSP
 * Checks the status, version number and configuration status
 */
TEST_F(NfcHidlTest, WriteCoreReset) {
  std::vector<uint8_t> cmd = CORE_RESET_CMD;
  NfcData data = cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(6ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  EXPECT_GE(VERSION, res.args->last_data_[4]);
  EXPECT_EQ(0ul, res.args->last_data_[5]);
}

/*
 * WriteCoreResetConfigReset:
 * Sends CORE_RESET_CMD_CONFIG_RESET
 * Waits for CORE_RESET_RSP
 * Checks the status, version number and configuration status
 */
TEST_F(NfcHidlTest, WriteCoreResetConfigReset) {
  std::vector<uint8_t> cmd = CORE_RESET_CMD_CONFIG_RESET;
  NfcData data = cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_RESET_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(6ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  EXPECT_GE(VERSION, res.args->last_data_[4]);
  EXPECT_EQ(1ul, res.args->last_data_[5]);
}

/*
 * WriteInvalidCommand:
 * Sends an invalid command
 * Waits for response
 * Checks SYNTAX_ERROR status
 */
TEST_F(NfcHidlTest, WriteInvalidCommand) {
  // Send an Error Command
  std::vector<uint8_t> cmd = INVALID_COMMAND;
  NfcData data = cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(4ul, res.args->last_data_.size());
  EXPECT_EQ(SYNTAX_ERROR, res.args->last_data_[3]);
}

/*
 * WriteInvalidAndThenValidCommand:
 * Sends an Faulty Data Packet
 * Waits for CORE_INTERFACE_ERROR_NTF
 * Checks SYNTAX_ERROR status
 * Repeat for 100 times appending 0xFF each time to the packet
 * Send CORE_CONN_CREATE_CMD for loop-back mode
 * Check the response
 */
TEST_F(NfcHidlTest, WriteInvalidAndThenValidCommand) {
  // Send an Error Data Packet
  std::vector<uint8_t> cmd = FAULTY_DATA_PACKET;
  NfcData data = cmd;
  size_t size = data.size();

  for (int i = 0; i < 100; i++) {
    data.resize(++size);
    data[size - 1] = 0xFF;
    EXPECT_EQ(data.size(), nfc_->write(data));
    // Wait for CORE_INTERFACE_ERROR_NTF
    auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
    EXPECT_TRUE(res.no_timeout);
    EXPECT_EQ(5ul, res.args->last_data_.size());
    EXPECT_EQ(0x60, res.args->last_data_[0]);
    EXPECT_EQ(0x08, res.args->last_data_[1]);
    EXPECT_EQ(0x02, res.args->last_data_[2]);
    EXPECT_EQ(SYNTAX_ERROR, res.args->last_data_[3]);
  }

  cmd = CORE_CONN_CREATE_CMD;
  data = cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_CONN_CREATE_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(7ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
}
/*
 * Bandwidth:
 * Sets the loop-back mode using CORE_CONN_CREATE_CMD
 * Sends max payload size data
 * Waits for the response
 * Checks the data received
 * Repeat to send total of 1Mb data
 */
TEST_F(NfcHidlTest, Bandwidth) {
  std::vector<uint8_t> cmd = CORE_CONN_CREATE_CMD;
  NfcData data = cmd;
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_CONN_CREATE_RSP
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendData);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(7ul, res.args->last_data_.size());
  EXPECT_EQ((int)NfcStatus::OK, res.args->last_data_[3]);
  uint8_t conn_id = res.args->last_data_[6];
  uint32_t max_payload_size = res.args->last_data_[4];

  for (int loops = 0; loops < NUMBER_LOOPS; loops++) {
      res.args->last_data_.resize(0);
      data.resize(max_payload_size + LOOP_BACK_HEADER_SIZE);
      data[0] = conn_id;
      data[1] = 0x00;
      data[2] = max_payload_size;
      for (uint32_t i = 0; i < max_payload_size; i++) {
          data[i + LOOP_BACK_HEADER_SIZE] = i;
    }
    EXPECT_EQ(max_payload_size + LOOP_BACK_HEADER_SIZE, nfc_->write(data));
    // Wait for data and CORE_CONN_CREDITS_NTF
    auto res1 = nfc_cb_->WaitForCallback(kCallbackNameSendData);
    EXPECT_TRUE(res1.no_timeout);
    auto res2 = nfc_cb_->WaitForCallback(kCallbackNameSendData);
    EXPECT_TRUE(res2.no_timeout);
    // Check if the same data was received back
    EXPECT_TRUE(res1.args);
    EXPECT_TRUE(res2.args);

    NfcData credits_ntf = res1.args->last_data_;
    NfcData received_data = res2.args->last_data_;
    /* It is possible that CORE_CONN_CREDITS_NTF is received before data,
     * Find the order and do further checks depending on that */
    if (received_data.size() != data.size()) {
        credits_ntf = res2.args->last_data_;
        received_data = res1.args->last_data_;
    }
    EXPECT_EQ(data.size(), received_data.size());
    for (size_t i = 0; i < data.size(); i++) {
        EXPECT_EQ(data[i], received_data[i]);
    }

    EXPECT_EQ(6ul, credits_ntf.size());
    // Check if the credit is refilled to 1
    EXPECT_EQ(1, credits_ntf[5]);
  }
}

/*
 * PowerCycle:
 * Calls powerCycle()
 * Waits for NfcEvent.OPEN_CPLT
 * Checks status
 */
TEST_F(NfcHidlTest, PowerCycle) {
  EXPECT_EQ(NfcStatus::OK, nfc_->powerCycle());
  // Wait for NfcEvent.OPEN_CPLT
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
  EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
}

/*
 * PowerCycleAfterClose:
 * Calls powerCycle() after close()
 * Checks status
 */
TEST_F(NfcHidlTest, PowerCycleAfterClose) {
  EXPECT_EQ(NfcStatus::OK, nfc_->close());
  // Wait for CLOSE_CPLT event
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::CLOSE_CPLT, res.args->last_event_);
  EXPECT_EQ(NfcStatus::OK, res.args->last_status_);

  EXPECT_EQ(NfcStatus::FAILED, nfc_->powerCycle());

  EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
  // Wait for OPEN_CPLT event
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
  EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
}

/*
 * CoreInitialized:
 * Calls coreInitialized() with different data
 * Waits for NfcEvent.POST_INIT_CPLT
 */
TEST_F(NfcHidlTest, CoreInitialized) {
  NfcData data;
  data.resize(1);
  NfcEvent last_event_;
  for (int i = 0; i <= 6; i++) {
      data[0] = i;
      EXPECT_EQ(NfcStatus::OK, nfc_->coreInitialized(data));
      // Wait for NfcEvent.POST_INIT_CPLT
      auto res = nfc_cb_->WaitForCallbackAny();
      if (res.name.compare(kCallbackNameSendEvent) == 0) {
          last_event_ = res.args->last_event_;
      }
      EXPECT_TRUE(res.no_timeout);
      EXPECT_EQ(NfcEvent::POST_INIT_CPLT, last_event_);
  }
}

/*
 * ControlGranted:
 * Calls controlGranted()
 * Checks the return value
 */
TEST_F(NfcHidlTest, ControlGranted) {
  EXPECT_EQ(NfcStatus::OK, nfc_->controlGranted());
}

/*
 * ControlGrantedAfterClose:
 * Call controlGranted() after close
 * Checks the return value
 */
TEST_F(NfcHidlTest, ControlGrantedAfterClose) {
  EXPECT_EQ(NfcStatus::OK, nfc_->close());
  // Wait for CLOSE_CPLT event
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::CLOSE_CPLT, res.args->last_event_);
  EXPECT_EQ(NfcStatus::OK, res.args->last_status_);

  EXPECT_EQ(NfcStatus::OK, nfc_->controlGranted());

  EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
  // Wait for OPEN_CPLT event
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
  EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
}

/* PreDiscover:
 * Calls prediscover()
 * Checks the return value
 */
TEST_F(NfcHidlTest, PreDiscover) {
  EXPECT_EQ(NfcStatus::OK, nfc_->prediscover());
}

/*
 * PreDiscoverAfterClose:
 * Call prediscover() after close
 * Checks the return value
 */
TEST_F(NfcHidlTest, PreDiscoverAfterClose) {
  EXPECT_EQ(NfcStatus::OK, nfc_->close());
  // Wait for CLOSE_CPLT event
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::CLOSE_CPLT, res.args->last_event_);
  EXPECT_EQ(NfcStatus::OK, res.args->last_status_);

  EXPECT_EQ(NfcStatus::OK, nfc_->prediscover());

  EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
  // Wait for OPEN_CPLT event
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
  EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
}

/*
 * CloseAfterClose:
 * Calls close() multiple times
 * Checks status
 */
TEST_F(NfcHidlTest, CloseAfterClose) {
  EXPECT_EQ(NfcStatus::OK, nfc_->close());
  // Wait for CLOSE_CPLT event
  auto res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::CLOSE_CPLT, res.args->last_event_);
  EXPECT_EQ(NfcStatus::OK, res.args->last_status_);

  EXPECT_EQ(NfcStatus::FAILED, nfc_->close());

  EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
  // Wait for OPEN_CPLT event
  res = nfc_cb_->WaitForCallback(kCallbackNameSendEvent);
  EXPECT_TRUE(res.no_timeout);
  EXPECT_EQ(NfcEvent::OPEN_CPLT, res.args->last_event_);
  EXPECT_EQ(NfcStatus::OK, res.args->last_status_);
}

/*
 * OpenAfterOpen:
 * Calls open() multiple times
 * Checks status
 */
TEST_F(NfcHidlTest, OpenAfterOpen) {
  EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
  EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
}

int main(int argc, char** argv) {
  ::testing::AddGlobalTestEnvironment(new NfcHidlEnvironment);
  ::testing::InitGoogleTest(&argc, argv);

  std::system("svc nfc disable"); /* Turn off NFC */
  sleep(5);

  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;

  std::system("svc nfc enable"); /* Turn on NFC */
  sleep(5);

  return status;
}
