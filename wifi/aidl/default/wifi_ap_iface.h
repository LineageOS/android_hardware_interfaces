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

#ifndef WIFI_AP_IFACE_H_
#define WIFI_AP_IFACE_H_

#include <aidl/android/hardware/wifi/BnWifiApIface.h>
#include <android-base/macros.h>

#include "wifi_iface_util.h"
#include "wifi_legacy_hal.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {

/**
 * AIDL interface object used to control an AP Iface instance.
 */
class WifiApIface : public BnWifiApIface {
  public:
    WifiApIface(const std::string& ifname, const std::vector<std::string>& instances,
                const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
                const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util);
    // Refer to |WifiChip::invalidate()|.
    void invalidate();
    bool isValid();
    std::string getName();
    void removeInstance(std::string instance);

    // AIDL methods exposed.
    ndk::ScopedAStatus getName(std::string* _aidl_return) override;
    ndk::ScopedAStatus setCountryCode(const std::array<uint8_t, 2>& in_code) override;
    ndk::ScopedAStatus setMacAddress(const std::array<uint8_t, 6>& in_mac) override;
    ndk::ScopedAStatus getFactoryMacAddress(std::array<uint8_t, 6>* _aidl_return) override;
    ndk::ScopedAStatus resetToFactoryMacAddress() override;
    ndk::ScopedAStatus getBridgedInstances(std::vector<std::string>* _aidl_return) override;

  private:
    // Corresponding worker functions for the AIDL methods.
    std::pair<std::string, ndk::ScopedAStatus> getNameInternal();
    ndk::ScopedAStatus setCountryCodeInternal(const std::array<uint8_t, 2>& code);
    ndk::ScopedAStatus setMacAddressInternal(const std::array<uint8_t, 6>& mac);
    std::pair<std::array<uint8_t, 6>, ndk::ScopedAStatus> getFactoryMacAddressInternal(
            const std::string& ifaceName);
    ndk::ScopedAStatus resetToFactoryMacAddressInternal();
    std::pair<std::vector<std::string>, ndk::ScopedAStatus> getBridgedInstancesInternal();

    std::string ifname_;
    std::vector<std::string> instances_;
    std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal_;
    std::weak_ptr<iface_util::WifiIfaceUtil> iface_util_;
    bool is_valid_;

    DISALLOW_COPY_AND_ASSIGN(WifiApIface);
};

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // WIFI_AP_IFACE_H_
