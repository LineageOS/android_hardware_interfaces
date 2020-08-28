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

#ifndef WIFI_RTT_CONTROLLER_H_
#define WIFI_RTT_CONTROLLER_H_

#include <android-base/macros.h>
#include <android/hardware/wifi/1.0/IWifiIface.h>
#include <android/hardware/wifi/1.4/IWifiRttController.h>
#include <android/hardware/wifi/1.4/IWifiRttControllerEventCallback.h>

#include "wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_4 {
namespace implementation {

/**
 * HIDL interface object used to control all RTT operations.
 */
class WifiRttController : public V1_4::IWifiRttController {
   public:
    WifiRttController(
        const std::string& iface_name, const sp<IWifiIface>& bound_iface,
        const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal);
    // Refer to |WifiChip::invalidate()|.
    void invalidate();
    bool isValid();
    std::vector<sp<IWifiRttControllerEventCallback>> getEventCallbacks();
    std::string getIfaceName();

    // HIDL methods exposed.
    Return<void> getBoundIface(getBoundIface_cb hidl_status_cb) override;
    Return<void> registerEventCallback(
        const sp<V1_0::IWifiRttControllerEventCallback>& callback,
        registerEventCallback_cb hidl_status_cb) override;
    Return<void> rangeRequest(uint32_t cmd_id,
                              const hidl_vec<V1_0::RttConfig>& rtt_configs,
                              rangeRequest_cb hidl_status_cb) override;
    Return<void> rangeCancel(uint32_t cmd_id,
                             const hidl_vec<hidl_array<uint8_t, 6>>& addrs,
                             rangeCancel_cb hidl_status_cb) override;
    Return<void> getCapabilities(getCapabilities_cb hidl_status_cb) override;
    Return<void> setLci(uint32_t cmd_id, const RttLciInformation& lci,
                        setLci_cb hidl_status_cb) override;
    Return<void> setLcr(uint32_t cmd_id, const RttLcrInformation& lcr,
                        setLcr_cb hidl_status_cb) override;
    Return<void> getResponderInfo(getResponderInfo_cb hidl_status_cb) override;
    Return<void> enableResponder(uint32_t cmd_id,
                                 const WifiChannelInfo& channel_hint,
                                 uint32_t max_duration_seconds,
                                 const V1_0::RttResponder& info,
                                 enableResponder_cb hidl_status_cb) override;
    Return<void> disableResponder(uint32_t cmd_id,
                                  disableResponder_cb hidl_status_cb) override;
    Return<void> registerEventCallback_1_4(
        const sp<IWifiRttControllerEventCallback>& callback,
        registerEventCallback_1_4_cb hidl_status_cb) override;
    Return<void> rangeRequest_1_4(uint32_t cmd_id,
                                  const hidl_vec<RttConfig>& rtt_configs,
                                  rangeRequest_1_4_cb hidl_status_cb) override;
    Return<void> getCapabilities_1_4(
        getCapabilities_1_4_cb hidl_status_cb) override;
    Return<void> getResponderInfo_1_4(
        getResponderInfo_1_4_cb hidl_status_cb) override;
    Return<void> enableResponder_1_4(
        uint32_t cmd_id, const WifiChannelInfo& channel_hint,
        uint32_t max_duration_seconds, const RttResponder& info,
        enableResponder_1_4_cb hidl_status_cb) override;

   private:
    // Corresponding worker functions for the HIDL methods.
    std::pair<WifiStatus, sp<IWifiIface>> getBoundIfaceInternal();
    WifiStatus registerEventCallbackInternal(
        const sp<V1_0::IWifiRttControllerEventCallback>& callback);
    WifiStatus rangeRequestInternal(
        uint32_t cmd_id, const std::vector<V1_0::RttConfig>& rtt_configs);
    WifiStatus rangeCancelInternal(
        uint32_t cmd_id, const std::vector<hidl_array<uint8_t, 6>>& addrs);
    std::pair<WifiStatus, V1_0::RttCapabilities> getCapabilitiesInternal();
    WifiStatus setLciInternal(uint32_t cmd_id, const RttLciInformation& lci);
    WifiStatus setLcrInternal(uint32_t cmd_id, const RttLcrInformation& lcr);
    std::pair<WifiStatus, V1_0::RttResponder> getResponderInfoInternal();
    WifiStatus enableResponderInternal(uint32_t cmd_id,
                                       const WifiChannelInfo& channel_hint,
                                       uint32_t max_duration_seconds,
                                       const V1_0::RttResponder& info);
    WifiStatus disableResponderInternal(uint32_t cmd_id);
    WifiStatus registerEventCallbackInternal_1_4(
        const sp<IWifiRttControllerEventCallback>& callback);
    WifiStatus rangeRequestInternal_1_4(
        uint32_t cmd_id, const std::vector<RttConfig>& rtt_configs);
    std::pair<WifiStatus, RttCapabilities> getCapabilitiesInternal_1_4();
    std::pair<WifiStatus, RttResponder> getResponderInfoInternal_1_4();
    WifiStatus enableResponderInternal_1_4(uint32_t cmd_id,
                                           const WifiChannelInfo& channel_hint,
                                           uint32_t max_duration_seconds,
                                           const RttResponder& info);

    std::string ifname_;
    sp<IWifiIface> bound_iface_;
    std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal_;
    std::vector<sp<IWifiRttControllerEventCallback>> event_callbacks_;
    bool is_valid_;

    DISALLOW_COPY_AND_ASSIGN(WifiRttController);
};

}  // namespace implementation
}  // namespace V1_4
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_RTT_CONTROLLER_H_
