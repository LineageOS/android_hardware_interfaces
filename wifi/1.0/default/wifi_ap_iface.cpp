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

#include <android-base/logging.h>

#include "hidl_return_util.h"
#include "wifi_ap_iface.h"
#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
using hidl_return_util::validateAndCall;

WifiApIface::WifiApIface(const std::string& ifname,
                         const std::weak_ptr<WifiLegacyHal> legacy_hal)
    : ifname_(ifname), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiApIface::invalidate() {
  legacy_hal_.reset();
  is_valid_ = false;
}

bool WifiApIface::isValid() {
  return is_valid_;
}

Return<void> WifiApIface::getName(getName_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiApIface::getNameInternal,
                         hidl_status_cb);
}

Return<void> WifiApIface::getType(getType_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiApIface::getTypeInternal,
                         hidl_status_cb);
}

std::pair<WifiStatus, std::string> WifiApIface::getNameInternal() {
  return {createWifiStatus(WifiStatusCode::SUCCESS), ifname_};
}

std::pair<WifiStatus, IfaceType> WifiApIface::getTypeInternal() {
  return {createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::AP};
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
