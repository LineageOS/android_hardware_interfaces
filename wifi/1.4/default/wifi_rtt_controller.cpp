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
#include "wifi_rtt_controller.h"
#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_4 {
namespace implementation {
using hidl_return_util::validateAndCall;

WifiRttController::WifiRttController(
    const std::string& iface_name, const sp<IWifiIface>& bound_iface,
    const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal)
    : ifname_(iface_name),
      bound_iface_(bound_iface),
      legacy_hal_(legacy_hal),
      is_valid_(true) {}

void WifiRttController::invalidate() {
    legacy_hal_.reset();
    event_callbacks_.clear();
    is_valid_ = false;
}

bool WifiRttController::isValid() { return is_valid_; }

std::vector<sp<IWifiRttControllerEventCallback>>
WifiRttController::getEventCallbacks() {
    return event_callbacks_;
}

std::string WifiRttController::getIfaceName() { return ifname_; }

Return<void> WifiRttController::getBoundIface(getBoundIface_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::getBoundIfaceInternal, hidl_status_cb);
}

Return<void> WifiRttController::registerEventCallback(
    const sp<V1_0::IWifiRttControllerEventCallback>& callback,
    registerEventCallback_cb hidl_status_cb) {
    return validateAndCall(this,
                           WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::registerEventCallbackInternal,
                           hidl_status_cb, callback);
}

Return<void> WifiRttController::rangeRequest(
    uint32_t cmd_id, const hidl_vec<V1_0::RttConfig>& rtt_configs,
    rangeRequest_cb hidl_status_cb) {
    return validateAndCall(this,
                           WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::rangeRequestInternal,
                           hidl_status_cb, cmd_id, rtt_configs);
}

Return<void> WifiRttController::rangeCancel(
    uint32_t cmd_id, const hidl_vec<hidl_array<uint8_t, 6>>& addrs,
    rangeCancel_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::rangeCancelInternal, hidl_status_cb, cmd_id, addrs);
}

Return<void> WifiRttController::getCapabilities(
    getCapabilities_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::getCapabilitiesInternal, hidl_status_cb);
}

Return<void> WifiRttController::setLci(uint32_t cmd_id,
                                       const RttLciInformation& lci,
                                       setLci_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::setLciInternal, hidl_status_cb, cmd_id, lci);
}

Return<void> WifiRttController::setLcr(uint32_t cmd_id,
                                       const RttLcrInformation& lcr,
                                       setLcr_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::setLcrInternal, hidl_status_cb, cmd_id, lcr);
}

Return<void> WifiRttController::getResponderInfo(
    getResponderInfo_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::getResponderInfoInternal, hidl_status_cb);
}

Return<void> WifiRttController::enableResponder(
    uint32_t cmd_id, const WifiChannelInfo& channel_hint,
    uint32_t max_duration_seconds, const V1_0::RttResponder& info,
    enableResponder_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::enableResponderInternal, hidl_status_cb, cmd_id,
        channel_hint, max_duration_seconds, info);
}

Return<void> WifiRttController::disableResponder(
    uint32_t cmd_id, disableResponder_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::disableResponderInternal, hidl_status_cb, cmd_id);
}

Return<void> WifiRttController::registerEventCallback_1_4(
    const sp<IWifiRttControllerEventCallback>& callback,
    registerEventCallback_1_4_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::registerEventCallbackInternal_1_4, hidl_status_cb,
        callback);
}

Return<void> WifiRttController::rangeRequest_1_4(
    uint32_t cmd_id, const hidl_vec<RttConfig>& rtt_configs,
    rangeRequest_1_4_cb hidl_status_cb) {
    return validateAndCall(this,
                           WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::rangeRequestInternal_1_4,
                           hidl_status_cb, cmd_id, rtt_configs);
}

Return<void> WifiRttController::getCapabilities_1_4(
    getCapabilities_1_4_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::getCapabilitiesInternal_1_4, hidl_status_cb);
}

Return<void> WifiRttController::getResponderInfo_1_4(
    getResponderInfo_1_4_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::getResponderInfoInternal_1_4, hidl_status_cb);
}

Return<void> WifiRttController::enableResponder_1_4(
    uint32_t cmd_id, const WifiChannelInfo& channel_hint,
    uint32_t max_duration_seconds, const RttResponder& info,
    enableResponder_1_4_cb hidl_status_cb) {
    return validateAndCall(
        this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
        &WifiRttController::enableResponderInternal_1_4, hidl_status_cb, cmd_id,
        channel_hint, max_duration_seconds, info);
}

std::pair<WifiStatus, sp<IWifiIface>>
WifiRttController::getBoundIfaceInternal() {
    return {createWifiStatus(WifiStatusCode::SUCCESS), bound_iface_};
}

WifiStatus WifiRttController::registerEventCallbackInternal(
    const sp<V1_0::IWifiRttControllerEventCallback>& /* callback */) {
    // Deprecated support for this api
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiRttController::rangeRequestInternal(
    uint32_t /* cmd_id */,
    const std::vector<V1_0::RttConfig>& /* rtt_configs */) {
    // Deprecated support for this api
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

WifiStatus WifiRttController::rangeCancelInternal(
    uint32_t cmd_id, const std::vector<hidl_array<uint8_t, 6>>& addrs) {
    std::vector<std::array<uint8_t, 6>> legacy_addrs;
    for (const auto& addr : addrs) {
        legacy_addrs.push_back(addr);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->cancelRttRangeRequest(ifname_, cmd_id,
                                                  legacy_addrs);
    return createWifiStatusFromLegacyError(legacy_status);
}

std::pair<WifiStatus, V1_0::RttCapabilities>
WifiRttController::getCapabilitiesInternal() {
    // Deprecated support for this api
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), {}};
}

WifiStatus WifiRttController::setLciInternal(uint32_t cmd_id,
                                             const RttLciInformation& lci) {
    legacy_hal::wifi_lci_information legacy_lci;
    if (!hidl_struct_util::convertHidlRttLciInformationToLegacy(lci,
                                                                &legacy_lci)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->setRttLci(ifname_, cmd_id, legacy_lci);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiRttController::setLcrInternal(uint32_t cmd_id,
                                             const RttLcrInformation& lcr) {
    legacy_hal::wifi_lcr_information legacy_lcr;
    if (!hidl_struct_util::convertHidlRttLcrInformationToLegacy(lcr,
                                                                &legacy_lcr)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->setRttLcr(ifname_, cmd_id, legacy_lcr);
    return createWifiStatusFromLegacyError(legacy_status);
}

std::pair<WifiStatus, V1_0::RttResponder>
WifiRttController::getResponderInfoInternal() {
    // Deprecated support for this api
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED), {}};
}

WifiStatus WifiRttController::enableResponderInternal(
    uint32_t /* cmd_id */, const WifiChannelInfo& /* channel_hint */,
    uint32_t /* max_duration_seconds */, const V1_0::RttResponder& /* info */) {
    // Deprecated support for this api
    return {createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED)};
}

WifiStatus WifiRttController::disableResponderInternal(uint32_t cmd_id) {
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->disableRttResponder(ifname_, cmd_id);
    return createWifiStatusFromLegacyError(legacy_status);
}

WifiStatus WifiRttController::registerEventCallbackInternal_1_4(
    const sp<IWifiRttControllerEventCallback>& callback) {
    // TODO(b/31632518): remove the callback when the client is destroyed
    event_callbacks_.emplace_back(callback);
    return createWifiStatus(WifiStatusCode::SUCCESS);
}

WifiStatus WifiRttController::rangeRequestInternal_1_4(
    uint32_t cmd_id, const std::vector<RttConfig>& rtt_configs) {
    std::vector<legacy_hal::wifi_rtt_config> legacy_configs;
    if (!hidl_struct_util::convertHidlVectorOfRttConfigToLegacy(
            rtt_configs, &legacy_configs)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    android::wp<WifiRttController> weak_ptr_this(this);
    const auto& on_results_callback =
        [weak_ptr_this](
            legacy_hal::wifi_request_id id,
            const std::vector<const legacy_hal::wifi_rtt_result*>& results) {
            const auto shared_ptr_this = weak_ptr_this.promote();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            std::vector<RttResult> hidl_results;
            if (!hidl_struct_util::convertLegacyVectorOfRttResultToHidl(
                    results, &hidl_results)) {
                LOG(ERROR) << "Failed to convert rtt results to HIDL structs";
                return;
            }
            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                callback->onResults_1_4(id, hidl_results);
            }
        };
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->startRttRangeRequest(
            ifname_, cmd_id, legacy_configs, on_results_callback);
    return createWifiStatusFromLegacyError(legacy_status);
}

std::pair<WifiStatus, RttCapabilities>
WifiRttController::getCapabilitiesInternal_1_4() {
    legacy_hal::wifi_error legacy_status;
    legacy_hal::wifi_rtt_capabilities legacy_caps;
    std::tie(legacy_status, legacy_caps) =
        legacy_hal_.lock()->getRttCapabilities(ifname_);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        return {createWifiStatusFromLegacyError(legacy_status), {}};
    }
    RttCapabilities hidl_caps;
    if (!hidl_struct_util::convertLegacyRttCapabilitiesToHidl(legacy_caps,
                                                              &hidl_caps)) {
        return {createWifiStatus(WifiStatusCode::ERROR_UNKNOWN), {}};
    }
    return {createWifiStatus(WifiStatusCode::SUCCESS), hidl_caps};
}

std::pair<WifiStatus, RttResponder>
WifiRttController::getResponderInfoInternal_1_4() {
    legacy_hal::wifi_error legacy_status;
    legacy_hal::wifi_rtt_responder legacy_responder;
    std::tie(legacy_status, legacy_responder) =
        legacy_hal_.lock()->getRttResponderInfo(ifname_);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        return {createWifiStatusFromLegacyError(legacy_status), {}};
    }
    RttResponder hidl_responder;
    if (!hidl_struct_util::convertLegacyRttResponderToHidl(legacy_responder,
                                                           &hidl_responder)) {
        return {createWifiStatus(WifiStatusCode::ERROR_UNKNOWN), {}};
    }
    return {createWifiStatus(WifiStatusCode::SUCCESS), hidl_responder};
}

WifiStatus WifiRttController::enableResponderInternal_1_4(
    uint32_t cmd_id, const WifiChannelInfo& channel_hint,
    uint32_t max_duration_seconds, const RttResponder& info) {
    legacy_hal::wifi_channel_info legacy_channel_info;
    if (!hidl_struct_util::convertHidlWifiChannelInfoToLegacy(
            channel_hint, &legacy_channel_info)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_rtt_responder legacy_responder;
    if (!hidl_struct_util::convertHidlRttResponderToLegacy(info,
                                                           &legacy_responder)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
        legacy_hal_.lock()->enableRttResponder(
            ifname_, cmd_id, legacy_channel_info, max_duration_seconds,
            legacy_responder);
    return createWifiStatusFromLegacyError(legacy_status);
}
}  // namespace implementation
}  // namespace V1_4
}  // namespace wifi
}  // namespace hardware
}  // namespace android
