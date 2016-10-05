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

#ifndef WIFI_CHIP_H_
#define WIFI_CHIP_H_

#include <set>

#include <android/hardware/wifi/1.0/IWifiChip.h>
#include <android-base/macros.h>
#include <hardware_legacy/wifi_hal.h>

#include "wifi_hal_state.h"

namespace android {
namespace hardware {
namespace wifi {

class WifiChip : public V1_0::IWifiChip {
 public:
  WifiChip(
      WifiHalState* hal_state, wifi_interface_handle interface_handle);

  void Invalidate();

  Return<void> registerEventCallback(
      const sp<V1_0::IWifiChipEventCallback>& callback) override;

  Return<void> getAvailableModes(getAvailableModes_cb cb) override;

  Return<void> configureChip(uint32_t mode_id) override;

  Return<uint32_t> getMode() override;

  Return<void> requestChipDebugInfo() override;

  Return<void> requestDriverDebugDump() override;

  Return<void> requestFirmwareDebugDump() override;

 private:
  WifiHalState* hal_state_;
  wifi_interface_handle interface_handle_;
  std::set<sp<V1_0::IWifiChipEventCallback>> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(WifiChip);
};

}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_CHIP_H_
