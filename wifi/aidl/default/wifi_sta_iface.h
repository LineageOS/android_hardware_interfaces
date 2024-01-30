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

#ifndef WIFI_STA_IFACE_H_
#define WIFI_STA_IFACE_H_

#include <aidl/android/hardware/wifi/BnWifiStaIface.h>
#include <aidl/android/hardware/wifi/IWifiStaIfaceEventCallback.h>
#include <android-base/macros.h>

#include "aidl_callback_util.h"
#include "wifi_iface_util.h"
#include "wifi_legacy_hal.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {

/**
 * AIDL interface object used to control a STA Iface instance.
 */
class WifiStaIface : public BnWifiStaIface {
  public:
    WifiStaIface(const std::string& ifname,
                 const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
                 const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util);

    // Factory method - use instead of default constructor.
    static std::shared_ptr<WifiStaIface> create(
            const std::string& ifname, const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
            const std::weak_ptr<iface_util::WifiIfaceUtil> iface_util);

    // Refer to |WifiChip::invalidate()|.
    void invalidate();
    bool isValid();
    std::set<std::shared_ptr<IWifiStaIfaceEventCallback>> getEventCallbacks();
    std::string getName();

    // AIDL methods exposed.
    ndk::ScopedAStatus getName(std::string* _aidl_return) override;
    ndk::ScopedAStatus registerEventCallback(
            const std::shared_ptr<IWifiStaIfaceEventCallback>& in_callback) override;
    ndk::ScopedAStatus getFeatureSet(int32_t* _aidl_return) override;
    ndk::ScopedAStatus getApfPacketFilterCapabilities(
            StaApfPacketFilterCapabilities* _aidl_return) override;
    ndk::ScopedAStatus installApfPacketFilter(const std::vector<uint8_t>& in_program) override;
    ndk::ScopedAStatus readApfPacketFilterData(std::vector<uint8_t>* _aidl_return) override;
    ndk::ScopedAStatus getBackgroundScanCapabilities(
            StaBackgroundScanCapabilities* _aidl_return) override;
    ndk::ScopedAStatus startBackgroundScan(int32_t in_cmdId,
                                           const StaBackgroundScanParameters& in_params) override;
    ndk::ScopedAStatus stopBackgroundScan(int32_t in_cmdId) override;
    ndk::ScopedAStatus enableLinkLayerStatsCollection(bool in_debug) override;
    ndk::ScopedAStatus disableLinkLayerStatsCollection() override;
    ndk::ScopedAStatus getLinkLayerStats(StaLinkLayerStats* _aidl_return) override;
    ndk::ScopedAStatus startRssiMonitoring(int32_t in_cmdId, int32_t in_maxRssi,
                                           int32_t in_minRssi) override;
    ndk::ScopedAStatus stopRssiMonitoring(int32_t in_cmdId) override;
    ndk::ScopedAStatus getRoamingCapabilities(StaRoamingCapabilities* _aidl_return) override;
    ndk::ScopedAStatus configureRoaming(const StaRoamingConfig& in_config) override;
    ndk::ScopedAStatus setRoamingState(StaRoamingState in_state) override;
    ndk::ScopedAStatus enableNdOffload(bool in_enable) override;
    ndk::ScopedAStatus startSendingKeepAlivePackets(int32_t in_cmdId,
                                                    const std::vector<uint8_t>& in_ipPacketData,
                                                    char16_t in_etherType,
                                                    const std::array<uint8_t, 6>& in_srcAddress,
                                                    const std::array<uint8_t, 6>& in_dstAddress,
                                                    int32_t in_periodInMs) override;
    ndk::ScopedAStatus stopSendingKeepAlivePackets(int32_t in_cmdId) override;
    ndk::ScopedAStatus startDebugPacketFateMonitoring() override;
    ndk::ScopedAStatus getDebugTxPacketFates(
            std::vector<WifiDebugTxPacketFateReport>* _aidl_return) override;
    ndk::ScopedAStatus getDebugRxPacketFates(
            std::vector<WifiDebugRxPacketFateReport>* _aidl_return) override;
    ndk::ScopedAStatus setMacAddress(const std::array<uint8_t, 6>& in_mac) override;
    ndk::ScopedAStatus getFactoryMacAddress(std::array<uint8_t, 6>* _aidl_return) override;
    ndk::ScopedAStatus setScanMode(bool in_enable) override;
    ndk::ScopedAStatus setDtimMultiplier(int32_t in_multiplier) override;
    ndk::ScopedAStatus getCachedScanData(CachedScanData* _aidl_return) override;
    ndk::ScopedAStatus twtGetCapabilities(TwtCapabilities* _aidl_return) override;
    ndk::ScopedAStatus twtSessionSetup(int in_cmdId, const TwtRequest& in_twtRequest) override;
    ndk::ScopedAStatus twtSessionUpdate(int in_cmdId, int32_t in_sessionId,
                                        const TwtRequest& in_twtRequest) override;
    ndk::ScopedAStatus twtSessionSuspend(int in_cmdId, int32_t in_sessionId) override;
    ndk::ScopedAStatus twtSessionResume(int in_cmdId, int32_t in_sessionId) override;
    ndk::ScopedAStatus twtSessionTeardown(int in_cmdId, int32_t in_sessionId) override;
    ndk::ScopedAStatus twtSessionGetStats(int in_cmdId, int32_t in_sessionId) override;

  private:
    // Corresponding worker functions for the AIDL methods.
    std::pair<std::string, ndk::ScopedAStatus> getNameInternal();
    ndk::ScopedAStatus registerEventCallbackInternal(
            const std::shared_ptr<IWifiStaIfaceEventCallback>& callback);
    std::pair<int32_t, ndk::ScopedAStatus> getFeatureSetInternal();
    std::pair<StaApfPacketFilterCapabilities, ndk::ScopedAStatus>
    getApfPacketFilterCapabilitiesInternal();
    ndk::ScopedAStatus installApfPacketFilterInternal(const std::vector<uint8_t>& program);
    std::pair<std::vector<uint8_t>, ndk::ScopedAStatus> readApfPacketFilterDataInternal();
    std::pair<StaBackgroundScanCapabilities, ndk::ScopedAStatus>
    getBackgroundScanCapabilitiesInternal();
    ndk::ScopedAStatus startBackgroundScanInternal(int32_t cmd_id,
                                                   const StaBackgroundScanParameters& params);
    ndk::ScopedAStatus stopBackgroundScanInternal(int32_t cmd_id);
    ndk::ScopedAStatus enableLinkLayerStatsCollectionInternal(bool debug);
    ndk::ScopedAStatus disableLinkLayerStatsCollectionInternal();
    std::pair<StaLinkLayerStats, ndk::ScopedAStatus> getLinkLayerStatsInternal();
    ndk::ScopedAStatus startRssiMonitoringInternal(int32_t cmd_id, int32_t max_rssi,
                                                   int32_t min_rssi);
    ndk::ScopedAStatus stopRssiMonitoringInternal(int32_t cmd_id);
    std::pair<StaRoamingCapabilities, ndk::ScopedAStatus> getRoamingCapabilitiesInternal();
    ndk::ScopedAStatus configureRoamingInternal(const StaRoamingConfig& config);
    ndk::ScopedAStatus setRoamingStateInternal(StaRoamingState state);
    ndk::ScopedAStatus enableNdOffloadInternal(bool enable);
    ndk::ScopedAStatus startSendingKeepAlivePacketsInternal(
            int32_t cmd_id, const std::vector<uint8_t>& ip_packet_data, char16_t ether_type,
            const std::array<uint8_t, 6>& src_address, const std::array<uint8_t, 6>& dst_address,
            int32_t period_in_ms);
    ndk::ScopedAStatus stopSendingKeepAlivePacketsInternal(int32_t cmd_id);
    ndk::ScopedAStatus startDebugPacketFateMonitoringInternal();
    std::pair<std::vector<WifiDebugTxPacketFateReport>, ndk::ScopedAStatus>
    getDebugTxPacketFatesInternal();
    std::pair<std::vector<WifiDebugRxPacketFateReport>, ndk::ScopedAStatus>
    getDebugRxPacketFatesInternal();
    ndk::ScopedAStatus setMacAddressInternal(const std::array<uint8_t, 6>& mac);
    std::pair<std::array<uint8_t, 6>, ndk::ScopedAStatus> getFactoryMacAddressInternal();
    ndk::ScopedAStatus setScanModeInternal(bool enable);
    ndk::ScopedAStatus setDtimMultiplierInternal(const int multiplier);
    std::pair<CachedScanData, ndk::ScopedAStatus> getCachedScanDataInternal();
    std::pair<TwtCapabilities, ndk::ScopedAStatus> twtGetCapabilitiesInternal();
    ndk::ScopedAStatus twtSessionSetupInternal(int cmdId, const TwtRequest& twtRequest);
    ndk::ScopedAStatus twtSessionUpdateInternal(int cmdId, int32_t sessionId,
                                                const TwtRequest& twtRequest);
    ndk::ScopedAStatus twtSessionSuspendInternal(int cmdId, int32_t sessionId);
    ndk::ScopedAStatus twtSessionResumeInternal(int cmdId, int32_t sessionId);
    ndk::ScopedAStatus twtSessionTeardownInternal(int cmdId, int32_t sessionId);
    ndk::ScopedAStatus twtSessionGetStatsInternal(int cmdId, int32_t sessionId);

    void setWeakPtr(std::weak_ptr<WifiStaIface> ptr);

    std::string ifname_;
    std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal_;
    std::weak_ptr<iface_util::WifiIfaceUtil> iface_util_;
    std::weak_ptr<WifiStaIface> weak_ptr_this_;
    bool is_valid_;
    aidl_callback_util::AidlCallbackHandler<IWifiStaIfaceEventCallback> event_cb_handler_;

    DISALLOW_COPY_AND_ASSIGN(WifiStaIface);
};

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // WIFI_STA_IFACE_H_
