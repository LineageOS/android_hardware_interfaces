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

#ifndef WIFI_RTT_CONTROLLER_H_
#define WIFI_RTT_CONTROLLER_H_

#include <aidl/android/hardware/wifi/BnWifiRttController.h>
#include <aidl/android/hardware/wifi/IWifiRttControllerEventCallback.h>
#include <aidl/android/hardware/wifi/IWifiStaIface.h>
#include <android-base/macros.h>

#include "wifi_legacy_hal.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {

/**
 * AIDL interface object used to control all RTT operations.
 */
class WifiRttController : public BnWifiRttController {
  public:
    WifiRttController(const std::string& iface_name,
                      const std::shared_ptr<IWifiStaIface>& bound_iface,
                      const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal);
    // Factory method - use instead of default constructor.
    static std::shared_ptr<WifiRttController> create(
            const std::string& iface_name, const std::shared_ptr<IWifiStaIface>& bound_iface,
            const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal);

    // Refer to |WifiChip::invalidate()|.
    void invalidate();
    bool isValid();
    std::vector<std::shared_ptr<IWifiRttControllerEventCallback>> getEventCallbacks();
    std::string getIfaceName();

    // AIDL methods exposed.
    ndk::ScopedAStatus getBoundIface(std::shared_ptr<IWifiStaIface>* _aidl_return) override;
    ndk::ScopedAStatus registerEventCallback(
            const std::shared_ptr<IWifiRttControllerEventCallback>& callback) override;
    ndk::ScopedAStatus rangeRequest(int32_t in_cmdId,
                                    const std::vector<RttConfig>& in_rttConfigs) override;
    ndk::ScopedAStatus rangeCancel(int32_t in_cmdId,
                                   const std::vector<MacAddress>& in_addrs) override;
    ndk::ScopedAStatus getCapabilities(RttCapabilities* _aidl_return) override;
    ndk::ScopedAStatus setLci(int32_t in_cmdId, const RttLciInformation& in_lci) override;
    ndk::ScopedAStatus setLcr(int32_t in_cmdId, const RttLcrInformation& in_lcr) override;
    ndk::ScopedAStatus getResponderInfo(RttResponder* _aidl_return) override;
    ndk::ScopedAStatus enableResponder(int32_t in_cmdId, const WifiChannelInfo& in_channelHint,
                                       int32_t in_maxDurationInSeconds,
                                       const RttResponder& in_info) override;
    ndk::ScopedAStatus disableResponder(int32_t in_cmdId) override;

  private:
    // Corresponding worker functions for the AIDL methods.
    std::pair<std::shared_ptr<IWifiStaIface>, ndk::ScopedAStatus> getBoundIfaceInternal();
    ndk::ScopedAStatus registerEventCallbackInternal(
            const std::shared_ptr<IWifiRttControllerEventCallback>& callback);
    ndk::ScopedAStatus rangeRequestInternal(int32_t cmd_id,
                                            const std::vector<RttConfig>& rtt_configs);
    ndk::ScopedAStatus rangeCancelInternal(int32_t cmd_id, const std::vector<MacAddress>& addrs);
    std::pair<RttCapabilities, ndk::ScopedAStatus> getCapabilitiesInternal();
    ndk::ScopedAStatus setLciInternal(int32_t cmd_id, const RttLciInformation& lci);
    ndk::ScopedAStatus setLcrInternal(int32_t cmd_id, const RttLcrInformation& lcr);
    std::pair<RttResponder, ndk::ScopedAStatus> getResponderInfoInternal();
    ndk::ScopedAStatus enableResponderInternal(int32_t cmd_id, const WifiChannelInfo& channel_hint,
                                               int32_t max_duration_seconds,
                                               const RttResponder& info);
    ndk::ScopedAStatus disableResponderInternal(int32_t cmd_id);

    void setWeakPtr(std::weak_ptr<WifiRttController> ptr);

    std::string ifname_;
    std::shared_ptr<IWifiStaIface> bound_iface_;
    std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal_;
    std::vector<std::shared_ptr<IWifiRttControllerEventCallback>> event_callbacks_;
    std::weak_ptr<WifiRttController> weak_ptr_this_;
    bool is_valid_;

    DISALLOW_COPY_AND_ASSIGN(WifiRttController);
};

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // WIFI_RTT_CONTROLLER_H_
