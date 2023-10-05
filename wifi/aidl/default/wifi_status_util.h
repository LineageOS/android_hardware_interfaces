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

#ifndef WIFI_STATUS_UTIL_H_
#define WIFI_STATUS_UTIL_H_

#include <aidl/android/hardware/wifi/IWifi.h>

#include "wifi_legacy_hal.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
using ::aidl::android::hardware::wifi::WifiStatusCode;

std::string legacyErrorToString(legacy_hal::wifi_error error);
ndk::ScopedAStatus createWifiStatus(WifiStatusCode code, const std::string& description);
ndk::ScopedAStatus createWifiStatus(WifiStatusCode code);
ndk::ScopedAStatus createWifiStatusFromLegacyError(legacy_hal::wifi_error error,
                                                   const std::string& description);
ndk::ScopedAStatus createWifiStatusFromLegacyError(legacy_hal::wifi_error error);

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // WIFI_STATUS_UTIL_H_
