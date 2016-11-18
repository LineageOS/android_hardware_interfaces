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
#include "wifi_sta_iface.h"
#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
using hidl_return_util::validateAndCall;

WifiStaIface::WifiStaIface(
    const std::string& ifname,
    const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal)
    : ifname_(ifname), legacy_hal_(legacy_hal), is_valid_(true) {}

void WifiStaIface::invalidate() {
  legacy_hal_.reset();
  event_callbacks_.clear();
  is_valid_ = false;
}

bool WifiStaIface::isValid() {
  return is_valid_;
}

Return<void> WifiStaIface::getName(getName_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::getNameInternal,
                         hidl_status_cb);
}

Return<void> WifiStaIface::getType(getType_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::getTypeInternal,
                         hidl_status_cb);
}

Return<void> WifiStaIface::registerEventCallback(
    const sp<IWifiStaIfaceEventCallback>& callback,
    registerEventCallback_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::registerEventCallbackInternal,
                         hidl_status_cb,
                         callback);
}

Return<void> WifiStaIface::getCapabilities(getCapabilities_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::getCapabilitiesInternal,
                         hidl_status_cb);
}

Return<void> WifiStaIface::getApfPacketFilterCapabilities(
    getApfPacketFilterCapabilities_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::getApfPacketFilterCapabilitiesInternal,
                         hidl_status_cb);
}

Return<void> WifiStaIface::installApfPacketFilter(
    uint32_t cmd_id,
    const hidl_vec<uint8_t>& program,
    installApfPacketFilter_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::installApfPacketFilterInternal,
                         hidl_status_cb,
                         cmd_id,
                         program);
}

Return<void> WifiStaIface::getBackgroundScanCapabilities(
    getBackgroundScanCapabilities_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::getBackgroundScanCapabilitiesInternal,
                         hidl_status_cb);
}

Return<void> WifiStaIface::getValidFrequenciesForBackgroundScan(
    StaBackgroundScanBand band,
    getValidFrequenciesForBackgroundScan_cb hidl_status_cb) {
  return validateAndCall(
      this,
      WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
      &WifiStaIface::getValidFrequenciesForBackgroundScanInternal,
      hidl_status_cb,
      band);
}

Return<void> WifiStaIface::startBackgroundScan(
    uint32_t cmd_id,
    const StaBackgroundScanParameters& params,
    startBackgroundScan_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::startBackgroundScanInternal,
                         hidl_status_cb,
                         cmd_id,
                         params);
}

Return<void> WifiStaIface::stopBackgroundScan(
    uint32_t cmd_id, stopBackgroundScan_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::stopBackgroundScanInternal,
                         hidl_status_cb,
                         cmd_id);
}

Return<void> WifiStaIface::enableLinkLayerStatsCollection(
    bool debug, enableLinkLayerStatsCollection_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::enableLinkLayerStatsCollectionInternal,
                         hidl_status_cb,
                         debug);
}

Return<void> WifiStaIface::disableLinkLayerStatsCollection(
    disableLinkLayerStatsCollection_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::disableLinkLayerStatsCollectionInternal,
                         hidl_status_cb);
}

Return<void> WifiStaIface::getLinkLayerStats(
    getLinkLayerStats_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::getLinkLayerStatsInternal,
                         hidl_status_cb);
}

Return<void> WifiStaIface::startDebugPacketFateMonitoring(
    startDebugPacketFateMonitoring_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::startDebugPacketFateMonitoringInternal,
                         hidl_status_cb);
}

Return<void> WifiStaIface::stopDebugPacketFateMonitoring(
    stopDebugPacketFateMonitoring_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::stopDebugPacketFateMonitoringInternal,
                         hidl_status_cb);
}

Return<void> WifiStaIface::getDebugTxPacketFates(
    getDebugTxPacketFates_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::getDebugTxPacketFatesInternal,
                         hidl_status_cb);
}

Return<void> WifiStaIface::getDebugRxPacketFates(
    getDebugRxPacketFates_cb hidl_status_cb) {
  return validateAndCall(this,
                         WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                         &WifiStaIface::getDebugRxPacketFatesInternal,
                         hidl_status_cb);
}

std::pair<WifiStatus, std::string> WifiStaIface::getNameInternal() {
  return {createWifiStatus(WifiStatusCode::SUCCESS), ifname_};
}

std::pair<WifiStatus, IfaceType> WifiStaIface::getTypeInternal() {
  return {createWifiStatus(WifiStatusCode::SUCCESS), IfaceType::STA};
}

WifiStatus WifiStaIface::registerEventCallbackInternal(
    const sp<IWifiStaIfaceEventCallback>& callback) {
  // TODO(b/31632518): remove the callback when the client is destroyed
  event_callbacks_.emplace_back(callback);
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

std::pair<WifiStatus, uint32_t> WifiStaIface::getCapabilitiesInternal() {
  // TODO implement
  return {createWifiStatus(WifiStatusCode::SUCCESS), 0};
}

std::pair<WifiStatus, StaApfPacketFilterCapabilities>
WifiStaIface::getApfPacketFilterCapabilitiesInternal() {
  // TODO implement
  return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
}

WifiStatus WifiStaIface::installApfPacketFilterInternal(
    uint32_t /* cmd_id */, const std::vector<uint8_t>& /* program */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

std::pair<WifiStatus, StaBackgroundScanCapabilities>
WifiStaIface::getBackgroundScanCapabilitiesInternal() {
  // TODO implement
  return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
}

std::pair<WifiStatus, std::vector<WifiChannelInMhz>>
WifiStaIface::getValidFrequenciesForBackgroundScanInternal(
    StaBackgroundScanBand /* band */) {
  // TODO implement
  return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
}

WifiStatus WifiStaIface::startBackgroundScanInternal(
    uint32_t /* cmd_id */, const StaBackgroundScanParameters& /* params */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiStaIface::stopBackgroundScanInternal(uint32_t /* cmd_id */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiStaIface::enableLinkLayerStatsCollectionInternal(
    bool /* debug */) {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiStaIface::disableLinkLayerStatsCollectionInternal() {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

std::pair<WifiStatus, StaLinkLayerStats>
WifiStaIface::getLinkLayerStatsInternal() {
  // TODO implement
  return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
}

WifiStatus WifiStaIface::startDebugPacketFateMonitoringInternal() {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiStaIface::stopDebugPacketFateMonitoringInternal() {
  // TODO implement
  return createWifiStatus(WifiStatusCode::SUCCESS);
}

std::pair<WifiStatus, std::vector<WifiDebugTxPacketFateReport>>
WifiStaIface::getDebugTxPacketFatesInternal() {
  // TODO implement
  return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
}

std::pair<WifiStatus, std::vector<WifiDebugRxPacketFateReport>>
WifiStaIface::getDebugRxPacketFatesInternal() {
  // TODO implement
  return {createWifiStatus(WifiStatusCode::SUCCESS), {}};
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
