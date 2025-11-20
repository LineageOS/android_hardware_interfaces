/*
 * Copyright 2025 The Android Open Source Project
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

#include "bluetooth_hal/extensions/thread/thread_handler.h"

#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/test/mock/mock_hci_router.h"
#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace bluetooth_hal {
namespace thread {
namespace {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::hci::MockHciRouter;
using ::bluetooth_hal::hci::MockHciRouterClientAgent;

using ::testing::Test;

class ThreadHandlerTest : public Test {
 protected:
  void SetUp() override {
    MockHciRouter::SetMockRouter(&mock_hci_router_);
    MockHciRouterClientAgent::SetMockAgent(&mock_hci_router_client_agent_);
  }

  void TearDown() override { ThreadHandler::Cleanup(); }

  MockHciRouter mock_hci_router_;
  MockHciRouterClientAgent mock_hci_router_client_agent_;
};

TEST_F(ThreadHandlerTest, GetHandlerWithoutInitialization) {
  EXPECT_DEATH(ThreadHandler::GetHandler(), "");
}

TEST_F(ThreadHandlerTest, HandleHandlerEnabled) {
  ThreadHandler::Initialize();
  EXPECT_TRUE(ThreadHandler::IsHandlerRunning());
}

TEST_F(ThreadHandlerTest, HandleHandlerDisabled) {
  EXPECT_FALSE(ThreadHandler::IsHandlerRunning());
}

TEST_F(ThreadHandlerTest, HandleDaemonDisabledAfterBtChipClosed) {
  ThreadHandler::Initialize();
  EXPECT_FALSE(ThreadHandler::GetHandler().IsDaemonRunning());

  ThreadHandler::GetHandler().OnBluetoothChipReady();
  EXPECT_TRUE(ThreadHandler::GetHandler().IsDaemonRunning());

  ThreadHandler::GetHandler().OnBluetoothChipClosed();
  EXPECT_FALSE(ThreadHandler::GetHandler().IsDaemonRunning());
}

TEST_F(ThreadHandlerTest, IsSameHandler) {
  ThreadHandler::Initialize();
  EXPECT_EQ(&ThreadHandler::GetHandler(), &ThreadHandler::GetHandler());
}

}  // namespace
}  // namespace thread
}  // namespace bluetooth_hal
