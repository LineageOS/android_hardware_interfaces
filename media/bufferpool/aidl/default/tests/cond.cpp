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

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_stability.h>
#include <android-base/logging.h>
#include <bufferpool2/ClientManager.h>

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <vector>

#include "allocator.h"

using aidl::android::hardware::media::bufferpool2::IClientManager;
using aidl::android::hardware::media::bufferpool2::ResultStatus;
using aidl::android::hardware::media::bufferpool2::implementation::BufferId;
using aidl::android::hardware::media::bufferpool2::implementation::ClientManager;
using aidl::android::hardware::media::bufferpool2::implementation::ConnectionId;
using aidl::android::hardware::media::bufferpool2::implementation::TransactionId;
using aidl::android::hardware::media::bufferpool2::BufferPoolData;

namespace {

const std::string testInstance  = std::string() + ClientManager::descriptor + "/condtest";

// communication message types between processes.
enum PipeCommand : int32_t {
    INIT_OK = 0,
    INIT_ERROR,
    SEND,
    RECEIVE_OK,
    RECEIVE_ERROR,
};

// communication message between processes.
union PipeMessage {
    struct  {
        int32_t command;
        BufferId bufferId;
        ConnectionId connectionId;
        TransactionId transactionId;
        int64_t  timestampUs;
    } data;
    char array[0];
};

constexpr int kSignalInt = 200;

// media.bufferpool test setup
class BufferpoolMultiTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    BufferPoolStatus status;
    mReceiverPid = -1;
    mConnectionValid = false;

    ASSERT_TRUE(pipe(mCommandPipeFds) == 0);
    ASSERT_TRUE(pipe(mResultPipeFds) == 0);

    mReceiverPid = fork();
    ASSERT_TRUE(mReceiverPid >= 0);

    if (mReceiverPid == 0) {
      doReceiver();
      // In order to ignore gtest behaviour, wait for being killed from
      // tearDown
      pause();
    }

    mManager = ClientManager::getInstance();
    ASSERT_NE(mManager, nullptr);

    mAllocator = std::make_shared<TestBufferPoolAllocator>();
    ASSERT_TRUE((bool)mAllocator);

    status = mManager->create(mAllocator, &mConnectionId);
    ASSERT_TRUE(status == ResultStatus::OK);
    mConnectionValid = true;
  }

  virtual void TearDown() override {
    if (mReceiverPid > 0) {
      kill(mReceiverPid, SIGKILL);
      int wstatus;
      wait(&wstatus);
    }

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
  pid_t mReceiverPid;
  int mCommandPipeFds[2];
  int mResultPipeFds[2];

  bool sendMessage(int *pipes, const PipeMessage &message) {
    int ret = write(pipes[1], message.array, sizeof(PipeMessage));
    return ret == sizeof(PipeMessage);
  }

  bool receiveMessage(int *pipes, PipeMessage *message) {
    int ret = read(pipes[0], message->array, sizeof(PipeMessage));
    return ret == sizeof(PipeMessage);
  }

  void doReceiver() {
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    PipeMessage message;
    mManager = ClientManager::getInstance();
    if (!mManager) {
      message.data.command = PipeCommand::INIT_ERROR;
      sendMessage(mResultPipeFds, message);
      return;
    }
    auto binder = mManager->asBinder();
    AIBinder_forceDowngradeToSystemStability(binder.get());
    binder_status_t status =
        AServiceManager_addService(binder.get(), testInstance.c_str());
    CHECK_EQ(status, STATUS_OK);
    if (status != android::OK) {
      message.data.command = PipeCommand::INIT_ERROR;
      sendMessage(mResultPipeFds, message);
      return;
    }
    message.data.command = PipeCommand::INIT_OK;
    sendMessage(mResultPipeFds, message);

    int val = 0;
    receiveMessage(mCommandPipeFds, &message);
    {
      native_handle_t *rhandle = nullptr;
      std::shared_ptr<BufferPoolData> rbuffer;
      void *mem = nullptr;
      IpcMutex *mutex = nullptr;
      BufferPoolStatus status = mManager->receive(
          message.data.connectionId, message.data.transactionId,
          message.data.bufferId, message.data.timestampUs, &rhandle, &rbuffer);
      mManager->close(message.data.connectionId);
      if (status != ResultStatus::OK) {
          message.data.command = PipeCommand::RECEIVE_ERROR;
          sendMessage(mResultPipeFds, message);
          return;
      }
      if (!TestBufferPoolAllocator::MapMemoryForMutex(rhandle, &mem)) {
          message.data.command = PipeCommand::RECEIVE_ERROR;
          sendMessage(mResultPipeFds, message);
          return;
      }
      mutex = IpcMutex::Import(mem);
      pthread_mutex_lock(&(mutex->lock));
      while (mutex->signalled != true) {
          pthread_cond_wait(&(mutex->cond), &(mutex->lock));
      }
      val = mutex->counter;
      pthread_mutex_unlock(&(mutex->lock));

      (void)TestBufferPoolAllocator::UnmapMemoryForMutex(mem);
      if (rhandle) {
        native_handle_close(rhandle);
        native_handle_delete(rhandle);
      }
    }
    if (val == kSignalInt) {
      message.data.command = PipeCommand::RECEIVE_OK;
    } else {
      message.data.command = PipeCommand::RECEIVE_ERROR;
    }
    sendMessage(mResultPipeFds, message);
  }
};

// Buffer transfer test between processes.
TEST_F(BufferpoolMultiTest, TransferBuffer) {
  BufferPoolStatus status;
  PipeMessage message;

  ASSERT_TRUE(receiveMessage(mResultPipeFds, &message));
  ABinderProcess_setThreadPoolMaxThreadCount(1);
  ABinderProcess_startThreadPool();


  std::shared_ptr<IClientManager> receiver =
      IClientManager::fromBinder(
          ndk::SpAIBinder(AServiceManager_waitForService(testInstance.c_str())));
  ASSERT_NE(receiver, nullptr);
  ConnectionId receiverId;

  bool isNew = true;
  status = mManager->registerSender(receiver, mConnectionId, &receiverId, &isNew);
  ASSERT_TRUE(status == ResultStatus::OK);
  {
    native_handle_t *shandle = nullptr;
    std::shared_ptr<BufferPoolData> sbuffer;
    TransactionId transactionId;
    int64_t postUs;
    std::vector<uint8_t> vecParams;
    void *mem = nullptr;
    IpcMutex *mutex = nullptr;

    getIpcMutexParams(&vecParams);
    status = mManager->allocate(mConnectionId, vecParams, &shandle, &sbuffer);
    ASSERT_TRUE(status == ResultStatus::OK);

    ASSERT_TRUE(TestBufferPoolAllocator::MapMemoryForMutex(shandle, &mem));

    mutex = new(mem) IpcMutex();
    mutex->init();

    status = mManager->postSend(receiverId, sbuffer, &transactionId, &postUs);
    ASSERT_TRUE(status == ResultStatus::OK);

    message.data.command = PipeCommand::SEND;
    message.data.bufferId = sbuffer->mId;
    message.data.connectionId = receiverId;
    message.data.transactionId = transactionId;
    message.data.timestampUs = postUs;
    sendMessage(mCommandPipeFds, message);
    for (int i=0; i < 200000000; ++i) {
      // no-op in order to ensure
      // pthread_cond_wait is called before pthread_cond_signal
    }
    pthread_mutex_lock(&(mutex->lock));
    mutex->counter = kSignalInt;
    mutex->signalled = true;
    pthread_cond_signal(&(mutex->cond));
    pthread_mutex_unlock(&(mutex->lock));
    (void)TestBufferPoolAllocator::UnmapMemoryForMutex(mem);
    if (shandle) {
      native_handle_close(shandle);
      native_handle_delete(shandle);
    }
  }
  EXPECT_TRUE(receiveMessage(mResultPipeFds, &message));
  EXPECT_TRUE(message.data.command == PipeCommand::RECEIVE_OK);
}

}  // anonymous namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  LOG(INFO) << "Test result = " << status;
  return status;
}
