/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include <android-base/macros.h>
#include <private/android_filesystem_config.h>

#include "wifi_iface_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace implementation {
namespace iface_util {

WifiIfaceUtil::WifiIfaceUtil() {}

std::array<uint8_t, 6> WifiIfaceUtil::getFactoryMacAddress(
    const std::string& iface_name) {
    return iface_tool_.GetFactoryMacAddress(iface_name.c_str());
}

bool WifiIfaceUtil::setMacAddress(const std::string& iface_name,
                                  const std::array<uint8_t, 6>& mac) {
    if (!iface_tool_.SetUpState(iface_name.c_str(), false)) {
        LOG(ERROR) << "SetUpState(false) failed.";
        return false;
    }
    if (!iface_tool_.SetMacAddress(iface_name.c_str(), mac)) {
        LOG(ERROR) << "SetMacAddress failed.";
        return false;
    }
    if (!iface_tool_.SetUpState(iface_name.c_str(), true)) {
        LOG(ERROR) << "SetUpState(true) failed.";
        return false;
    }
    LOG(DEBUG) << "Successfully SetMacAddress.";
    return true;
}

}  // namespace iface_util
}  // namespace implementation
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android
