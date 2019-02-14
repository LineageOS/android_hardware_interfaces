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

#ifndef WIFI_IFACE_UTIL_H_
#define WIFI_IFACE_UTIL_H_

#include <wifi_system/interface_tool.h>

#include <android/hardware/wifi/1.0/IWifi.h>

namespace android {
namespace hardware {
namespace wifi {
namespace V1_3 {
namespace implementation {
namespace iface_util {

/**
 * Util class for common iface operations.
 */
class WifiIfaceUtil {
   public:
    WifiIfaceUtil();
    virtual ~WifiIfaceUtil() = default;

    virtual std::array<uint8_t, 6> getFactoryMacAddress(
        const std::string& iface_name);
    virtual bool setMacAddress(const std::string& iface_name,
                               const std::array<uint8_t, 6>& mac);

   private:
    wifi_system::InterfaceTool iface_tool_;
};

}  // namespace iface_util
}  // namespace implementation
}  // namespace V1_3
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_IFACE_UTIL_H_
