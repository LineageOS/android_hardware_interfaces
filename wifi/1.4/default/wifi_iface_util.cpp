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

#include <net/if.h>
#include <cstddef>
#include <iostream>
#include <limits>
#include <random>

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <private/android_filesystem_config.h>

#undef NAN
#include "wifi_iface_util.h"

namespace {
// Constants to set the local bit & clear the multicast bit.
constexpr uint8_t kMacAddressMulticastMask = 0x01;
constexpr uint8_t kMacAddressLocallyAssignedMask = 0x02;
}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_4 {
namespace implementation {
namespace iface_util {

WifiIfaceUtil::WifiIfaceUtil(
    const std::weak_ptr<wifi_system::InterfaceTool> iface_tool)
    : iface_tool_(iface_tool),
      random_mac_address_index_(0),
      event_handlers_map_() {
    for (int i=0; i < MAX_RANDOM_MAC_ADDR_INDEX; i++)
        random_mac_address_[i] = nullptr;
}

std::array<uint8_t, 6> WifiIfaceUtil::getFactoryMacAddress(
    const std::string& iface_name) {
    return iface_tool_.lock()->GetFactoryMacAddress(iface_name.c_str());
}

bool WifiIfaceUtil::setMacAddress(const std::string& iface_name,
                                  const std::array<uint8_t, 6>& mac) {
#ifndef WIFI_AVOID_IFACE_RESET_MAC_CHANGE
    if (!iface_tool_.lock()->SetUpState(iface_name.c_str(), false)) {
        LOG(ERROR) << "SetUpState(false) failed.";
        return false;
    }
#endif
    if (!iface_tool_.lock()->SetMacAddress(iface_name.c_str(), mac)) {
        LOG(ERROR) << "SetMacAddress failed.";
        return false;
    }
#ifndef WIFI_AVOID_IFACE_RESET_MAC_CHANGE
    if (!iface_tool_.lock()->SetUpState(iface_name.c_str(), true)) {
        LOG(ERROR) << "SetUpState(true) failed.";
        return false;
    }
#endif
    IfaceEventHandlers event_handlers = {};
    const auto it = event_handlers_map_.find(iface_name);
    if (it != event_handlers_map_.end()) {
        event_handlers = it->second;
    }
    if (event_handlers.on_state_toggle_off_on != nullptr) {
        event_handlers.on_state_toggle_off_on(iface_name);
    }
    LOG(DEBUG) << "Successfully SetMacAddress.";
    return true;
}

void WifiIfaceUtil::setRandomMacAddressIndex(int idx) {
    if (idx >= MAX_RANDOM_MAC_ADDR_INDEX) {
        LOG(ERROR) << "Requested random mac address index crossed max limit!!";
        return;
    }

    random_mac_address_index_ = idx;
}

std::array<uint8_t, 6> WifiIfaceUtil::getOrCreateRandomMacAddress() {
    if (random_mac_address_[random_mac_address_index_]) {
        return *random_mac_address_[random_mac_address_index_].get();
    }
    random_mac_address_[random_mac_address_index_] =
        std::make_unique<std::array<uint8_t, 6>>(createRandomMacAddress());
    return *random_mac_address_[random_mac_address_index_].get();
}

void WifiIfaceUtil::registerIfaceEventHandlers(const std::string& iface_name,
                                               IfaceEventHandlers handlers) {
    event_handlers_map_[iface_name] = handlers;
}

void WifiIfaceUtil::unregisterIfaceEventHandlers(
    const std::string& iface_name) {
    event_handlers_map_.erase(iface_name);
}

std::array<uint8_t, 6> WifiIfaceUtil::createRandomMacAddress() {
    std::array<uint8_t, 6> address = {};
    std::random_device rd;
    std::default_random_engine engine(rd());
    std::uniform_int_distribution<uint8_t> dist(
        std::numeric_limits<uint8_t>::min(),
        std::numeric_limits<uint8_t>::max());
    for (size_t i = 0; i < address.size(); i++) {
        address[i] = dist(engine);
    }
    // Set the local bit and clear the multicast bit.
    address[0] |= kMacAddressLocallyAssignedMask;
    address[0] &= ~kMacAddressMulticastMask;
    return address;
}

bool WifiIfaceUtil::setUpState(const std::string& iface_name, bool request_up) {
    if (!iface_tool_.lock()->SetUpState(iface_name.c_str(), request_up)) {
        LOG(ERROR) << "SetUpState to " << request_up << " failed";
        return false;
    }
    return true;
}

unsigned WifiIfaceUtil::ifNameToIndex(const std::string& iface_name) {
    return if_nametoindex(iface_name.c_str());
}
}  // namespace iface_util
}  // namespace implementation
}  // namespace V1_4
}  // namespace wifi
}  // namespace hardware
}  // namespace android
