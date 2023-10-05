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

#include "wifi_ap_iface.h"

#include <android-base/logging.h>

#include "aidl_return_util.h"
#include "aidl_struct_util.h"
#include "wifi_status_util.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
using aidl_return_util::validateAndCall;

WifiApIface::WifiApIface(const std::string& ifname, const std::vector<std::string>& instances,
                         const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
                         const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util)
    : ifname_(ifname),
      instances_(instances),
      legacy_hal_(legacy_hal),
      iface_util_(iface_util),
      is_valid_(true) {}

void WifiApIface::invalidate() {
    legacy_hal_.reset();
    is_valid_ = false;
}

bool WifiApIface::isValid() {
    return is_valid_;
}

std::string WifiApIface::getName() {
    return ifname_;
}

void WifiApIface::removeInstance(std::string instance) {
    instances_.erase(std::remove(instances_.begin(), instances_.end(), instance), instances_.end());
}

ndk::ScopedAStatus WifiApIface::getName(std::string* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getNameInternal, _aidl_return);
}

ndk::ScopedAStatus WifiApIface::setCountryCode(const std::array<uint8_t, 2>& in_code) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::setCountryCodeInternal, in_code);
}

ndk::ScopedAStatus WifiApIface::setMacAddress(const std::array<uint8_t, 6>& in_mac) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::setMacAddressInternal, in_mac);
}

ndk::ScopedAStatus WifiApIface::getFactoryMacAddress(std::array<uint8_t, 6>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getFactoryMacAddressInternal, _aidl_return,
                           instances_.size() > 0 ? instances_[0] : ifname_);
}

ndk::ScopedAStatus WifiApIface::resetToFactoryMacAddress() {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::resetToFactoryMacAddressInternal);
}

ndk::ScopedAStatus WifiApIface::getBridgedInstances(std::vector<std::string>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getBridgedInstancesInternal, _aidl_return);
}

std::pair<std::string, ndk::ScopedAStatus> WifiApIface::getNameInternal() {
    return {ifname_, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiApIface::setCountryCodeInternal(const std::array<uint8_t, 2>& code) {
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->setCountryCode(
            instances_.size() > 0 ? instances_[0] : ifname_, code);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiApIface::setMacAddressInternal(const std::array<uint8_t, 6>& mac) {
    // Support random MAC up to 2 interfaces
    if (instances_.size() == 2) {
        int rbyte = 1;
        for (auto const& intf : instances_) {
            std::array<uint8_t, 6> rmac = mac;
            // reverse the bits to avoid collision
            rmac[rbyte] = 0xff - rmac[rbyte];
            if (!iface_util_.lock()->setMacAddress(intf, rmac)) {
                LOG(INFO) << "Failed to set random mac address on " << intf;
                return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
            }
            rbyte++;
        }
    }
    // It also needs to set mac address for bridged interface, otherwise the mac
    // address of bridged interface will be changed after one of instance
    // down.
    if (!iface_util_.lock()->setMacAddress(ifname_, mac)) {
        LOG(ERROR) << "Fail to config MAC for interface " << ifname_;
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return ndk::ScopedAStatus::ok();
}

std::pair<std::array<uint8_t, 6>, ndk::ScopedAStatus> WifiApIface::getFactoryMacAddressInternal(
        const std::string& ifaceName) {
    std::array<uint8_t, 6> mac = iface_util_.lock()->getFactoryMacAddress(ifaceName);
    if (mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0) {
        return {mac, createWifiStatus(WifiStatusCode::ERROR_UNKNOWN)};
    }
    return {mac, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiApIface::resetToFactoryMacAddressInternal() {
    std::pair<std::array<uint8_t, 6>, ndk::ScopedAStatus> getMacResult;
    if (instances_.size() == 2) {
        for (auto const& intf : instances_) {
            getMacResult = getFactoryMacAddressInternal(intf);
            LOG(DEBUG) << "Reset MAC to factory MAC on " << intf;
            if (!getMacResult.second.isOk() ||
                !iface_util_.lock()->setMacAddress(intf, getMacResult.first)) {
                return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
            }
        }
        // We need to set mac address for bridged interface, otherwise the mac
        // address of the bridged interface will be changed after one of the
        // instances goes down. Thus we are generating a random MAC address for
        // the bridged interface even if we got the request to reset the Factory
        // MAC. This is because the bridged interface is an internal interface
        // for the operation of bpf and other networking operations.
        if (!iface_util_.lock()->setMacAddress(ifname_,
                                               iface_util_.lock()->createRandomMacAddress())) {
            LOG(ERROR) << "Fail to config MAC for bridged interface " << ifname_;
            return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
        }
    } else {
        getMacResult = getFactoryMacAddressInternal(ifname_);
        LOG(DEBUG) << "Reset MAC to factory MAC on " << ifname_;
        if (!getMacResult.second.isOk() ||
            !iface_util_.lock()->setMacAddress(ifname_, getMacResult.first)) {
            return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
        }
    }
    return ndk::ScopedAStatus::ok();
}

std::pair<std::vector<std::string>, ndk::ScopedAStatus> WifiApIface::getBridgedInstancesInternal() {
    return {instances_, ndk::ScopedAStatus::ok()};
}

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl
