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

#ifndef WIFI_RTT_CONTROLLER_H_
#define WIFI_RTT_CONTROLLER_H_

#include <android-base/macros.h>
#include <android/hardware/wifi/1.0/IWifiIface.h>
#include <android/hardware/wifi/1.0/IWifiRttController.h>

#include "wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

/**
 * HIDL interface object used to control all RTT operations.
 */
class WifiRttController : public IWifiRttController {
 public:
  WifiRttController(const sp<IWifiIface>& bound_iface,
                    const std::weak_ptr<WifiLegacyHal> legacy_hal);
  // Refer to |WifiChip::invalidate()|.
  void invalidate();
  bool isValid();

  // HIDL methods exposed.
  Return<void> getBoundIface(getBoundIface_cb hidl_status_cb) override;

 private:
  // Corresponding worker functions for the HIDL methods.
  std::pair<WifiStatus, sp<IWifiIface>> getBoundIfaceInternal();

  sp<IWifiIface> bound_iface_;
  std::weak_ptr<WifiLegacyHal> legacy_hal_;
  bool is_valid_;

  DISALLOW_COPY_AND_ASSIGN(WifiRttController);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_RTT_CONTROLLER_H_
