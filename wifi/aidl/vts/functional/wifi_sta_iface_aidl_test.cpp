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

    // Checks if the MdnsOffloadManagerService is installed.
    bool isMdnsOffloadServicePresent() {
        int status =
                // --query-flags MATCH_SYSTEM_ONLY(1048576) will only return matched service
                // installed on system or system_ext partition. The MdnsOffloadManagerService should
                // be installed on system_ext partition.
                // NOLINTNEXTLINE(cert-env33-c)
                system("pm query-services --query-flags 1048576"
                       " com.android.tv.mdnsoffloadmanager/"
                       "com.android.tv.mdnsoffloadmanager.MdnsOffloadManagerService"
                       " | egrep -q mdnsoffloadmanager");
        return status == 0;
    }

    // Detected panel TV device by using ro.oem.key1 property.
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
    // Flat panel TV devices that support MDNS offload do not have to implement APF if the WiFi
    // chipset does not have sufficient RAM to do so.
    if (isPanelTvDevice() && isMdnsOffloadServicePresent()) {
        GTEST_SKIP() << "Panel TV supports mDNS offload. It is not required to support APF";
    }
    int vendor_api_level = property_get_int32("ro.vendor.api_level", 0);
    // Before VSR 14, APF support is optional.
    if (vendor_api_level < __ANDROID_API_U__) {
        if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::APF)) {
            GTEST_SKIP() << "APF packet filter capabilities are not supported.";
        }
        StaApfPacketFilterCapabilities apf_caps = {};
        EXPECT_TRUE(wifi_sta_iface_->getApfPacketFilterCapabilities(&apf_caps).isOk());
    } else {
        EXPECT_TRUE(isFeatureSupported(IWifiStaIface::FeatureSetMask::APF));
        StaApfPacketFilterCapabilities apf_caps = {};
        EXPECT_TRUE(wifi_sta_iface_->getApfPacketFilterCapabilities(&apf_caps).isOk());
        // The APF version must be 4 and the usable memory must be at least
        // 1024 bytes.
        EXPECT_EQ(apf_caps.version, 4);
        EXPECT_GE(apf_caps.maxLength, 1024);
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
