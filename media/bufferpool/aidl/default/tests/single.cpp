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

#define LOG_TAG "buffferpool_unit_test"

#include <gtest/gtest.h>

#include <android-base/logging.h>
#include <binder/ProcessState.h>
#include <bufferpool2/ClientManager.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <vector>
#include "allocator.h"

using aidl::android::hardware::media::bufferpool2::implementation::BufferId;
using aidl::android::hardware::media::bufferpool2::implementation::BufferPoolStatus;
using aidl::android::hardware::media::bufferpool2::implementation::ClientManager;
using aidl::android::hardware::media::bufferpool2::implementation::ConnectionId;
using aidl::android::hardware::media::bufferpool2::implementation::TransactionId;
using aidl::android::hardware::media::bufferpool2::BufferPoolData;

namespace {

// Number of iteration for buffer allocation test.
constexpr static int kNumAllocationTest = 3;

// Number of iteration for buffer recycling test.
constexpr static int kNumRecycleTest = 3;

// media.bufferpool test setup
class BufferpoolSingleTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    BufferPoolStatus status;
    mConnectionValid = false;

    mManager = ClientManager::getInstance();
    ASSERT_NE(mManager, nullptr);

    mAllocator = std::make_shared<TestBufferPoolAllocator>();
    ASSERT_TRUE((bool)mAllocator);

    status = mManager->create(mAllocator, &mConnectionId);
    ASSERT_TRUE(status == ResultStatus::OK);

    mConnectionValid = true;

    bool isNew = true;
    status = mManager->registerSender(mManager, mConnectionId, &mReceiverId, &isNew);
    ASSERT_TRUE(status == ResultStatus::OK && isNew == false &&
                mReceiverId == mConnectionId);
  }

  virtual void TearDown() override {
    if (mConnectionValid) {
      mManager->close(mConnectionId);
    }
  }

 protected:
  static void description(const std::string& description) {
    RecordProperty("description", description);
  }

  std::shared_ptr<ClientManager> mManager;
  std::shared_ptr<BufferPoolAllocator> mAllocator;
  bool mConnectionValid;
  ConnectionId mConnectionId;
  ConnectionId mReceiverId;

};

// Buffer allocation test.
// Check whether each buffer allocation is done successfully with
// unique buffer id.
TEST_F(BufferpoolSingleTest, AllocateBuffer) {
  BufferPoolStatus status;
  std::vector<uint8_t> vecParams;
  getTestAllocatorParams(&vecParams);

  std::shared_ptr<BufferPoolData> buffer[kNumAllocationTest];
  native_handle_t *allocHandle = nullptr;
  for (int i = 0; i < kNumAllocationTest; ++i) {
    status = mManager->allocate(mConnectionId, vecParams, &allocHandle, &buffer[i]);
    ASSERT_TRUE(status == ResultStatus::OK);
    if (allocHandle) {
      native_handle_close(allocHandle);
      native_handle_delete(allocHandle);
    }
  }
  for (int i = 0; i < kNumAllocationTest; ++i) {
    for (int j = i + 1; j < kNumAllocationTest; ++j) {
      ASSERT_TRUE(buffer[i]->mId != buffer[j]->mId);
    }
  }
  EXPECT_TRUE(kNumAllocationTest > 1);
}

// Buffer recycle test.
// Check whether de-allocated buffers are recycled.
TEST_F(BufferpoolSingleTest, RecycleBuffer) {
  BufferPoolStatus status;
  std::vector<uint8_t> vecParams;
  getTestAllocatorParams(&vecParams);

  BufferId bid[kNumRecycleTest];
  for (int i = 0; i < kNumRecycleTest; ++i) {
    std::shared_ptr<BufferPoolData> buffer;
    native_handle_t *allocHandle = nullptr;
    status = mManager->allocate(mConnectionId, vecParams, &allocHandle, &buffer);
    ASSERT_TRUE(status == ResultStatus::OK);
    bid[i] = buffer->mId;
    if (allocHandle) {
      native_handle_close(allocHandle);
      native_handle_delete(allocHandle);
    }
  }
  for (int i = 1; i < kNumRecycleTest; ++i) {
    ASSERT_TRUE(bid[i - 1] == bid[i]);
  }
  EXPECT_TRUE(kNumRecycleTest > 1);
}

// Buffer transfer test.
// Check whether buffer is transferred to another client successfully.
TEST_F(BufferpoolSingleTest, TransferBuffer) {
  BufferPoolStatus status;
  std::vector<uint8_t> vecParams;
  getTestAllocatorParams(&vecParams);
  std::shared_ptr<BufferPoolData> sbuffer, rbuffer;
  native_handle_t *allocHandle = nullptr;
  native_handle_t *recvHandle = nullptr;

  TransactionId transactionId;
  int64_t postMs;

  status = mManager->allocate(mConnectionId, vecParams, &allocHandle, &sbuffer);
  ASSERT_TRUE(status == ResultStatus::OK);
  ASSERT_TRUE(TestBufferPoolAllocator::Fill(allocHandle, 0x77));
  status = mManager->postSend(mReceiverId, sbuffer, &transactionId, &postMs);
  ASSERT_TRUE(status == ResultStatus::OK);
  status = mManager->receive(mReceiverId, transactionId, sbuffer->mId, postMs,
                             &recvHandle, &rbuffer);
  EXPECT_TRUE(status == ResultStatus::OK);
  ASSERT_TRUE(TestBufferPoolAllocator::Verify(recvHandle, 0x77));

  if (allocHandle) {
    native_handle_close(allocHandle);
    native_handle_delete(allocHandle);
  }
  if (recvHandle) {
    native_handle_close(recvHandle);
    native_handle_delete(recvHandle);
  }
}

}  // anonymous namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;
  return status;
}
