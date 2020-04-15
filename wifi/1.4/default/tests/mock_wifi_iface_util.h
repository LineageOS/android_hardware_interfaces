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

#ifndef MOCK_WIFI_IFACE_UTIL_H_
#define MOCK_WIFI_IFACE_UTIL_H_

#include <gmock/gmock.h>

#include "wifi_iface_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_4 {
namespace implementation {
namespace iface_util {

class MockWifiIfaceUtil : public WifiIfaceUtil {
   public:
    MockWifiIfaceUtil(
        const std::weak_ptr<wifi_system::InterfaceTool> iface_tool);
    MOCK_METHOD1(getFactoryMacAddress,
                 std::array<uint8_t, 6>(const std::string&));
    MOCK_METHOD2(setMacAddress,
                 bool(const std::string&, const std::array<uint8_t, 6>&));
    MOCK_METHOD0(getOrCreateRandomMacAddress, std::array<uint8_t, 6>());
    MOCK_METHOD2(registerIfaceEventHandlers,
                 void(const std::string&, IfaceEventHandlers));
    MOCK_METHOD1(unregisterIfaceEventHandlers, void(const std::string&));
    MOCK_METHOD2(setUpState, bool(const std::string&, bool));
};
}  // namespace iface_util
}  // namespace implementation
}  // namespace V1_4
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // MOCK_WIFI_IFACE_UTIL_H_
