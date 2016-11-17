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

#include "wifi_chip.h"
#include "wifi_status_util.h"

namespace {
// Chip ID to use for the only supported chip.
static constexpr android::hardware::wifi::V1_0::ChipId kChipId = 0;
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

Wifi::Wifi()
    : legacy_hal_(new WifiLegacyHal()), run_state_(RunState::STOPPED) {}

Return<void> Wifi::registerEventCallback(
    const sp<IWifiEventCallback>& event_callback) {
  // TODO(b/31632518): remove the callback when the client is destroyed
  event_callbacks_.emplace_back(event_callback);
  return Void();
}

Return<bool> Wifi::isStarted() {
  return run_state_ != RunState::STOPPED;
}

Return<void> Wifi::start(start_cb hidl_status_cb) {
  if (run_state_ == RunState::STARTED) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
    return Void();
  } else if (run_state_ == RunState::STOPPING) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE,
                                    "HAL is stopping"));
    return Void();
  }

  LOG(INFO) << "Starting HAL";
  wifi_error legacy_status = legacy_hal_->start();
  if (legacy_status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to start Wifi HAL";
    hidl_status_cb(
        createWifiStatusFromLegacyError(legacy_status, "Failed to start HAL"));
    return Void();
  }

  // Create the chip instance once the HAL is started.
  chip_ = new WifiChip(kChipId, legacy_hal_);
  run_state_ = RunState::STARTED;
  for (const auto& callback : event_callbacks_) {
    if (!callback->onStart().getStatus().isOk()) {
      LOG(ERROR) << "Failed to invoke onStart callback";
    };
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> Wifi::stop(stop_cb hidl_status_cb) {
  if (run_state_ == RunState::STOPPED) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
    return Void();
  } else if (run_state_ == RunState::STOPPING) {
    hidl_status_cb(createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE,
                                    "HAL is stopping"));
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
    for (const auto& callback : event_callbacks_) {
      if (!callback->onStop().getStatus().isOk()) {
        LOG(ERROR) << "Failed to invoke onStop callback";
      };
    }
  };
  wifi_error legacy_status = legacy_hal_->stop(on_complete_callback_);
  if (legacy_status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to stop Wifi HAL";
    WifiStatus wifi_status =
        createWifiStatusFromLegacyError(legacy_status, "Failed to stop HAL");
    for (const auto& callback : event_callbacks_) {
      callback->onFailure(wifi_status);
    }
    hidl_status_cb(wifi_status);
    return Void();
  }
  hidl_status_cb(createWifiStatus(WifiStatusCode::SUCCESS));
  return Void();
}

Return<void> Wifi::getChipIds(getChipIds_cb hidl_status_cb) {
  std::vector<ChipId> chip_ids;
  if (chip_.get()) {
    chip_ids.emplace_back(kChipId);
  }
  hidl_vec<ChipId> hidl_data;
  hidl_data.setToExternal(chip_ids.data(), chip_ids.size());
  hidl_status_cb(hidl_data);
  return Void();
}

Return<void> Wifi::getChip(ChipId chip_id, getChip_cb hidl_status_cb) {
  if (chip_.get() && chip_id == kChipId) {
    hidl_status_cb(chip_);
  } else {
    hidl_status_cb(nullptr);
  }
  return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
