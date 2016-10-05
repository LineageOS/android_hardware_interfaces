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
namespace V1_0 {
namespace implementation {

WifiChip::WifiChip(std::weak_ptr<WifiLegacyHal> legacy_hal)
    : legacy_hal_(legacy_hal) {}

void WifiChip::invalidate() {
  legacy_hal_.reset();
  callbacks_.clear();
}

Return<void> WifiChip::registerEventCallback(
    const sp<IWifiChipEventCallback>& callback) {
  if (!legacy_hal_.lock())
    return Void();
  // TODO(b/31632518): remove the callback when the client is destroyed
  callbacks_.insert(callback);
  return Void();
}

Return<void> WifiChip::getAvailableModes(getAvailableModes_cb cb) {
  if (!legacy_hal_.lock()) {
    cb(hidl_vec<ChipMode>());
    return Void();
  } else {
    // TODO add implementation
    return Void();
  }
}

Return<void> WifiChip::configureChip(uint32_t /*mode_id*/) {
  if (!legacy_hal_.lock())
    return Void();
  // TODO add implementation
  return Void();
}

Return<uint32_t> WifiChip::getMode() {
  if (!legacy_hal_.lock())
    return 0;
  // TODO add implementation
  return 0;
}

Return<void> WifiChip::requestChipDebugInfo() {
  if (!legacy_hal_.lock())
    return Void();

  IWifiChipEventCallback::ChipDebugInfo result;

  std::pair<wifi_error, std::string> ret =
      legacy_hal_.lock()->getWlanDriverVersion();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver version: "
               << LegacyErrorToString(ret.first);
    return Void();
  }
  result.driverDescription = ret.second.c_str();

  ret = legacy_hal_.lock()->getWlanFirmwareVersion();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware version: "
               << LegacyErrorToString(ret.first);
    return Void();
  }
  result.firmwareDescription = ret.second.c_str();

  for (const auto& callback : callbacks_) {
    callback->onChipDebugInfoAvailable(result);
  }
  return Void();
}

Return<void> WifiChip::requestDriverDebugDump() {
  if (!legacy_hal_.lock())
    return Void();

  std::pair<wifi_error, std::vector<char>> ret =
      legacy_hal_.lock()->requestWlanDriverMemoryDump();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver debug dump: "
               << LegacyErrorToString(ret.first);
    return Void();
  }

  auto& driver_dump = ret.second;
  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(driver_dump.data()),
                          driver_dump.size());
  for (const auto& callback : callbacks_) {
    callback->onDriverDebugDumpAvailable(hidl_data);
  }
  return Void();
}

Return<void> WifiChip::requestFirmwareDebugDump() {
  if (!legacy_hal_.lock())
    return Void();

  std::pair<wifi_error, std::vector<char>> ret =
      legacy_hal_.lock()->requestWlanFirmwareMemoryDump();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware debug dump: "
               << LegacyErrorToString(ret.first);
    return Void();
  }

  auto& firmware_dump = ret.second;
  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(firmware_dump.data()),
                          firmware_dump.size());
  for (const auto& callback : callbacks_) {
    callback->onFirmwareDebugDumpAvailable(hidl_data);
  }
  return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
