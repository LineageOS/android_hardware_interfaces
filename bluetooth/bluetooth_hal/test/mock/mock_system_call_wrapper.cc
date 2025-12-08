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

#include "bluetooth_hal/test/mock/mock_system_call_wrapper.h"

#include "android-base/logging.h"
#include "bluetooth_hal/util/system_call_wrapper.h"

namespace bluetooth_hal {
namespace util {

SystemCallWrapper& SystemCallWrapper::GetWrapper() {
  if (!MockSystemCallWrapper::mock_system_call_wrapper_) {
    LOG(FATAL) << __func__
               << ": mock_system_call_wrapper_ is nullptr. Did you forget to "
                  "call SetMockWrapper in your test SetUp?";
  }
  return *MockSystemCallWrapper::mock_system_call_wrapper_;
}

void MockSystemCallWrapper::SetMockWrapper(MockSystemCallWrapper* wrapper) {
  mock_system_call_wrapper_ = wrapper;
}

}  // namespace util
}  // namespace bluetooth_hal
