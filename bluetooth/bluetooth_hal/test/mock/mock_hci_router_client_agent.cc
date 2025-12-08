/*
 * Copyright 2024 The Android Open Source Project
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

#include "bluetooth_hal/test/mock/mock_hci_router_client_agent.h"

#include "android-base/logging.h"
#include "bluetooth_hal/hci_router_client_agent.h"

namespace bluetooth_hal {
namespace hci {

HciRouterClientAgent& HciRouterClientAgent::GetAgent() {
  if (!MockHciRouterClientAgent::mock_agent_) {
    LOG(FATAL) << __func__
               << ": mock_agent_ is nullptr. Did you forget to call "
                  "SetMockAgent in your test SetUp?";
  }
  return *MockHciRouterClientAgent::mock_agent_;
}

void MockHciRouterClientAgent::SetMockAgent(MockHciRouterClientAgent* agent) {
  mock_agent_ = agent;
}

}  // namespace hci
}  // namespace bluetooth_hal
