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

#include <android-base/macros.h>
#include <android/hardware/wifi/1.0/IWifiChip.h>

#include "wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

/**
 * HIDL interface object used to control a Wifi HAL chip instance.
 * Since there is only a single chip instance used today, there is no
 * identifying handle information stored here.
 */
class WifiChip : public IWifiChip {
 public:
  WifiChip(ChipId chip_id, const std::weak_ptr<WifiLegacyHal> legacy_hal);
  // HIDL does not provide a built-in mechanism to let the server invalidate
  // a HIDL interface object after creation. If any client process holds onto
  // a reference to the object in their context, any method calls on that
  // reference will continue to be directed to the server.
  //
  // However Wifi HAL needs to control the lifetime of these objects. So, add
  // a public |invalidate| method to |WifiChip| and it's child objects. This
  // will be used to mark an object invalid when either:
  // a) Wifi HAL is stopped, or
  // b) Wifi Chip is reconfigured.
  //
  // All HIDL method implementations should check if the object is still marked
  // valid before processing them.
  void invalidate();

  // HIDL methods exposed.
  Return<ChipId> getId() override;
  Return<void> registerEventCallback(
      const sp<IWifiChipEventCallback>& callback) override;
  Return<void> getAvailableModes(getAvailableModes_cb cb) override;
  Return<void> configureChip(uint32_t mode_id) override;
  Return<uint32_t> getMode() override;
  Return<void> requestChipDebugInfo() override;
  Return<void> requestDriverDebugDump() override;
  Return<void> requestFirmwareDebugDump() override;

 private:
  ChipId chip_id_;
  std::weak_ptr<WifiLegacyHal> legacy_hal_;
  std::set<sp<IWifiChipEventCallback>> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(WifiChip);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_CHIP_H_
