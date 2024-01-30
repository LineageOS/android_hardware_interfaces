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

#include "wifi_rtt_controller.h"

#include <android-base/logging.h>

#include "aidl_return_util.h"
#include "aidl_struct_util.h"
#include "wifi_status_util.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
using aidl_return_util::validateAndCall;

WifiRttController::WifiRttController(const std::string& iface_name,
                                     const std::shared_ptr<IWifiStaIface>& bound_iface,
                                     const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal)
    : ifname_(iface_name), bound_iface_(bound_iface), legacy_hal_(legacy_hal), is_valid_(true) {}

std::shared_ptr<WifiRttController> WifiRttController::create(
        const std::string& iface_name, const std::shared_ptr<IWifiStaIface>& bound_iface,
        const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal) {
    std::shared_ptr<WifiRttController> ptr =
            ndk::SharedRefBase::make<WifiRttController>(iface_name, bound_iface, legacy_hal);
    std::weak_ptr<WifiRttController> weak_ptr_this(ptr);
    ptr->setWeakPtr(weak_ptr_this);
    return ptr;
}

void WifiRttController::invalidate() {
    legacy_hal_.reset();
    event_callbacks_.clear();
    is_valid_ = false;
};

bool WifiRttController::isValid() {
    return is_valid_;
}

void WifiRttController::setWeakPtr(std::weak_ptr<WifiRttController> ptr) {
    weak_ptr_this_ = ptr;
}

std::vector<std::shared_ptr<IWifiRttControllerEventCallback>>
WifiRttController::getEventCallbacks() {
    return event_callbacks_;
}

std::string WifiRttController::getIfaceName() {
    return ifname_;
}

ndk::ScopedAStatus WifiRttController::getBoundIface(std::shared_ptr<IWifiStaIface>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::getBoundIfaceInternal, _aidl_return);
}

ndk::ScopedAStatus WifiRttController::registerEventCallback(
        const std::shared_ptr<IWifiRttControllerEventCallback>& callback) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::registerEventCallbackInternal, callback);
}

ndk::ScopedAStatus WifiRttController::rangeRequest(int32_t in_cmdId,
                                                   const std::vector<RttConfig>& in_rttConfigs) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::rangeRequestInternal, in_cmdId, in_rttConfigs);
}

ndk::ScopedAStatus WifiRttController::rangeCancel(int32_t in_cmdId,
                                                  const std::vector<MacAddress>& in_addrs) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::rangeCancelInternal, in_cmdId, in_addrs);
}

ndk::ScopedAStatus WifiRttController::getCapabilities(RttCapabilities* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::getCapabilitiesInternal, _aidl_return);
}

ndk::ScopedAStatus WifiRttController::setLci(int32_t in_cmdId, const RttLciInformation& in_lci) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::setLciInternal, in_cmdId, in_lci);
}

ndk::ScopedAStatus WifiRttController::setLcr(int32_t in_cmdId, const RttLcrInformation& in_lcr) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::setLcrInternal, in_cmdId, in_lcr);
}

ndk::ScopedAStatus WifiRttController::getResponderInfo(RttResponder* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::getResponderInfoInternal, _aidl_return);
}

ndk::ScopedAStatus WifiRttController::enableResponder(int32_t in_cmdId,
                                                      const WifiChannelInfo& in_channelHint,
                                                      int32_t in_maxDurationInSeconds,
                                                      const RttResponder& in_info) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::enableResponderInternal, in_cmdId, in_channelHint,
                           in_maxDurationInSeconds, in_info);
}

ndk::ScopedAStatus WifiRttController::disableResponder(int32_t in_cmdId) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_RTT_CONTROLLER_INVALID,
                           &WifiRttController::disableResponderInternal, in_cmdId);
}

std::pair<std::shared_ptr<IWifiStaIface>, ndk::ScopedAStatus>
WifiRttController::getBoundIfaceInternal() {
    return {bound_iface_, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiRttController::registerEventCallbackInternal(
        const std::shared_ptr<IWifiRttControllerEventCallback>& callback) {
    event_callbacks_.emplace_back(callback);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WifiRttController::rangeRequestInternal(
        int32_t cmd_id, const std::vector<RttConfig>& rtt_configs) {
    // Try 11mc & 11az ranging (v3)
    std::vector<legacy_hal::wifi_rtt_config_v3> legacy_configs_v3;
    if (!aidl_struct_util::convertAidlVectorOfRttConfigToLegacyV3(rtt_configs,
                                                                  &legacy_configs_v3)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    std::weak_ptr<WifiRttController> weak_ptr_this = weak_ptr_this_;
    const auto& on_results_callback_v3 =
            [weak_ptr_this](legacy_hal::wifi_request_id id,
                            const std::vector<const legacy_hal::wifi_rtt_result_v3*>& results) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "v3 Callback invoked on an invalid object";
                    return;
                }
                std::vector<RttResult> aidl_results;
                if (!aidl_struct_util::convertLegacyVectorOfRttResultV3ToAidl(results,
                                                                              &aidl_results)) {
                    LOG(ERROR) << "Failed to convert rtt results v3 to AIDL structs";
                    return;
                }
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->onResults(id, aidl_results).isOk()) {
                        LOG(ERROR) << "Failed to invoke the v3 callback";
                    }
                }
            };
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->startRttRangeRequestV3(
            ifname_, cmd_id, legacy_configs_v3, on_results_callback_v3);

    if (legacy_status != legacy_hal::WIFI_ERROR_NOT_SUPPORTED) {
        return createWifiStatusFromLegacyError(legacy_status);
    }

    // Fallback to 11mc ranging.
    std::vector<legacy_hal::wifi_rtt_config> legacy_configs;
    if (!aidl_struct_util::convertAidlVectorOfRttConfigToLegacy(rtt_configs, &legacy_configs)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    const auto& on_results_callback =
            [weak_ptr_this](legacy_hal::wifi_request_id id,
                            const std::vector<const legacy_hal::wifi_rtt_result*>& results) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                std::vector<RttResult> aidl_results;
                if (!aidl_struct_util::convertLegacyVectorOfRttResultToAidl(results,
                                                                            &aidl_results)) {
                    LOG(ERROR) << "Failed to convert rtt results to AIDL structs";
                    return;
                }
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->onResults(id, aidl_results).isOk()) {
                        LOG(ERROR) << "Failed to invoke the callback";
                    }
                }
            };
    const auto& on_results_callback_v2 =
            [weak_ptr_this](legacy_hal::wifi_request_id id,
                            const std::vector<const legacy_hal::wifi_rtt_result_v2*>& results) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "v2 Callback invoked on an invalid object";
                    return;
                }
                std::vector<RttResult> aidl_results;
                if (!aidl_struct_util::convertLegacyVectorOfRttResultV2ToAidl(results,
                                                                              &aidl_results)) {
                    LOG(ERROR) << "Failed to convert rtt results v2 to AIDL structs";
                    return;
                }
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->onResults(id, aidl_results).isOk()) {
                        LOG(ERROR) << "Failed to invoke the v2 callback";
                    }
                }
            };
    legacy_status = legacy_hal_.lock()->startRttRangeRequest(
            ifname_, cmd_id, legacy_configs, on_results_callback, on_results_callback_v2);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiRttController::rangeCancelInternal(int32_t cmd_id,
                                                          const std::vector<MacAddress>& addrs) {
    std::vector<std::array<uint8_t, ETH_ALEN>> legacy_addrs;
    for (const auto& addr : addrs) {
        std::array<uint8_t, ETH_ALEN> addr_array;
        std::copy_n(addr.data.begin(), ETH_ALEN, addr_array.begin());
        legacy_addrs.push_back(addr_array);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->cancelRttRangeRequest(ifname_, cmd_id, legacy_addrs);
    return createWifiStatusFromLegacyError(legacy_status);
}

std::pair<RttCapabilities, ndk::ScopedAStatus> WifiRttController::getCapabilitiesInternal() {
    legacy_hal::wifi_error legacy_status;
    legacy_hal::wifi_rtt_capabilities_v3 legacy_caps_v3;
    std::tie(legacy_status, legacy_caps_v3) = legacy_hal_.lock()->getRttCapabilitiesV3(ifname_);
    // Try v3 API first, if it is not supported fallback.
    if (legacy_status == legacy_hal::WIFI_ERROR_NOT_SUPPORTED) {
        legacy_hal::wifi_rtt_capabilities legacy_caps;
        std::tie(legacy_status, legacy_caps) = legacy_hal_.lock()->getRttCapabilities(ifname_);
        if (legacy_status != legacy_hal::WIFI_SUCCESS) {
            return {RttCapabilities{}, createWifiStatusFromLegacyError(legacy_status)};
        }

        RttCapabilities aidl_caps;
        if (!aidl_struct_util::convertLegacyRttCapabilitiesToAidl(legacy_caps, &aidl_caps)) {
            return {RttCapabilities{}, createWifiStatus(WifiStatusCode::ERROR_UNKNOWN)};
        }
        return {aidl_caps, ndk::ScopedAStatus::ok()};
    }

    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        return {RttCapabilities{}, createWifiStatusFromLegacyError(legacy_status)};
    }

    RttCapabilities aidl_caps;
    if (!aidl_struct_util::convertLegacyRttCapabilitiesV3ToAidl(legacy_caps_v3, &aidl_caps)) {
        return {RttCapabilities{}, createWifiStatus(WifiStatusCode::ERROR_UNKNOWN)};
    }
    return {aidl_caps, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiRttController::setLciInternal(int32_t cmd_id, const RttLciInformation& lci) {
    legacy_hal::wifi_lci_information legacy_lci;
    if (!aidl_struct_util::convertAidlRttLciInformationToLegacy(lci, &legacy_lci)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->setRttLci(ifname_, cmd_id, legacy_lci);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiRttController::setLcrInternal(int32_t cmd_id, const RttLcrInformation& lcr) {
    legacy_hal::wifi_lcr_information legacy_lcr;
    if (!aidl_struct_util::convertAidlRttLcrInformationToLegacy(lcr, &legacy_lcr)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->setRttLcr(ifname_, cmd_id, legacy_lcr);
    return createWifiStatusFromLegacyError(legacy_status);
}

std::pair<RttResponder, ndk::ScopedAStatus> WifiRttController::getResponderInfoInternal() {
    legacy_hal::wifi_error legacy_status;
    legacy_hal::wifi_rtt_responder legacy_responder;
    std::tie(legacy_status, legacy_responder) = legacy_hal_.lock()->getRttResponderInfo(ifname_);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        return {RttResponder{}, createWifiStatusFromLegacyError(legacy_status)};
    }
    RttResponder aidl_responder;
    if (!aidl_struct_util::convertLegacyRttResponderToAidl(legacy_responder, &aidl_responder)) {
        return {RttResponder{}, createWifiStatus(WifiStatusCode::ERROR_UNKNOWN)};
    }
    return {aidl_responder, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiRttController::enableResponderInternal(int32_t cmd_id,
                                                              const WifiChannelInfo& channel_hint,
                                                              int32_t max_duration_seconds,
                                                              const RttResponder& info) {
    legacy_hal::wifi_channel_info legacy_channel_info;
    if (!aidl_struct_util::convertAidlWifiChannelInfoToLegacy(channel_hint, &legacy_channel_info)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_rtt_responder legacy_responder;
    if (!aidl_struct_util::convertAidlRttResponderToLegacy(info, &legacy_responder)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->enableRttResponder(
            ifname_, cmd_id, legacy_channel_info, max_duration_seconds, legacy_responder);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiRttController::disableResponderInternal(int32_t cmd_id) {
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->disableRttResponder(ifname_, cmd_id);
    return createWifiStatusFromLegacyError(legacy_status);
}

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl
