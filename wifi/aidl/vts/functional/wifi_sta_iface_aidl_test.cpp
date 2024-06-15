/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <cctype>
#include <vector>

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/wifi/BnWifi.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>

#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::CachedScanData;
using aidl::android::hardware::wifi::IWifi;
using aidl::android::hardware::wifi::IWifiStaIface;
using aidl::android::hardware::wifi::MacAddress;
using aidl::android::hardware::wifi::Ssid;
using aidl::android::hardware::wifi::StaApfPacketFilterCapabilities;
using aidl::android::hardware::wifi::StaBackgroundScanCapabilities;
using aidl::android::hardware::wifi::StaLinkLayerStats;
using aidl::android::hardware::wifi::StaRoamingCapabilities;
using aidl::android::hardware::wifi::StaRoamingConfig;
using aidl::android::hardware::wifi::StaRoamingState;
using aidl::android::hardware::wifi::TwtCapabilities;
using aidl::android::hardware::wifi::TwtRequest;
using aidl::android::hardware::wifi::WifiBand;
using aidl::android::hardware::wifi::WifiDebugRxPacketFateReport;
using aidl::android::hardware::wifi::WifiDebugTxPacketFateReport;
using aidl::android::hardware::wifi::WifiStatusCode;

class WifiStaIfaceAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        stopWifiService(getInstanceName());
        wifi_sta_iface_ = getWifiStaIface(getInstanceName());
        ASSERT_NE(nullptr, wifi_sta_iface_.get());
        ASSERT_TRUE(wifi_sta_iface_->getInterfaceVersion(&interface_version_).isOk());
    }

    void TearDown() override { stopWifiService(getInstanceName()); }

  protected:
    bool isFeatureSupported(IWifiStaIface::FeatureSetMask expected) {
        int32_t features = 0;
        EXPECT_TRUE(wifi_sta_iface_->getFeatureSet(&features).isOk());
        return features & static_cast<int32_t>(expected);
    }

    ndk::ScopedAStatus createStaIface(std::shared_ptr<IWifiStaIface>* sta_iface) {
        std::shared_ptr<IWifiChip> wifi_chip = getWifiChip(getInstanceName());
        EXPECT_NE(nullptr, wifi_chip.get());
        return wifi_chip->createStaIface(sta_iface);
    }

    std::shared_ptr<IWifiStaIface> wifi_sta_iface_;
    int interface_version_;

    // Checks if the mDNS Offload is supported by any NIC.
    bool isMdnsOffloadPresentInNIC() {
        return testing::deviceSupportsFeature("com.google.android.tv.mdns_offload");
    }

    bool doesDeviceSupportFullNetworkingUnder2w() {
        return testing::deviceSupportsFeature("com.google.android.tv.full_networking_under_2w");
    }

    // Detect TV devices.
    bool isTvDevice() {
        return testing::deviceSupportsFeature("android.software.leanback") ||
               testing::deviceSupportsFeature("android.hardware.type.television");
    }

    // Detect Panel TV devices by using ro.oem.key1 property.
    // https://docs.partner.android.com/tv/build/platform/props-vars/ro-oem-key1
    bool isPanelTvDevice() {
        const std::string oem_key1 = getPropertyString("ro.oem.key1");
        if (oem_key1.size() < 9) {
            return false;
        }
        if (oem_key1.substr(0, 3) != "ATV") {
            return false;
        }
        const std::string psz_string = oem_key1.substr(6, 3);
        // If PSZ string contains non digit, then it is not a panel TV device.
        for (char ch : psz_string) {
            if (!isdigit(ch)) {
                return false;
            }
        }
        // If PSZ is "000", then it is not a panel TV device.
        if (psz_string == "000") {
            return false;
        }
        return true;
    }

    std::string getPropertyString(const char* property_name) {
        char property_string_raw_bytes[PROPERTY_VALUE_MAX] = {};
        int len = property_get(property_name, property_string_raw_bytes, "");
        return std::string(property_string_raw_bytes, len);
    }

  private:
    const char* getInstanceName() { return GetParam().c_str(); }
};

/*
 * GetFactoryMacAddress
 * Ensures that calls to getFactoryMacAddress will retrieve a non-zero MAC.
 */
TEST_P(WifiStaIfaceAidlTest, GetFactoryMacAddress) {
    std::array<uint8_t, 6> mac;
    EXPECT_TRUE(wifi_sta_iface_->getFactoryMacAddress(&mac).isOk());
    std::array<uint8_t, 6> all_zero_mac = {0, 0, 0, 0, 0, 0};
    EXPECT_NE(mac, all_zero_mac);
}

/*
 * GetFeatureSet
 */
TEST_P(WifiStaIfaceAidlTest, GetFeatureSet) {
    int32_t features = 0;
    EXPECT_TRUE(wifi_sta_iface_->getFeatureSet(&features).isOk());
    EXPECT_NE(features, 0);
}

/*
 * CheckApfIsSupported:
 * Ensures the APF packet filter is fully supported as required in VSR 14:
 * https://docs.partner.android.com/gms/policies/vsr/vsr-14
 */
// @VsrTest = 5.3.12
TEST_P(WifiStaIfaceAidlTest, CheckApfIsSupported) {
    const std::string oem_key1 = getPropertyString("ro.oem.key1");
    if (isTvDevice()) {
        // Flat panel TV devices that support MDNS offload do not have to implement APF if the WiFi
        // chipset does not have sufficient RAM to do so.
        if (isPanelTvDevice() && isMdnsOffloadPresentInNIC()) {
            GTEST_SKIP() << "Panel TV supports mDNS offload. It is not required to support APF";
        }
        // For TV devices declaring the
        // com.google.android.tv.full_networking_under_2w feature, this indicates
        // the device can meet the <= 2W standby power requirement while
        // continuously processing network packets on the CPU, even in standby mode.
        // In these cases, APF support is strongly recommended rather than being
        // mandatory.
        if (doesDeviceSupportFullNetworkingUnder2w()) {
            GTEST_SKIP() << "TV Device meets the <= 2W standby power demand requirement. It is not "
                            "required to support APF.";
        }
    }
    int vendor_api_level = property_get_int32("ro.vendor.api_level", 0);
    // Before VSR 14, APF support is optional.
    if (vendor_api_level < __ANDROID_API_U__) {
        if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::APF)) {
            GTEST_SKIP() << "APF packet filter capabilities are not supported.";
        }
        StaApfPacketFilterCapabilities apf_caps = {};
        EXPECT_TRUE(wifi_sta_iface_->getApfPacketFilterCapabilities(&apf_caps).isOk());
        return;
    }

    EXPECT_TRUE(isFeatureSupported(IWifiStaIface::FeatureSetMask::APF));
    StaApfPacketFilterCapabilities apf_caps = {};
    EXPECT_TRUE(wifi_sta_iface_->getApfPacketFilterCapabilities(&apf_caps).isOk());
    EXPECT_GE(apf_caps.version, 4);
    // Based on VSR-14 the usable memory must be at least 1024 bytes.
    EXPECT_GE(apf_caps.maxLength, 1024);
    if (vendor_api_level >= __ANDROID_API_V__) {
        // Based on VSR-15 the usable memory must be at least 2000 bytes.
        EXPECT_GE(apf_caps.maxLength, 2000);
    }
}

/*
 * GetBackgroundScanCapabilities
 */
TEST_P(WifiStaIfaceAidlTest, GetBackgroundScanCapabilities) {
    if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::BACKGROUND_SCAN)) {
        GTEST_SKIP() << "Background scan capabilities are not supported.";
    }
    StaBackgroundScanCapabilities caps = {};
    EXPECT_TRUE(wifi_sta_iface_->getBackgroundScanCapabilities(&caps).isOk());
}

/*
 * GetLinkLayerStats
 * Ensures that calls to getLinkLayerStats will retrieve a non-empty
 * StaLinkLayerStats after link layer stats collection is enabled.
 */
TEST_P(WifiStaIfaceAidlTest, GetLinkLayerStats) {
    if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::LINK_LAYER_STATS)) {
        GTEST_SKIP() << "Skipping this test since link layer stats are not supported.";
    }

    // Enable link layer stats collection.
    EXPECT_TRUE(wifi_sta_iface_->enableLinkLayerStatsCollection(true).isOk());

    // Retrieve link layer stats.
    StaLinkLayerStats link_layer_stats = {};
    EXPECT_TRUE(wifi_sta_iface_->getLinkLayerStats(&link_layer_stats).isOk());
    EXPECT_GT(link_layer_stats.timeStampInMs, 0);

    // Try to create a 2nd iface. If successful, it should fill the duty cycle field.
    std::shared_ptr<IWifiStaIface> iface;
    auto status = createStaIface(&iface);
    if (status.isOk()) {
        EXPECT_GT(link_layer_stats.iface.links[0].timeSliceDutyCycleInPercent, 0);
    }

    // Disable link layer stats collection.
    EXPECT_TRUE(wifi_sta_iface_->disableLinkLayerStatsCollection().isOk());
}

/*
 * SetMacAddress
 * Ensures that calls to setMacAddress will return successfully.
 */
TEST_P(WifiStaIfaceAidlTest, SetMacAddress) {
    std::array<uint8_t, 6> mac = {0x12, 0x22, 0x33, 0x52, 0x10, 0x41};
    EXPECT_TRUE(wifi_sta_iface_->setMacAddress(mac).isOk());
}

/*
 * SetScanMode
 */
TEST_P(WifiStaIfaceAidlTest, SetScanMode) {
    auto status = wifi_sta_iface_->setScanMode(true);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));

    status = wifi_sta_iface_->setScanMode(false);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
}

/*
 * LinkLayerStatsCollection
 */
TEST_P(WifiStaIfaceAidlTest, LinkLayerStatsCollection) {
    if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::LINK_LAYER_STATS)) {
        GTEST_SKIP() << "Link layer stats collection is not supported.";
    }

    // Enable link layer stats collection.
    EXPECT_TRUE(wifi_sta_iface_->enableLinkLayerStatsCollection(true).isOk());

    // Retrieve link layer stats.
    StaLinkLayerStats link_layer_stats = {};
    EXPECT_TRUE(wifi_sta_iface_->getLinkLayerStats(&link_layer_stats).isOk());

    // Disable link layer stats collection.
    EXPECT_TRUE(wifi_sta_iface_->disableLinkLayerStatsCollection().isOk());
}

/*
 * RSSIMonitoring
 * Ensures that calls to startRssiMonitoring and stopRssiMonitoring will fail
 * if the device is not connected to an AP.
 */
TEST_P(WifiStaIfaceAidlTest, RSSIMonitoring) {
    if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::RSSI_MONITOR)) {
        GTEST_SKIP() << "RSSI monitoring is not supported.";
    }

    const int cmd = 1;
    const int maxRssi = -50;
    const int minRssi = -90;
    // Expected to fail because device is not connected to an AP.
    EXPECT_FALSE(wifi_sta_iface_->startRssiMonitoring(cmd, maxRssi, minRssi).isOk());
    EXPECT_FALSE(wifi_sta_iface_->stopRssiMonitoring(cmd).isOk());
}

/*
 * RoamingControl
 */
TEST_P(WifiStaIfaceAidlTest, RoamingControl) {
    if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::CONTROL_ROAMING)) {
        GTEST_SKIP() << "Roaming control is not supported.";
    }

    // Retrieve roaming capabilities.
    StaRoamingCapabilities caps = {};
    EXPECT_TRUE(wifi_sta_iface_->getRoamingCapabilities(&caps).isOk());

    // Set up roaming configuration based on roaming capabilities.
    StaRoamingConfig roaming_config = {};
    if (caps.maxBlocklistSize > 0) {
        MacAddress block_list_entry;
        block_list_entry.data = std::array<uint8_t, 6>{{0x11, 0x22, 0x33, 0x44, 0x55, 0x66}};
        roaming_config.bssidBlocklist = {block_list_entry};
    }
    if (caps.maxAllowlistSize > 0) {
        Ssid allow_list_entry = {};
        allow_list_entry.data = std::array<uint8_t, 32>{{0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC}};
        roaming_config.ssidAllowlist = {allow_list_entry};
    }

    // Configure roaming.
    EXPECT_TRUE(wifi_sta_iface_->configureRoaming(roaming_config).isOk());

    // Enable roaming.
    EXPECT_TRUE(wifi_sta_iface_->setRoamingState(StaRoamingState::ENABLED).isOk());
}

/*
 * RoamingModeControl
 */
TEST_P(WifiStaIfaceAidlTest, RoamingModeControl) {
    if (interface_version_ < 2) {
        GTEST_SKIP() << "Roaming mode control is available as of sta_iface V2";
    }
    if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::ROAMING_MODE_CONTROL)) {
        GTEST_SKIP() << "Roaming mode control is not supported.";
    }

    // Enable aggressive roaming.
    EXPECT_TRUE(wifi_sta_iface_->setRoamingState(StaRoamingState::AGGRESSIVE).isOk());
}

/*
 * EnableNDOffload
 */
TEST_P(WifiStaIfaceAidlTest, EnableNDOffload) {
    if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::ND_OFFLOAD)) {
        GTEST_SKIP() << "ND offload is not supported.";
    }
    EXPECT_TRUE(wifi_sta_iface_->enableNdOffload(true).isOk());
}

/*
 * PacketFateMonitoring
 */
TEST_P(WifiStaIfaceAidlTest, PacketFateMonitoring) {
    // Start packet fate monitoring.
    auto status = wifi_sta_iface_->startDebugPacketFateMonitoring();
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));

    // Retrieve packets.
    if (status.isOk()) {
        std::vector<WifiDebugRxPacketFateReport> rx_reports;
        std::vector<WifiDebugTxPacketFateReport> tx_reports;
        EXPECT_TRUE(wifi_sta_iface_->getDebugRxPacketFates(&rx_reports).isOk());
        EXPECT_TRUE(wifi_sta_iface_->getDebugTxPacketFates(&tx_reports).isOk());
    }
}

/*
 * CachedScanData
 */
TEST_P(WifiStaIfaceAidlTest, CachedScanData) {
    if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::CACHED_SCAN_DATA)) {
        GTEST_SKIP() << "Cached scan data is not supported.";
    }

    // Retrieve cached scan data.
    CachedScanData cached_scan_data = {};
    EXPECT_TRUE(wifi_sta_iface_->getCachedScanData(&cached_scan_data).isOk());

    if (cached_scan_data.cachedScanResults.size() > 0) {
        EXPECT_GT(cached_scan_data.cachedScanResults[0].frequencyMhz, 0);
    }
}

TEST_P(WifiStaIfaceAidlTest, TwtGetCapabilities) {
    if (interface_version_ < 2) {
        GTEST_SKIP() << "TwtGetCapabilities is available as of sta_iface V2";
    }

    TwtCapabilities twt_capabilities = {};
    auto status = wifi_sta_iface_->twtGetCapabilities(&twt_capabilities);
    if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        GTEST_SKIP() << "twtGetCapabilities() is not supported by the vendor";
    }
    EXPECT_TRUE(status.isOk());
    if (!twt_capabilities.isTwtRequesterSupported) {
        GTEST_SKIP() << "TWT is not supported";
    }

    EXPECT_GT(twt_capabilities.minWakeDurationUs, 0);
    EXPECT_GT(twt_capabilities.maxWakeDurationUs, 0);
    EXPECT_GT(twt_capabilities.minWakeIntervalUs, 0);
    EXPECT_GT(twt_capabilities.maxWakeIntervalUs, 0);
}

TEST_P(WifiStaIfaceAidlTest, TwtSessionSetup) {
    if (interface_version_ < 2) {
        GTEST_SKIP() << "TwtSessionSetup is available as of sta_iface V2";
    }

    TwtCapabilities twt_capabilities = {};
    auto status = wifi_sta_iface_->twtGetCapabilities(&twt_capabilities);
    if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        GTEST_SKIP() << "twtGetCapabilities() is not supported by the vendor";
    }
    EXPECT_TRUE(status.isOk());
    if (!twt_capabilities.isTwtRequesterSupported) {
        GTEST_SKIP() << "TWT is not supported";
    }

    TwtRequest twtRequest;
    twtRequest.mloLinkId = 0;
    twtRequest.minWakeDurationUs = twt_capabilities.minWakeDurationUs;
    twtRequest.maxWakeDurationUs = twt_capabilities.maxWakeDurationUs;
    twtRequest.minWakeIntervalUs = twt_capabilities.minWakeIntervalUs;
    twtRequest.maxWakeIntervalUs = twt_capabilities.maxWakeIntervalUs;
    EXPECT_TRUE(wifi_sta_iface_->twtSessionSetup(1, twtRequest).isOk());
}

TEST_P(WifiStaIfaceAidlTest, TwtSessionGetStats) {
    if (interface_version_ < 2) {
        GTEST_SKIP() << "TwtSessionGetStats is available as of sta_iface V2";
    }

    TwtCapabilities twt_capabilities = {};
    auto status = wifi_sta_iface_->twtGetCapabilities(&twt_capabilities);
    if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        GTEST_SKIP() << "twtGetCapabilities() is not supported by the vendor";
    }
    EXPECT_TRUE(status.isOk());
    if (!twt_capabilities.isTwtRequesterSupported) {
        GTEST_SKIP() << "TWT is not supported";
    }

    // Expecting a IWifiStaIfaceEventCallback.onTwtFailure() with INVALID_PARAMS
    // as the error code.
    EXPECT_TRUE(wifi_sta_iface_->twtSessionGetStats(1, 10).isOk());
}

TEST_P(WifiStaIfaceAidlTest, TwtSessionTeardown) {
    if (interface_version_ < 2) {
        GTEST_SKIP() << "TwtSessionTeardown is available as of sta_iface V3";
    }

    TwtCapabilities twt_capabilities = {};
    auto status = wifi_sta_iface_->twtGetCapabilities(&twt_capabilities);
    if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        GTEST_SKIP() << "twtGetCapabilities() is not supported by the vendor";
    }
    EXPECT_TRUE(status.isOk());
    if (!twt_capabilities.isTwtRequesterSupported) {
        GTEST_SKIP() << "TWT is not supported";
    }

    // Expecting a IWifiStaIfaceEventCallback.onTwtFailure() with INVALID_PARAMS
    // as  the error code.
    EXPECT_TRUE(wifi_sta_iface_->twtSessionTeardown(1, 10).isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiStaIfaceAidlTest);
INSTANTIATE_TEST_SUITE_P(WifiTest, WifiStaIfaceAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IWifi::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
    android::ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
