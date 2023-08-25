/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "wifi_p2p_iface.h"

#include <android-base/logging.h>

#include "aidl_return_util.h"
#include "wifi_status_util.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
using aidl_return_util::validateAndCall;

WifiP2pIface::WifiP2pIface(const std::string& ifname,
                           const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal)
    : ifname_(ifname), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiP2pIface::invalidate() {
    legacy_hal_.reset();
    is_valid_ = false;
}

bool WifiP2pIface::isValid() {
    return is_valid_;
}

std::string WifiP2pIface::getName() {
    return ifname_;
}

ndk::ScopedAStatus WifiP2pIface::getName(std::string* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiP2pIface::getNameInternal, _aidl_return);
}

std::pair<std::string, ndk::ScopedAStatus> WifiP2pIface::getNameInternal() {
    return {ifname_, ndk::ScopedAStatus::ok()};
}

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl
