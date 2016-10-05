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

#include "wifi.h"

#include <android-base/logging.h>

#include "failure_reason_util.h"
#include "wifi_chip.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

Wifi::Wifi()
    : legacy_hal_(new WifiLegacyHal()), run_state_(RunState::STOPPED) {}

Return<void> Wifi::registerEventCallback(
    const sp<IWifiEventCallback>& callback) {
  // TODO(b/31632518): remove the callback when the client is destroyed
  callbacks_.insert(callback);
  return Void();
}

Return<bool> Wifi::isStarted() {
  return run_state_ != RunState::STOPPED;
}

Return<void> Wifi::start() {
  if (run_state_ == RunState::STARTED) {
    for (const auto& callback : callbacks_) {
      callback->onStart();
    }
    return Void();
  } else if (run_state_ == RunState::STOPPING) {
    for (const auto& callback : callbacks_) {
      callback->onStartFailure(CreateFailureReason(
          CommandFailureReason::NOT_AVAILABLE, "HAL is stopping"));
    }
    return Void();
  }

  LOG(INFO) << "Starting HAL";
  wifi_error status = legacy_hal_->start();
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to start Wifi HAL";
    for (auto& callback : callbacks_) {
      callback->onStartFailure(
          CreateFailureReasonLegacyError(status, "Failed to start HAL"));
    }
    return Void();
  }

  // Create the chip instance once the HAL is started.
  chip_ = new WifiChip(legacy_hal_);
  run_state_ = RunState::STARTED;
  for (const auto& callback : callbacks_) {
    callback->onStart();
  }
  return Void();
}

Return<void> Wifi::stop() {
  if (run_state_ == RunState::STOPPED) {
    for (const auto& callback : callbacks_) {
      callback->onStop();
    }
    return Void();
  } else if (run_state_ == RunState::STOPPING) {
    return Void();
  }

  LOG(INFO) << "Stopping HAL";
  run_state_ = RunState::STOPPING;
  const auto on_complete_callback_ = [&]() {
    if (chip_.get()) {
      chip_->invalidate();
    }
    chip_.clear();
    run_state_ = RunState::STOPPED;
    for (const auto& callback : callbacks_) {
      callback->onStop();
    }
  };
  wifi_error status = legacy_hal_->stop(on_complete_callback_);
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to stop Wifi HAL";
    for (const auto& callback : callbacks_) {
      callback->onFailure(
          CreateFailureReasonLegacyError(status, "Failed to stop HAL"));
    }
  }
  return Void();
}

Return<void> Wifi::getChip(getChip_cb cb) {
  cb(chip_);
  return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
