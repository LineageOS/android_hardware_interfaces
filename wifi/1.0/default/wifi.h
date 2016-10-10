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

#ifndef WIFI_H_
#define WIFI_H_

#include <functional>

#include <android-base/macros.h>
#include <android/hardware/wifi/1.0/IWifi.h>
#include <utils/Looper.h>

#include "wifi_chip.h"
#include "wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

/**
 * Root HIDL interface object used to control the Wifi HAL.
 */
class Wifi : public IWifi {
 public:
  Wifi();

  // HIDL methods exposed.
  Return<void> registerEventCallback(
      const sp<IWifiEventCallback>& callback) override;
  Return<bool> isStarted() override;
  Return<void> start() override;
  Return<void> stop() override;
  Return<void> getChipIds(getChipIds_cb cb) override;
  Return<void> getChip(ChipId chip_id, getChip_cb cb) override;

 private:
  enum class RunState { STOPPED, STARTED, STOPPING };

  // Instance is created in this root level |IWifi| HIDL interface object
  // and shared with all the child HIDL interface objects.
  std::shared_ptr<WifiLegacyHal> legacy_hal_;
  RunState run_state_;
  std::vector<sp<IWifiEventCallback>> callbacks_;
  sp<WifiChip> chip_;

  DISALLOW_COPY_AND_ASSIGN(Wifi);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_H_
