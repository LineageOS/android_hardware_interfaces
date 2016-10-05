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

#include "wifi_chip.h"

#include <android-base/logging.h>

#include "failure_reason_util.h"

namespace android {
namespace hardware {
namespace wifi {

WifiChip::WifiChip(
    WifiHalState* hal_state, wifi_interface_handle interface_handle)
    : hal_state_(hal_state), interface_handle_(interface_handle) {}

void WifiChip::Invalidate() {
  hal_state_ = nullptr;
  callbacks_.clear();
}

Return<void> WifiChip::registerEventCallback(
    const sp<V1_0::IWifiChipEventCallback>& callback) {
  if (!hal_state_) return Void();
  // TODO(b/31632518): remove the callback when the client is destroyed
  callbacks_.insert(callback);
  return Void();
}

Return<void> WifiChip::getAvailableModes(getAvailableModes_cb cb) {
  if (!hal_state_) {
    cb(hidl_vec<ChipMode>());
    return Void();
  } else {
    // TODO add implementation
    return Void();
  }
}

Return<void> WifiChip::configureChip(uint32_t /*mode_id*/) {
  if (!hal_state_) return Void();
  // TODO add implementation
  return Void();
}

Return<uint32_t> WifiChip::getMode() {
  if (!hal_state_) return 0;
  // TODO add implementation
  return 0;
}

Return<void> WifiChip::requestChipDebugInfo() {
  if (!hal_state_) return Void();

  V1_0::IWifiChipEventCallback::ChipDebugInfo result;
  result.driverDescription = "<unknown>";
  result.firmwareDescription = "<unknown>";
  char buffer[256];

  // get driver version
  bzero(buffer, sizeof(buffer));
  wifi_error ret = hal_state_->func_table_.wifi_get_driver_version(
      interface_handle_, buffer, sizeof(buffer));
  if (ret == WIFI_SUCCESS) {
    result.driverDescription = buffer;
  } else {
    LOG(WARNING) << "Failed to get driver version: "
                 << LegacyErrorToString(ret);
  }

  // get firmware version
  bzero(buffer, sizeof(buffer));
  ret = hal_state_->func_table_.wifi_get_firmware_version(
      interface_handle_, buffer, sizeof(buffer));
  if (ret == WIFI_SUCCESS) {
    result.firmwareDescription = buffer;
  } else {
    LOG(WARNING) << "Failed to get firmware version: "
                 << LegacyErrorToString(ret);
  }

  // send callback
  for (auto& callback : callbacks_) {
    callback->onChipDebugInfoAvailable(result);
  }
  return Void();
}

Return<void> WifiChip::requestDriverDebugDump() {
  // TODO implement
  return Void();
}

Return<void> WifiChip::requestFirmwareDebugDump() {
  // TODO implement
  return Void();
}


}  // namespace wifi
}  // namespace hardware
}  // namespace android
