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

#ifndef WIFI_P2P_IFACE_H_
#define WIFI_P2P_IFACE_H_

#include <aidl/android/hardware/wifi/BnWifiP2pIface.h>
#include <android-base/macros.h>

#include "wifi_legacy_hal.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {

/**
 * AIDL interface object used to control a P2P Iface instance.
 */
class WifiP2pIface : public BnWifiP2pIface {
  public:
    WifiP2pIface(const std::string& ifname,
                 const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal);
    // Refer to |WifiChip::invalidate()|.
    void invalidate();
    bool isValid();
    std::string getName();

    // AIDL methods exposed.
    ndk::ScopedAStatus getName(std::string* _aidl_return) override;

  private:
    // Corresponding worker functions for the AIDL methods.
    std::pair<std::string, ndk::ScopedAStatus> getNameInternal();

    std::string ifname_;
    std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal_;
    bool is_valid_;

    DISALLOW_COPY_AND_ASSIGN(WifiP2pIface);
};

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // WIFI_P2P_IFACE_H_
