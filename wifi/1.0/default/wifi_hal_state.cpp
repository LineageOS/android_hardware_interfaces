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

#include "wifi_hal_state.h"

#include <android-base/logging.h>
#include <hardware_legacy/wifi_hal.h>
#include <utils/Looper.h>

namespace {
class FunctionMessageHandler : public android::MessageHandler {
 public:
  explicit FunctionMessageHandler(const std::function<void()>& callback)
      : callback_(callback) {}

  ~FunctionMessageHandler() override = default;

  virtual void handleMessage(const android::Message& /*message*/) {
    callback_();
  }

 private:
  const std::function<void()> callback_;

  DISALLOW_COPY_AND_ASSIGN(FunctionMessageHandler);
};
}

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiHalState::WifiHalState(sp<Looper>& looper)
    : run_state_(RunState::STOPPED), looper_(looper) {}

void WifiHalState::PostTask(const std::function<void()>& callback) {
  sp<MessageHandler> message_handler = new FunctionMessageHandler(callback);
  looper_->sendMessage(message_handler, NULL);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
