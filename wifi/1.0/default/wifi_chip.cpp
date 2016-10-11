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

#include "wifi_chip.h"

#include <android-base/logging.h>

#include "failure_reason_util.h"

namespace {
using android::sp;
using android::hardware::hidl_vec;
using android::hardware::hidl_string;

hidl_vec<hidl_string> createHidlVecOfIfaceNames(const std::string& ifname) {
  std::vector<hidl_string> ifnames;
  if (!ifname.empty()) {
    hidl_string hidl_ifname;
    hidl_ifname = ifname.c_str();
    ifnames.emplace_back(hidl_ifname);
  }
  hidl_vec<hidl_string> hidl_ifnames;
  hidl_ifnames.setToExternal(ifnames.data(), ifnames.size());
  return hidl_ifnames;
}

template <typename Iface>
void invalidateAndClear(sp<Iface>& iface) {
  if (iface.get()) {
    iface->invalidate();
    iface.clear();
  }
}
}  // namepsace

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

WifiChip::WifiChip(ChipId chip_id,
                   const std::weak_ptr<WifiLegacyHal> legacy_hal)
    : chip_id_(chip_id), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiChip::invalidate() {
  invalidateAndRemoveAllIfaces();
  legacy_hal_.reset();
  callbacks_.clear();
  is_valid_ = false;
}

Return<ChipId> WifiChip::getId() {
  return chip_id_;
}

Return<void> WifiChip::registerEventCallback(
    const sp<IWifiChipEventCallback>& callback) {
  if (!is_valid_)
    return Void();
  // TODO(b/31632518): remove the callback when the client is destroyed
  callbacks_.emplace_back(callback);
  return Void();
}

Return<void> WifiChip::getAvailableModes(getAvailableModes_cb cb) {
  if (!is_valid_) {
    cb(hidl_vec<ChipMode>());
    return Void();
  } else {
    // TODO add implementation
    return Void();
  }
}

Return<void> WifiChip::configureChip(uint32_t /*mode_id*/) {
  if (!is_valid_)
    return Void();

  invalidateAndRemoveAllIfaces();
  // TODO add implementation
  return Void();
}

Return<uint32_t> WifiChip::getMode() {
  if (!is_valid_)
    return 0;
  // TODO add implementation
  return 0;
}

Return<void> WifiChip::requestChipDebugInfo() {
  if (!is_valid_)
    return Void();

  IWifiChipEventCallback::ChipDebugInfo result;

  std::pair<wifi_error, std::string> ret =
      legacy_hal_.lock()->getDriverVersion();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver version: "
               << LegacyErrorToString(ret.first);
    return Void();
  }
  result.driverDescription = ret.second.c_str();

  ret = legacy_hal_.lock()->getFirmwareVersion();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware version: "
               << LegacyErrorToString(ret.first);
    return Void();
  }
  result.firmwareDescription = ret.second.c_str();

  for (const auto& callback : callbacks_) {
    callback->onChipDebugInfoAvailable(result);
  }
  return Void();
}

Return<void> WifiChip::requestDriverDebugDump() {
  if (!is_valid_)
    return Void();

  std::pair<wifi_error, std::vector<char>> ret =
      legacy_hal_.lock()->requestDriverMemoryDump();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver debug dump: "
               << LegacyErrorToString(ret.first);
    return Void();
  }

  auto& driver_dump = ret.second;
  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(driver_dump.data()),
                          driver_dump.size());
  for (const auto& callback : callbacks_) {
    callback->onDriverDebugDumpAvailable(hidl_data);
  }
  return Void();
}

Return<void> WifiChip::requestFirmwareDebugDump() {
  if (!is_valid_)
    return Void();

  std::pair<wifi_error, std::vector<char>> ret =
      legacy_hal_.lock()->requestFirmwareMemoryDump();
  if (ret.first != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware debug dump: "
               << LegacyErrorToString(ret.first);
    return Void();
  }

  auto& firmware_dump = ret.second;
  hidl_vec<uint8_t> hidl_data;
  hidl_data.setToExternal(reinterpret_cast<uint8_t*>(firmware_dump.data()),
                          firmware_dump.size());
  for (const auto& callback : callbacks_) {
    callback->onFirmwareDebugDumpAvailable(hidl_data);
  }
  return Void();
}

Return<void> WifiChip::createApIface(createApIface_cb cb) {
  if (!is_valid_) {
    cb(nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getApIfaceName();
  ap_iface_ = new WifiApIface(ifname, legacy_hal_);
  cb(ap_iface_);
  return Void();
}

Return<void> WifiChip::getApIfaceNames(getApIfaceNames_cb cb) {
  if (!is_valid_) {
    cb(hidl_vec<hidl_string>());
    return Void();
  }

  std::string ifname;
  if (ap_iface_.get()) {
    ifname = legacy_hal_.lock()->getApIfaceName().c_str();
  }
  cb(createHidlVecOfIfaceNames(ifname));
  return Void();
}

Return<void> WifiChip::getApIface(const hidl_string& ifname, getApIface_cb cb) {
  if (!is_valid_) {
    cb(nullptr);
    return Void();
  }

  if (ap_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getApIfaceName())) {
    cb(ap_iface_);
  } else {
    cb(nullptr);
  }
  return Void();
}

Return<void> WifiChip::createNanIface(createNanIface_cb cb) {
  if (!is_valid_) {
    cb(nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getNanIfaceName();
  nan_iface_ = new WifiNanIface(ifname, legacy_hal_);
  cb(nan_iface_);
  return Void();
}

Return<void> WifiChip::getNanIfaceNames(getNanIfaceNames_cb cb) {
  if (!is_valid_) {
    cb(hidl_vec<hidl_string>());
    return Void();
  }

  std::string ifname;
  if (nan_iface_.get()) {
    ifname = legacy_hal_.lock()->getNanIfaceName().c_str();
  }
  cb(createHidlVecOfIfaceNames(ifname));
  return Void();
}

Return<void> WifiChip::getNanIface(const hidl_string& ifname,
                                   getNanIface_cb cb) {
  if (!is_valid_) {
    cb(nullptr);
    return Void();
  }

  if (nan_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getNanIfaceName())) {
    cb(nan_iface_);
  } else {
    cb(nullptr);
  }
  return Void();
}

Return<void> WifiChip::createP2pIface(createP2pIface_cb cb) {
  if (!is_valid_) {
    cb(nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getP2pIfaceName();
  p2p_iface_ = new WifiP2pIface(ifname, legacy_hal_);
  cb(p2p_iface_);
  return Void();
}

Return<void> WifiChip::getP2pIfaceNames(getP2pIfaceNames_cb cb) {
  if (!is_valid_) {
    cb(hidl_vec<hidl_string>());
    return Void();
  }

  std::string ifname;
  if (p2p_iface_.get()) {
    ifname = legacy_hal_.lock()->getP2pIfaceName().c_str();
  }
  cb(createHidlVecOfIfaceNames(ifname));
  return Void();
}

Return<void> WifiChip::getP2pIface(const hidl_string& ifname,
                                   getP2pIface_cb cb) {
  if (!is_valid_) {
    cb(nullptr);
    return Void();
  }

  if (p2p_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getP2pIfaceName())) {
    cb(p2p_iface_);
  } else {
    cb(nullptr);
  }
  return Void();
}

Return<void> WifiChip::createStaIface(createStaIface_cb cb) {
  if (!is_valid_) {
    cb(nullptr);
    return Void();
  }

  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getStaIfaceName();
  sta_iface_ = new WifiStaIface(ifname, legacy_hal_);
  cb(sta_iface_);
  return Void();
}

Return<void> WifiChip::getStaIfaceNames(getStaIfaceNames_cb cb) {
  if (!is_valid_) {
    cb(hidl_vec<hidl_string>());
    return Void();
  }

  std::string ifname;
  if (sta_iface_.get()) {
    ifname = legacy_hal_.lock()->getStaIfaceName().c_str();
  }
  cb(createHidlVecOfIfaceNames(ifname));
  return Void();
}

Return<void> WifiChip::getStaIface(const hidl_string& ifname,
                                   getStaIface_cb cb) {
  if (!is_valid_) {
    cb(nullptr);
    return Void();
  }

  if (sta_iface_.get() &&
      (ifname.c_str() == legacy_hal_.lock()->getStaIfaceName())) {
    cb(sta_iface_);
  } else {
    cb(nullptr);
  }
  return Void();
}

Return<void> WifiChip::createRttController(const sp<IWifiIface>& bound_iface,
                                           createRttController_cb cb) {
  if (!is_valid_) {
    cb(nullptr);
    return Void();
  }

  sp<WifiRttController> rtt = new WifiRttController(bound_iface, legacy_hal_);
  rtt_controllers_.emplace_back(rtt);
  cb(rtt);
  return Void();
}

void WifiChip::invalidateAndRemoveAllIfaces() {
  invalidateAndClear(ap_iface_);
  invalidateAndClear(nan_iface_);
  invalidateAndClear(p2p_iface_);
  invalidateAndClear(sta_iface_);
  // Since all the ifaces are invalid now, all RTT controller objects
  // using those ifaces also need to be invalidated.
  for (const auto& rtt : rtt_controllers_) {
    rtt->invalidate();
  }
  rtt_controllers_.clear();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
