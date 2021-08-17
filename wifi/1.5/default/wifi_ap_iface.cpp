/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "hidl_return_util.h"
#include "hidl_struct_util.h"
#include "wifi_ap_iface.h"
#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {
using hidl_return_util::validateAndCall;

WifiApIface::WifiApIface(
    const std::string& ifname, const std::vector<std::string>& instances,
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

bool WifiApIface::isValid() { return is_valid_; }

std::string WifiApIface::getName() { return ifname_; }

void WifiApIface::removeInstance(std::string instance) {
    instances_.erase(
        std::remove(instances_.begin(), instances_.end(), instance),
        instances_.end());
}

Return<void> WifiApIface::getName(getName_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getNameInternal, hidl_status_cb);
}

Return<void> WifiApIface::getType(getType_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getTypeInternal, hidl_status_cb);
}

Return<void> WifiApIface::setCountryCode(const hidl_array<int8_t, 2>& code,
                                         setCountryCode_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::setCountryCodeInternal, hidl_status_cb,
                           code);
}

Return<void> WifiApIface::getValidFrequenciesForBand(
    V1_0::WifiBand band, getValidFrequenciesForBand_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getValidFrequenciesForBandInternal,
                           hidl_status_cb, band);
}

Return<void> WifiApIface::setMacAddress(const hidl_array<uint8_t, 6>& mac,
                                        setMacAddress_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::setMacAddressInternal, hidl_status_cb,
                           mac);
}

Return<void> WifiApIface::getFactoryMacAddress(
    getFactoryMacAddress_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getFactoryMacAddressInternal,
                           hidl_status_cb,
                           instances_.size() > 0 ? instances_[0] : ifname_);
}

Return<void> WifiApIface::resetToFactoryMacAddress(
    resetToFactoryMacAddress_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::resetToFactoryMacAddressInternal,
                           hidl_status_cb);
}

Return<void> WifiApIface::getBridgedInstances(
    getBridgedInstances_cb hidl_status_cb) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiApIface::getBridgedInstancesInternal,
                           hidl_status_cb);
}

std::pair<WifiStatus, std::string> WifiApIface::getNameInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), ifname_};
}

std::pair<WifiStatus, IfaceType> WifiApIface::getTypeInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::AP};
}

WifiStatus WifiApIface::setCountryCodeInternal(
    const std::array<int8_t, 2>& code) {
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->setCountryCode(
        instances_.size() > 0 ? instances_[0] : ifname_, code);
    return createWifiStatusFromLegacyError(legacy_status);
}

std::pair<WifiStatus, std::vector<WifiChannelInMhz>>
WifiApIface::getValidFrequenciesForBandInternal(V1_0::WifiBand band) {
    static_assert(sizeof(WifiChannelInMhz) == sizeof(uint32_t),
                  "Size mismatch");
    legacy_hal::wifi_error legacy_status;
    std::vector<uint32_t> valid_frequencies;
    std::tie(legacy_status, valid_frequencies) =
        legacy_hal_.lock()->getValidFrequenciesForBand(
            instances_.size() > 0 ? instances_[0] : ifname_,
            hidl_struct_util::convertHidlWifiBandToLegacy(band));
    return {createWifiStatusFromLegacyError(legacy_status), valid_frequencies};
}

WifiStatus WifiApIface::setMacAddressInternal(
    const std::array<uint8_t, 6>& mac) {
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
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

std::pair<WifiStatus, std::array<uint8_t, 6>>
WifiApIface::getFactoryMacAddressInternal(const std::string& ifaceName) {
    std::array<uint8_t, 6> mac =
        iface_util_.lock()->getFactoryMacAddress(ifaceName);
    if (mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 &&
        mac[4] == 0 && mac[5] == 0) {
        return {createWifiStatus(WifiStatusCode::ERROR_UNKNOWN), mac};
    }
    return {createWifiStatus(WifiStatusCode::SUCCESS), mac};
}

WifiStatus WifiApIface::resetToFactoryMacAddressInternal() {
    std::pair<WifiStatus, std::array<uint8_t, 6>> getMacResult;
    if (instances_.size() == 2) {
        for (auto const& intf : instances_) {
            getMacResult = getFactoryMacAddressInternal(intf);
            LOG(DEBUG) << "Reset MAC to factory MAC on " << intf;
            if (getMacResult.first.code != WifiStatusCode::SUCCESS ||
                !iface_util_.lock()->setMacAddress(intf, getMacResult.second)) {
                return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
            }
        }
        // It needs to set mac address for bridged interface, otherwise the mac
        // address of the bridged interface will be changed after one of the
        // instance down. Thus we are generating a random MAC address for the
        // bridged interface even if we got the request to reset the Factory
        // MAC. Since the bridged interface is an internal interface for the
        // operation of bpf and others networking operation.
        if (!iface_util_.lock()->setMacAddress(
                ifname_, iface_util_.lock()->createRandomMacAddress())) {
            LOG(ERROR) << "Fail to config MAC for bridged interface "
                       << ifname_;
            return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
        }
    } else {
        getMacResult = getFactoryMacAddressInternal(ifname_);
        LOG(DEBUG) << "Reset MAC to factory MAC on " << ifname_;
        if (getMacResult.first.code != WifiStatusCode::SUCCESS ||
            !iface_util_.lock()->setMacAddress(ifname_, getMacResult.second)) {
            return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
        }
    }
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

std::pair<WifiStatus, std::vector<hidl_string>>
WifiApIface::getBridgedInstancesInternal() {
    std::vector<hidl_string> instances;
    for (const auto& instance_name : instances_) {
        instances.push_back(instance_name);
    }
    return {createWifiStatus(WifiStatusCode::SUCCESS), instances};
}
}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android
