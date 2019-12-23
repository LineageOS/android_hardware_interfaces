/*
 * Copyright (C) 2017 The Android Open Source Project
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
#include <gmock/gmock.h>

#undef NAN  // This is weird, NAN is defined in bionic/libc/include/math.h:38
#include "mock_wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {
namespace legacy_hal {

MockWifiLegacyHal::MockWifiLegacyHal(
    const std::weak_ptr<wifi_system::InterfaceTool> iface_tool,
    const wifi_hal_fn& fn, bool is_primary)
    : WifiLegacyHal(iface_tool, fn, is_primary) {}
}  // namespace legacy_hal
}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android
