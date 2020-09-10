/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Staache License, Version 2.0 (the "License");
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

#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/1.0/IWifiStaIface.h>
#include <android/hardware/wifi/1.3/IWifiStaIface.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::wifi::V1_0::Bssid;
using ::android::hardware::wifi::V1_0::CommandId;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::IWifi;
using ::android::hardware::wifi::V1_0::IWifiStaIface;
using ::android::hardware::wifi::V1_0::Rssi;
using ::android::hardware::wifi::V1_0::Ssid;
using ::android::hardware::wifi::V1_0::StaApfPacketFilterCapabilities;
using ::android::hardware::wifi::V1_0::StaRoamingConfig;
using ::android::hardware::wifi::V1_0::StaRoamingState;
using ::android::hardware::wifi::V1_0::WifiBand;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;

/**
 * Fixture to use for all STA Iface HIDL interface tests.
 */
class WifiStaIfaceHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        // Make sure test starts with a clean state
        stopWifi(GetInstanceName());

        wifi_sta_iface_ = getWifiStaIface(GetInstanceName());
        ASSERT_NE(nullptr, wifi_sta_iface_.get());
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

   protected:
    bool isCapabilitySupported(IWifiStaIface::StaIfaceCapabilityMask cap_mask) {
        const auto& status_and_caps =
            HIDL_INVOKE(wifi_sta_iface_, getCapabilities);
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
        return (status_and_caps.second & cap_mask) != 0;
    }

    sp<IWifiStaIface> wifi_sta_iface_;
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * Create:
 * Ensures that an instance of the IWifiStaIface proxy object is
 * successfully created.
 */
TEST_P(WifiStaIfaceHidlTest, Create) {
    // The creation of a proxy object is tested as part of SetUp method.
}

/*
 * GetCapabilities:
 */
TEST_P(WifiStaIfaceHidlTest, GetCapabilities) {
    const auto& status_and_caps = HIDL_INVOKE(wifi_sta_iface_, getCapabilities);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
    EXPECT_GT(status_and_caps.second, 0u);
}

/*
 * GetType:
 * Ensures that the correct interface type is returned for station interface.
 */
TEST_P(WifiStaIfaceHidlTest, GetType) {
    const auto& status_and_type = HIDL_INVOKE(wifi_sta_iface_, getType);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_type.first.code);
    EXPECT_EQ(IfaceType::STA, status_and_type.second);
}

/*
 * GetApfPacketFilterCapabilities:
 * Ensures that we can retrieve APF packet filter capabilites.
 */
TEST_P(WifiStaIfaceHidlTest, GetApfPacketFilterCapabilities) {
    if (!isCapabilitySupported(IWifiStaIface::StaIfaceCapabilityMask::APF)) {
        // No-op if APF packet filer is not supported.
        return;
    }

    const auto& status_and_caps =
        HIDL_INVOKE(wifi_sta_iface_, getApfPacketFilterCapabilities);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
}

/*
 * GetBackgroundScanCapabilities:
 * Ensures that we can retrieve background scan capabilities.
 */
TEST_P(WifiStaIfaceHidlTest, GetBackgroundScanCapabilities) {
    if (!isCapabilitySupported(
            IWifiStaIface::StaIfaceCapabilityMask::BACKGROUND_SCAN)) {
        // No-op if background scan is not supported.
        return;
    }

    const auto& status_and_caps =
        HIDL_INVOKE(wifi_sta_iface_, getBackgroundScanCapabilities);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
}

/*
 * GetValidFrequenciesForBand:
 * Ensures that we can retrieve valid frequencies for 2.4 GHz band.
 */
TEST_P(WifiStaIfaceHidlTest, GetValidFrequenciesForBand) {
    const auto& status_and_freqs = HIDL_INVOKE(
        wifi_sta_iface_, getValidFrequenciesForBand, WifiBand::BAND_24GHZ);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_freqs.first.code);
    EXPECT_GT(status_and_freqs.second.size(), 0u);
}

/*
 * LinkLayerStatsCollection:
 * Ensures that calls to enable, disable, and retrieve link layer stats
 * will return a success status code.
 */
TEST_P(WifiStaIfaceHidlTest, LinkLayerStatsCollection) {
    if (!isCapabilitySupported(
            IWifiStaIface::StaIfaceCapabilityMask::LINK_LAYER_STATS)) {
        // No-op if link layer stats is not supported.
        return;
    }

    sp<::android::hardware::wifi::V1_3::IWifiStaIface> iface_converted =
        ::android::hardware::wifi::V1_3::IWifiStaIface::castFrom(
            wifi_sta_iface_);
    if (iface_converted != nullptr) {
        // Skip this test since this API is deprecated in this newer HAL version
        return;
    }

    // Enable link layer stats collection.
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_sta_iface_, enableLinkLayerStatsCollection, true)
                  .code);
    // Retrieve link layer stats.
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_sta_iface_, getLinkLayerStats).first.code);
    // Disable link layer stats collection.
    EXPECT_EQ(
        WifiStatusCode::SUCCESS,
        HIDL_INVOKE(wifi_sta_iface_, disableLinkLayerStatsCollection).code);
}

/*
 * RSSIMonitoring:
 * Ensures that calls to enable RSSI monitoring will return an error status
 * code if device is not connected to an AP.
 * Ensures that calls to disable RSSI monitoring will return an error status
 * code if RSSI monitoring is not enabled.
 */
TEST_P(WifiStaIfaceHidlTest, RSSIMonitoring) {
    if (!isCapabilitySupported(
            IWifiStaIface::StaIfaceCapabilityMask::RSSI_MONITOR)) {
        // No-op if RSSI monitor is not supported.
        return;
    }

    const CommandId kCmd = 1;
    const Rssi kMaxRssi = -50;
    const Rssi kMinRssi = -90;
    // This is going to fail because device is not connected to an AP.
    EXPECT_NE(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_sta_iface_, startRssiMonitoring, kCmd, kMaxRssi,
                          kMinRssi)
                  .code);
    // This is going to fail because RSSI monitoring is not enabled.
    EXPECT_NE(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_sta_iface_, stopRssiMonitoring, kCmd).code);
}

/*
 * RoamingControl:
 * Ensures that calls to configure and enable roaming will return a success
 * status code.
 */
TEST_P(WifiStaIfaceHidlTest, RoamingControl) {
    if (!isCapabilitySupported(
            IWifiStaIface::StaIfaceCapabilityMask::CONTROL_ROAMING)) {
        // No-op if roaming control is not supported.
        return;
    }

    // Retrieve roaming capabilities.
    const auto& status_and_cap =
        HIDL_INVOKE(wifi_sta_iface_, getRoamingCapabilities);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_cap.first.code);

    // Setup roaming configuration based on roaming capabilities.
    const auto& cap = status_and_cap.second;
    StaRoamingConfig roaming_config;
    if (cap.maxBlacklistSize > 0) {
        Bssid black_list_bssid{
            std::array<uint8_t, 6>{{0x11, 0x22, 0x33, 0x44, 0x55, 0x66}}};
        roaming_config.bssidBlacklist =
            android::hardware::hidl_vec<Bssid>{black_list_bssid};
    }
    if (cap.maxWhitelistSize > 0) {
        Ssid white_list_ssid{
            std::array<uint8_t, 32>{{0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC}}};
        roaming_config.ssidWhitelist =
            android::hardware::hidl_vec<Ssid>{white_list_ssid};
    }

    // Configure roaming.
    EXPECT_EQ(
        WifiStatusCode::SUCCESS,
        HIDL_INVOKE(wifi_sta_iface_, configureRoaming, roaming_config).code);

    // Enable roaming.
    EXPECT_EQ(
        WifiStatusCode::SUCCESS,
        HIDL_INVOKE(wifi_sta_iface_, setRoamingState, StaRoamingState::ENABLED)
            .code);
}

/*
 * EnableNDOffload:
 * Ensures that calls to enable neighbor discovery offload will return a success
 * status code.
 */
TEST_P(WifiStaIfaceHidlTest, EnableNDOffload) {
    if (!isCapabilitySupported(
            IWifiStaIface::StaIfaceCapabilityMask::ND_OFFLOAD)) {
        // No-op if nd offload is not supported.
        return;
    }
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_sta_iface_, enableNdOffload, true).code);
}

/*
 * SetScanningMacOui:
 * Ensures that calls to set scanning MAC OUI will return a NOT_SUPPORTED
 * code since it is now deprecated.
 */
TEST_P(WifiStaIfaceHidlTest, SetScanningMacOui) {
    if (!isCapabilitySupported(
            IWifiStaIface::StaIfaceCapabilityMask::SCAN_RAND)) {
        // No-op if SetScanningMacOui is not supported.
        return;
    }
    const android::hardware::hidl_array<uint8_t, 3> kOui{
        std::array<uint8_t, 3>{{0x10, 0x22, 0x33}}};
    EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED,
              HIDL_INVOKE(wifi_sta_iface_, setScanningMacOui, kOui).code);
}

/*
 * PacketFateMonitoring:
 * Ensures that calls to start packet fate monitoring and retrieve TX/RX
 * packets will return a success status code.
 */
TEST_P(WifiStaIfaceHidlTest, PacketFateMonitoring) {
    if (!isCapabilitySupported(
            IWifiStaIface::StaIfaceCapabilityMask::DEBUG_PACKET_FATE)) {
        // No-op if packet fate monitor is not supported.
        return;
    }
    // Start packet fate monitoring.
    EXPECT_EQ(
        WifiStatusCode::SUCCESS,
        HIDL_INVOKE(wifi_sta_iface_, startDebugPacketFateMonitoring).code);

    // Retrieve packets.
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_sta_iface_, getDebugTxPacketFates).first.code);
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_sta_iface_, getDebugRxPacketFates).first.code);
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiStaIfaceHidlTest,
    testing::ValuesIn(
        android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
