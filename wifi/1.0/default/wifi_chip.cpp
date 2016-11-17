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
#include "wifi_chip.h"
#include "wifi_status_util.h"

namespace {
using android::sp;
using android::hardware::hidl_vec;
using android::hardware::hidl_string;

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
using hidl_return_util::validateAndCall;

WifiChip::WifiChip(ChipId chip_id,
                   const std::weak_ptr<WifiLegacyHal> legacy_hal)
    : chip_id_(chip_id), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiChip::invalidate() {
  invalidateAndRemoveAllIfaces();
  legacy_hal_.reset();
  event_callbacks_.clear();
  is_valid_ = false;
}

bool WifiChip::isValid() {
  return is_valid_;
}

Return<void> WifiChip::getId(getId_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getIdInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::registerEventCallback(
    const sp<IWifiChipEventCallback>& event_callback,
    registerEventCallback_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::registerEventCallbackInternal,
                         hidl_status_cb,
                         event_callback);
}

Return<void> WifiChip::getAvailableModes(getAvailableModes_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getAvailableModesInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::configureChip(uint32_t mode_id,
                                     configureChip_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::configureChipInternal,
                         hidl_status_cb,
                         mode_id);
}

Return<void> WifiChip::getMode(getMode_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getModeInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::requestChipDebugInfo(
    requestChipDebugInfo_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::requestChipDebugInfoInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::requestDriverDebugDump(
    requestDriverDebugDump_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::requestDriverDebugDumpInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::requestFirmwareDebugDump(
    requestFirmwareDebugDump_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::requestFirmwareDebugDumpInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::createApIface(createApIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::createApIfaceInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::getApIfaceNames(getApIfaceNames_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getApIfaceNamesInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::getApIface(const hidl_string& ifname,
                                  getApIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getApIfaceInternal,
                         hidl_status_cb,
                         ifname);
}

Return<void> WifiChip::createNanIface(createNanIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::createNanIfaceInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::getNanIfaceNames(getNanIfaceNames_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getNanIfaceNamesInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::getNanIface(const hidl_string& ifname,
                                   getNanIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getNanIfaceInternal,
                         hidl_status_cb,
                         ifname);
}

Return<void> WifiChip::createP2pIface(createP2pIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::createP2pIfaceInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::getP2pIfaceNames(getP2pIfaceNames_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getP2pIfaceNamesInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::getP2pIface(const hidl_string& ifname,
                                   getP2pIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getP2pIfaceInternal,
                         hidl_status_cb,
                         ifname);
}

Return<void> WifiChip::createStaIface(createStaIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::createStaIfaceInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::getStaIfaceNames(getStaIfaceNames_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getStaIfaceNamesInternal,
                         hidl_status_cb);
}

Return<void> WifiChip::getStaIface(const hidl_string& ifname,
                                   getStaIface_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::getStaIfaceInternal,
                         hidl_status_cb,
                         ifname);
}

Return<void> WifiChip::createRttController(
    const sp<IWifiIface>& bound_iface, createRttController_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                         &WifiChip::createRttControllerInternal,
                         hidl_status_cb,
                         bound_iface);
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

std::pair<WifiStatus, ChipId> WifiChip::getIdInternal() {
  return {createWifiStatus(WifiStatusCode::SUCCESS), chip_id_};
}

WifiStatus WifiChip::registerEventCallbackInternal(
    const sp<IWifiChipEventCallback>& event_callback) {
  // TODO(b/31632518): remove the callback when the client is destroyed
  event_callbacks_.emplace_back(event_callback);
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

std::pair<WifiStatus, std::vector<IWifiChip::ChipMode>>
WifiChip::getAvailableModesInternal() {
  // TODO add implementation
  return {createWifiStatus(WifiStatusCode::SUCCESS),
          std::vector<IWifiChip::ChipMode>()};
}

WifiStatus WifiChip::configureChipInternal(uint32_t /* mode_id */) {
  invalidateAndRemoveAllIfaces();
  // TODO add implementation
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

std::pair<WifiStatus, uint32_t> WifiChip::getModeInternal() {
  // TODO add implementation
  return {createWifiStatus(WifiStatusCode::SUCCESS), 0};
}

std::pair<WifiStatus, IWifiChip::ChipDebugInfo>
WifiChip::requestChipDebugInfoInternal() {
  IWifiChip::ChipDebugInfo result;
  wifi_error legacy_status;
  std::string driver_desc;
  std::tie(legacy_status, driver_desc) = legacy_hal_.lock()->getDriverVersion();
  if (legacy_status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver version: "
               << legacyErrorToString(legacy_status);
    WifiStatus status = createWifiStatusFromLegacyError(
        legacy_status, "failed to get driver version");
    return {status, result};
  }
  result.driverDescription = driver_desc.c_str();

  std::string firmware_desc;
  std::tie(legacy_status, firmware_desc) =
      legacy_hal_.lock()->getFirmwareVersion();
  if (legacy_status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware version: "
               << legacyErrorToString(legacy_status);
    WifiStatus status = createWifiStatusFromLegacyError(
        legacy_status, "failed to get firmware version");
    return {status, result};
  }
  result.firmwareDescription = firmware_desc.c_str();

  return {createWifiStatus(WifiStatusCode::SUCCESS), result};
}

std::pair<WifiStatus, std::vector<uint8_t>>
WifiChip::requestDriverDebugDumpInternal() {
  wifi_error legacy_status;
  std::vector<uint8_t> driver_dump;
  std::tie(legacy_status, driver_dump) =
      legacy_hal_.lock()->requestDriverMemoryDump();
  if (legacy_status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get driver debug dump: "
               << legacyErrorToString(legacy_status);
    return {createWifiStatusFromLegacyError(legacy_status),
            std::vector<uint8_t>()};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS), driver_dump};
}

std::pair<WifiStatus, std::vector<uint8_t>>
WifiChip::requestFirmwareDebugDumpInternal() {
  wifi_error legacy_status;
  std::vector<uint8_t> firmware_dump;
  std::tie(legacy_status, firmware_dump) =
      legacy_hal_.lock()->requestFirmwareMemoryDump();
  if (legacy_status != WIFI_SUCCESS) {
    LOG(ERROR) << "Failed to get firmware debug dump: "
               << legacyErrorToString(legacy_status);
    return {createWifiStatusFromLegacyError(legacy_status),
            std::vector<uint8_t>()};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS), firmware_dump};
}

std::pair<WifiStatus, sp<IWifiApIface>> WifiChip::createApIfaceInternal() {
  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getApIfaceName();
  ap_iface_ = new WifiApIface(ifname, legacy_hal_);
  return {createWifiStatus(WifiStatusCode::SUCCESS), ap_iface_};
}

std::pair<WifiStatus, std::vector<hidl_string>>
WifiChip::getApIfaceNamesInternal() {
  if (!ap_iface_.get()) {
    return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS),
          {legacy_hal_.lock()->getApIfaceName()}};
}

std::pair<WifiStatus, sp<IWifiApIface>> WifiChip::getApIfaceInternal(
    const hidl_string& ifname) {
  if (!ap_iface_.get() ||
      (ifname.c_str() != legacy_hal_.lock()->getApIfaceName())) {
    return {createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS), ap_iface_};
}

std::pair<WifiStatus, sp<IWifiNanIface>> WifiChip::createNanIfaceInternal() {
  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getNanIfaceName();
  nan_iface_ = new WifiNanIface(ifname, legacy_hal_);
  return {createWifiStatus(WifiStatusCode::SUCCESS), nan_iface_};
}

std::pair<WifiStatus, std::vector<hidl_string>>
WifiChip::getNanIfaceNamesInternal() {
  if (!nan_iface_.get()) {
    return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS),
          {legacy_hal_.lock()->getNanIfaceName()}};
}

std::pair<WifiStatus, sp<IWifiNanIface>> WifiChip::getNanIfaceInternal(
    const hidl_string& ifname) {
  if (!nan_iface_.get() ||
      (ifname.c_str() != legacy_hal_.lock()->getNanIfaceName())) {
    return {createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS), nan_iface_};
}

std::pair<WifiStatus, sp<IWifiP2pIface>> WifiChip::createP2pIfaceInternal() {
  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getP2pIfaceName();
  p2p_iface_ = new WifiP2pIface(ifname, legacy_hal_);
  return {createWifiStatus(WifiStatusCode::SUCCESS), p2p_iface_};
}

std::pair<WifiStatus, std::vector<hidl_string>>
WifiChip::getP2pIfaceNamesInternal() {
  if (!p2p_iface_.get()) {
    return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS),
          {legacy_hal_.lock()->getP2pIfaceName()}};
}

std::pair<WifiStatus, sp<IWifiP2pIface>> WifiChip::getP2pIfaceInternal(
    const hidl_string& ifname) {
  if (!p2p_iface_.get() ||
      (ifname.c_str() != legacy_hal_.lock()->getP2pIfaceName())) {
    return {createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS), p2p_iface_};
}

std::pair<WifiStatus, sp<IWifiStaIface>> WifiChip::createStaIfaceInternal() {
  // TODO(b/31997422): Disallow this based on the chip combination.
  std::string ifname = legacy_hal_.lock()->getStaIfaceName();
  sta_iface_ = new WifiStaIface(ifname, legacy_hal_);
  return {createWifiStatus(WifiStatusCode::SUCCESS), sta_iface_};
}

std::pair<WifiStatus, std::vector<hidl_string>>
WifiChip::getStaIfaceNamesInternal() {
  if (!sta_iface_.get()) {
    return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS),
          {legacy_hal_.lock()->getStaIfaceName()}};
}

std::pair<WifiStatus, sp<IWifiStaIface>> WifiChip::getStaIfaceInternal(
    const hidl_string& ifname) {
  if (!sta_iface_.get() ||
      (ifname.c_str() != legacy_hal_.lock()->getStaIfaceName())) {
    return {createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS), nullptr};
  }
  return {createWifiStatus(WifiStatusCode::SUCCESS), sta_iface_};
}

std::pair<WifiStatus, sp<IWifiRttController>>
WifiChip::createRttControllerInternal(const sp<IWifiIface>& bound_iface) {
  sp<WifiRttController> rtt = new WifiRttController(bound_iface, legacy_hal_);
  rtt_controllers_.emplace_back(rtt);
  return {createWifiStatus(WifiStatusCode::SUCCESS), rtt};
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
