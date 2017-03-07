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

#include <VtsHalHidlTargetBaseTest.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

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

// The main test class for NFC HIDL HAL.
class NfcHidlTest : public ::testing::VtsHalHidlTargetBaseTest {
 public:
  virtual void SetUp() override {
    nfc_ = ::testing::VtsHalHidlTargetBaseTest::getService<INfc>();
    ASSERT_NE(nfc_, nullptr);

    nfc_cb_ = new NfcClientCallback(*this);
    ASSERT_NE(nfc_cb_, nullptr);

    count = 0;
    last_event_ = NfcEvent::ERROR;
    last_status_ = NfcStatus::FAILED;

    EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
    // Wait for OPEN_CPLT event
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(NfcEvent::OPEN_CPLT, last_event_);
    EXPECT_EQ(NfcStatus::OK, last_status_);
  }

  virtual void TearDown() override {
    EXPECT_EQ(NfcStatus::OK, nfc_->close());
    // Wait for CLOSE_CPLT event
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(NfcEvent::CLOSE_CPLT, last_event_);
    EXPECT_EQ(NfcStatus::OK, last_status_);
  }

  /* Used as a mechanism to inform the test about data/event callback */
  inline void notify() {
    std::unique_lock<std::mutex> lock(mtx);
    count++;
    cv.notify_one();
  }

  /* Test code calls this function to wait for data/event callback */
  inline std::cv_status wait() {
    std::unique_lock<std::mutex> lock(mtx);

    std::cv_status status = std::cv_status::no_timeout;
    auto now = std::chrono::system_clock::now();
    while (count == 0) {
      status = cv.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
      if (status == std::cv_status::timeout) return status;
    }
    count--;
    return status;
  }

  /* Callback class for data & Event. */
  class NfcClientCallback : public INfcClientCallback {
    NfcHidlTest& parent_;

   public:
    NfcClientCallback(NfcHidlTest& parent) : parent_(parent){};

    virtual ~NfcClientCallback() = default;

    /* sendEvent callback function - Records the Event & Status
     * and notifies the TEST
     **/
    Return<void> sendEvent(NfcEvent event, NfcStatus event_status) override {
      parent_.last_event_ = event;
      parent_.last_status_ = event_status;
      parent_.notify();
      return Void();
    };

    /* sendData callback function. Records the data and notifies the TEST*/
    Return<void> sendData(const NfcData& data) override {
      size_t size = parent_.last_data_.size();
      parent_.last_data_.resize(size + 1);
      parent_.last_data_[size] = data;
      parent_.notify();
      return Void();
    };
  };

  sp<INfc> nfc_;
  sp<INfcClientCallback> nfc_cb_;
  NfcEvent last_event_;
  NfcStatus last_status_;
  hidl_vec<NfcData> last_data_;

 private:
  std::mutex mtx;
  std::condition_variable cv;
  int count;
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
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(1ul, last_data_.size());
  EXPECT_EQ(6ul, last_data_[0].size());
  EXPECT_EQ((int)NfcStatus::OK, last_data_[0][3]);
  EXPECT_GE(VERSION, last_data_[0][4]);
  EXPECT_EQ(0ul, last_data_[0][5]);
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
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(1ul, last_data_.size());
  EXPECT_EQ(6ul, last_data_[0].size());
  EXPECT_EQ((int)NfcStatus::OK, last_data_[0][3]);
  EXPECT_GE(VERSION, last_data_[0][4]);
  EXPECT_EQ(1ul, last_data_[0][5]);
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
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(1ul, last_data_.size());
  EXPECT_EQ(4ul, last_data_[0].size());
  EXPECT_EQ(SYNTAX_ERROR, last_data_[0][3]);
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
    last_data_.resize(0);
    data.resize(++size);
    data[size - 1] = 0xFF;
    EXPECT_EQ(data.size(), nfc_->write(data));
    // Wait for CORE_INTERFACE_ERROR_NTF
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(1ul, last_data_.size());
    EXPECT_EQ(5ul, last_data_[0].size());
    EXPECT_EQ(0x60, last_data_[0][0]);
    EXPECT_EQ(0x08, last_data_[0][1]);
    EXPECT_EQ(0x02, last_data_[0][2]);
    EXPECT_EQ(SYNTAX_ERROR, last_data_[0][3]);
  }

  cmd = CORE_CONN_CREATE_CMD;
  data = cmd;
  last_data_.resize(0);
  EXPECT_EQ(data.size(), nfc_->write(data));
  // Wait for CORE_CONN_CREATE_RSP
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(1ul, last_data_.size());
  EXPECT_EQ(7ul, last_data_[0].size());
  EXPECT_EQ((int)NfcStatus::OK, last_data_[0][3]);
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
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(1ul, last_data_.size());
  EXPECT_EQ(7ul, last_data_[0].size());
  EXPECT_EQ((int)NfcStatus::OK, last_data_[0][3]);
  uint8_t conn_id = last_data_[0][6];
  uint32_t max_payload_size = last_data_[0][4];

  for (int loops = 0; loops < NUMBER_LOOPS; loops++) {
    last_data_.resize(0);
    data.resize(max_payload_size + LOOP_BACK_HEADER_SIZE);
    data[0] = conn_id;
    data[1] = 0x00;
    data[2] = max_payload_size;
    for (uint32_t i = 0; i < max_payload_size; i++) {
      data[i + LOOP_BACK_HEADER_SIZE] = i;
    }
    EXPECT_EQ(max_payload_size + LOOP_BACK_HEADER_SIZE, nfc_->write(data));
    // Wait for data and CORE_CONN_CREDITS_NTF
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    // Check if the same data was recieved back
    EXPECT_EQ(2ul, last_data_.size());

    /* It is possible that CORE_CONN_CREDITS_NTF is received before data,
     * Find the order and do further checks depending on that */
    uint8_t data_index = last_data_[0].size() == data.size() ? 0 : 1;
    EXPECT_EQ(data.size(), last_data_[data_index].size());
    for (size_t i = 0; i < data.size(); i++) {
      EXPECT_EQ(data[i], last_data_[data_index][i]);
    }

    EXPECT_EQ(6ul, last_data_[!data_index].size());
    // Check if the credit is refilled to 1
    EXPECT_EQ(1, last_data_[!data_index][5]);
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
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(NfcEvent::OPEN_CPLT, last_event_);
  EXPECT_EQ(NfcStatus::OK, last_status_);
}

/*
 * PowerCycleAfterClose:
 * Calls powerCycle() after close()
 * Checks status
 */
TEST_F(NfcHidlTest, PowerCycleAfterClose) {
  EXPECT_EQ(NfcStatus::OK, nfc_->close());
  // Wait for CLOSE_CPLT event
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(NfcEvent::CLOSE_CPLT, last_event_);
  EXPECT_EQ(NfcStatus::OK, last_status_);

  EXPECT_EQ(NfcStatus::FAILED, nfc_->powerCycle());

  EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
  // Wait for OPEN_CPLT event
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(NfcEvent::OPEN_CPLT, last_event_);
  EXPECT_EQ(NfcStatus::OK, last_status_);
}

/*
 * CoreInitialized:
 * Calls coreInitialized() with different data
 * Waits for NfcEvent.POST_INIT_CPLT
 */
TEST_F(NfcHidlTest, CoreInitialized) {
  NfcData data;
  data.resize(1);
  for (int i = 0; i <= 6; i++)
  {
    data[0] = i;
    EXPECT_EQ(NfcStatus::OK, nfc_->coreInitialized(data));
    // Wait for NfcEvent.POST_INIT_CPLT
    EXPECT_EQ(std::cv_status::no_timeout, wait());
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
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(NfcEvent::CLOSE_CPLT, last_event_);
  EXPECT_EQ(NfcStatus::OK, last_status_);

  EXPECT_EQ(NfcStatus::OK, nfc_->controlGranted());

  EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
  // Wait for OPEN_CPLT event
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(NfcEvent::OPEN_CPLT, last_event_);
  EXPECT_EQ(NfcStatus::OK, last_status_);
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
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(NfcEvent::CLOSE_CPLT, last_event_);
  EXPECT_EQ(NfcStatus::OK, last_status_);

  EXPECT_EQ(NfcStatus::OK, nfc_->prediscover());

  EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
  // Wait for OPEN_CPLT event
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(NfcEvent::OPEN_CPLT, last_event_);
  EXPECT_EQ(NfcStatus::OK, last_status_);
}

/*
 * CloseAfterClose:
 * Calls close() multiple times
 * Checks status
 */
TEST_F(NfcHidlTest, CloseAfterClose) {
  EXPECT_EQ(NfcStatus::OK, nfc_->close());
  // Wait for CLOSE_CPLT event
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(NfcEvent::CLOSE_CPLT, last_event_);
  EXPECT_EQ(NfcStatus::OK, last_status_);

  EXPECT_EQ(NfcStatus::FAILED, nfc_->close());

  EXPECT_EQ(NfcStatus::OK, nfc_->open(nfc_cb_));
  // Wait for OPEN_CPLT event
  EXPECT_EQ(std::cv_status::no_timeout, wait());
  EXPECT_EQ(NfcEvent::OPEN_CPLT, last_event_);
  EXPECT_EQ(NfcStatus::OK, last_status_);
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
