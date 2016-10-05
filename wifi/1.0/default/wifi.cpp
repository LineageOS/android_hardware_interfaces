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
#include <cutils/properties.h>

#include "failure_reason_util.h"
#include "wifi_chip.h"

using RunState = ::android::hardware::wifi::WifiHalState::RunState;

namespace {
std::string GetWlanInterfaceName() {
  char buffer[PROPERTY_VALUE_MAX];
  property_get("wifi.interface", buffer, "wlan0");
  return buffer;
}
}

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

Wifi::Wifi(sp<Looper>& looper) : state_(looper) {
  CHECK_EQ(init_wifi_vendor_hal_func_table(&state_.func_table_), WIFI_SUCCESS)
      << "Failed to initialize hal func table";
}

Return<void> Wifi::registerEventCallback(
    const sp<IWifiEventCallback>& callback) {
  // TODO(b/31632518): remove the callback when the client is destroyed
  callbacks_.insert(callback);
  return Void();
}

Return<bool> Wifi::isStarted() {
  return state_.run_state_ != RunState::STOPPED;
}

Return<void> Wifi::start() {
  if (state_.run_state_ == RunState::STARTED) {
    for (auto& callback : callbacks_) {
      callback->onStart();
    }
    return Void();
  } else if (state_.run_state_ == RunState::STOPPING) {
    for (auto& callback : callbacks_) {
      callback->onStartFailure(CreateFailureReason(
          CommandFailureReason::NOT_AVAILABLE, "HAL is stopping"));
    }
    return Void();
  }

  LOG(INFO) << "Initializing HAL";
  wifi_error status = state_.func_table_.wifi_initialize(&state_.hal_handle_);
  if (status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to initialize Wifi HAL";
    for (auto& callback : callbacks_) {
      callback->onStartFailure(
          CreateFailureReasonLegacyError(status, "Failed to initialize HAL"));
    }
    return Void();
  }

  event_loop_thread_ = std::thread(&Wifi::DoHalEventLoop, this);

  wifi_interface_handle iface_handle =
      FindInterfaceHandle(GetWlanInterfaceName());
  if (iface_handle != kInterfaceNotFoundHandle) {
    chip_ = new WifiChip(&state_, iface_handle);
  } else {
    // TODO fail to init?
  }

  state_.run_state_ = RunState::STARTED;
  for (auto& callback : callbacks_) {
    callback->onStart();
  }
  return Void();
}

wifi_interface_handle Wifi::FindInterfaceHandle(const std::string& ifname) {
  int num_iface_handles = 0;
  wifi_interface_handle* iface_handles = nullptr;
  wifi_error ret = state_.func_table_.wifi_get_ifaces(
      state_.hal_handle_, &num_iface_handles, &iface_handles);
  if (ret != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to enumerate interface handles: "
               << LegacyErrorToString(ret);
    return kInterfaceNotFoundHandle;
  }

  char buffer[IFNAMSIZ];
  for (int i = 0; i < num_iface_handles; ++i) {
    bzero(buffer, sizeof(buffer));
    ret = state_.func_table_.wifi_get_iface_name(
        iface_handles[i], buffer, sizeof(buffer));
    if (ret != WIFI_SUCCESS) {
      LOG(WARNING) << "Failed to get interface handle name: "
                   << LegacyErrorToString(ret);
      continue;
    }
    if (ifname == buffer) {
      return iface_handles[i];
    }
  }
  return kInterfaceNotFoundHandle;
}

void NoopHalCleanupHandler(wifi_handle) {}

Return<void> Wifi::stop() {
  if (state_.run_state_ == RunState::STOPPED) {
    for (auto& callback : callbacks_) {
      callback->onStop();
    }
    return Void();
  } else if (state_.run_state_ == RunState::STOPPING) {
    return Void();
  }

  LOG(INFO) << "Cleaning up HAL";
  awaiting_hal_cleanup_command_ = true;
  awaiting_hal_event_loop_termination_ = true;
  state_.run_state_ = RunState::STOPPING;

  if (chip_.get())
    chip_->Invalidate();
  chip_.clear();

  state_.func_table_.wifi_cleanup(state_.hal_handle_, NoopHalCleanupHandler);
  awaiting_hal_cleanup_command_ = false;
  LOG(VERBOSE) << "HAL cleanup command complete";
  FinishHalCleanup();
  return Void();
}

void Wifi::DoHalEventLoop() {
  LOG(VERBOSE) << "Starting HAL event loop";
  state_.func_table_.wifi_event_loop(state_.hal_handle_);
  if (state_.run_state_ != RunState::STOPPING) {
    LOG(FATAL) << "HAL event loop terminated, but HAL was not stopping";
  }
  LOG(VERBOSE) << "HAL Event loop terminated";
  event_loop_thread_.detach();
  state_.PostTask([this]() {
    awaiting_hal_event_loop_termination_ = false;
    FinishHalCleanup();
  });
}

void Wifi::FinishHalCleanup() {
  if (!awaiting_hal_cleanup_command_ && !awaiting_hal_event_loop_termination_) {
    state_.run_state_ = RunState::STOPPED;
    LOG(INFO) << "HAL cleanup complete";
    for (auto& callback : callbacks_) {
      callback->onStop();
    }
  }
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
