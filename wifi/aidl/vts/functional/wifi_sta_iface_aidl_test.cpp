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
#include <aidl/android/hardware/wifi/BnWifiStaIfaceEventCallback.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <cutils/properties.h>

#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::BnWifiStaIfaceEventCallback;
using aidl::android::hardware::wifi::CachedScanData;
using aidl::android::hardware::wifi::IWifi;
using aidl::android::hardware::wifi::IWifiStaIface;
using aidl::android::hardware::wifi::MacAddress;
using aidl::android::hardware::wifi::Ssid;
using aidl::android::hardware::wifi::StaApfPacketFilterCapabilities;
using aidl::android::hardware::wifi::StaBackgroundScanCapabilities;
using aidl::android::hardware::wifi::StaBackgroundScanParameters;
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

namespace {
const int kTestCmdId = 123;
const std::array<uint8_t, 6> kTestMacAddr1 = {0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f};
const std::array<uint8_t, 6> kTestMacAddr2 = {0x4a, 0x5b, 0x6c, 0x7d, 0x8e, 0x9f};
// Global variable to track whether the Wifi framework should be re-enabled after testing.
static bool sWifiFrameworkDisabledByTest = false;
}  // namespace

class WifiStaIfaceAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        if (!::testing::deviceSupportsFeature("android.hardware.wifi")) {
            GTEST_SKIP() << "Skipping this test since wifi is not supported.";
        }
        stopWifiService(getInstanceName());
        wifi_sta_iface_ = getWifiStaIface(getInstanceName());
        if (wifi_sta_iface_ == nullptr) {
            // Try retrieving the iface pointer one more time. This call is
            // a common source of testing flake.
            wifi_sta_iface_ = getWifiStaIface(getInstanceName());
        }
        ASSERT_NE(nullptr, wifi_sta_iface_.get());
        ASSERT_TRUE(wifi_sta_iface_->getInterfaceVersion(&interface_version_).isOk());
    }

    void TearDown() override { stopWifiService(getInstanceName()); }

    // Runs at the beginning of the test suite.
    static void SetUpTestSuite() {
        if (isWifiFrameworkEnabled()) {
            LOG(INFO) << "Disabling the Wifi framework for testing";
            setWifiFrameworkEnabled(false);
            sleep(2);
            sWifiFrameworkDisabledByTest = true;
        }
    }

    // Runs at the end of the test suite.
    static void TearDownTestSuite() {
        if (sWifiFrameworkDisabledByTest) {
            LOG(INFO) << "Re-enabling the Wifi framework after testing";
            setWifiFrameworkEnabled(true);
        }
    }

  protected:
    bool isFeatureSupported(IWifiStaIface::FeatureSetMask expected) {
        int32_t features = 0;
        EXPECT_TRUE(wifi_sta_iface_->getFeatureSet(&features).isOk());
        return features & static_cast<int32_t>(expected);
    }

    bool isTwtSupported() {
        TwtCapabilities twt_capabilities = {};
        auto status = wifi_sta_iface_->twtGetCapabilities(&twt_capabilities);
        return status.isOk() && twt_capabilities.isTwtRequesterSupported;
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

    bool isPcDevice() { return testing::deviceSupportsFeature("android.hardware.type.pc"); }

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
// @VsrTest = VSR-5.3.12-001|VSR-5.3.12-003|VSR-5.3.12-004|VSR-5.3.12-009
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
    if (isPcDevice()) {
        GTEST_SKIP() << "PC devices do not support APF.";
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

class WifiStaIfaceEventCallback : public BnWifiStaIfaceEventCallback {
  public:
    WifiStaIfaceEventCallback() = default;

    ::ndk::ScopedAStatus onBackgroundFullScanResult(
            int32_t /* in_cmdId */, int32_t /* in_bucketsScanned */,
            const ::aidl::android::hardware::wifi::StaScanResult& /* in_result */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onBackgroundScanFailure(int32_t /* in_cmdId */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onBackgroundScanResults(
            int32_t /* in_cmdId */,
            const std::vector<::aidl::android::hardware::wifi::StaScanData>& /* in_scanDatas */)
            override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onRssiThresholdBreached(int32_t /* in_cmdId */,
                                                 const std::array<uint8_t, 6>& /* in_currBssid */,
                                                 int32_t /* in_currRssi */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onTwtFailure(int32_t /*in_cmdId*/,
                                      ::aidl::android::hardware::wifi::IWifiStaIfaceEventCallback::
                                              TwtErrorCode /* in_error */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onTwtSessionCreate(
            int32_t /* in_cmdId */,
            const ::aidl::android::hardware::wifi::TwtSession& /* in_twtSession */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onTwtSessionUpdate(
            int32_t /* in_cmdId */,
            const ::aidl::android::hardware::wifi::TwtSession& /* in_twtSession */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onTwtSessionTeardown(
            int32_t /* in_cmdId */, int32_t /* in_twtSessionId */,
            ::aidl::android::hardware::wifi::IWifiStaIfaceEventCallback::
                    TwtTeardownReasonCode /* in_reasonCode */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onTwtSessionStats(
            int32_t /* in_cmdId */, int32_t /* in_twtSessionId */,
            const ::aidl::android::hardware::wifi::TwtSessionStats& /* in_twtSessionStats */)
            override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onTwtSessionSuspend(int32_t /* in_cmdId */,
                                             int32_t /* in_twtSessionId */) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onTwtSessionResume(int32_t /* in_cmdId */,
                                            int32_t /* in_twtSessionId */) override {
        return ndk::ScopedAStatus::ok();
    }
};

/**
 * TwtGetCapabilities
 */
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

/**
 * TwtSessionSetup
 */
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
    const std::shared_ptr<WifiStaIfaceEventCallback> callback =
            ndk::SharedRefBase::make<WifiStaIfaceEventCallback>();
    ASSERT_NE(callback, nullptr);
    EXPECT_TRUE(wifi_sta_iface_->registerEventCallback(callback).isOk());

    TwtRequest twtRequest;
    twtRequest.mloLinkId = 0;
    twtRequest.minWakeDurationUs = twt_capabilities.minWakeDurationUs;
    twtRequest.maxWakeDurationUs = twt_capabilities.maxWakeDurationUs;
    twtRequest.minWakeIntervalUs = twt_capabilities.minWakeIntervalUs;
    twtRequest.maxWakeIntervalUs = twt_capabilities.maxWakeIntervalUs;

    status = wifi_sta_iface_->twtSessionSetup(1, twtRequest);
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED));
}

/**
 * TwtSessionGetStats
 */
TEST_P(WifiStaIfaceAidlTest, TwtSessionGetStats) {
    if (interface_version_ < 2) {
        GTEST_SKIP() << "TwtSessionGetStats is available as of sta_iface V2";
    }
    if (!isTwtSupported()) {
        GTEST_SKIP() << "TWT is not supported";
    }

    auto status = wifi_sta_iface_->twtSessionGetStats(1, 10);
    // Expecting a IWifiStaIfaceEventCallback.onTwtFailure() with INVALID_PARAMS
    // as the error code, or that the call returns WifiStatusCode::ERROR_INVALID_ARGS.
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
}

/**
 * TwtSessionTeardown
 */
TEST_P(WifiStaIfaceAidlTest, TwtSessionTeardown) {
    if (interface_version_ < 2) {
        GTEST_SKIP() << "TwtSessionTeardown is available as of sta_iface V2";
    }
    if (!isTwtSupported()) {
        GTEST_SKIP() << "TWT is not supported";
    }

    auto status = wifi_sta_iface_->twtSessionTeardown(1, 10);
    // Expecting a IWifiStaIfaceEventCallback.onTwtFailure() with INVALID_PARAMS
    // as the error code, or that the call returns WifiStatusCode::ERROR_INVALID_ARGS.
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
}

/**
 * TwtSessionUpdate
 */
TEST_P(WifiStaIfaceAidlTest, TwtSessionUpdate) {
    if (interface_version_ < 2) {
        GTEST_SKIP() << "TwtSessionUpdate is available as of sta_iface V2";
    }
    if (!isTwtSupported()) {
        GTEST_SKIP() << "TWT is not supported";
    }

    TwtRequest twtRequest;
    twtRequest.mloLinkId = 0;
    twtRequest.minWakeDurationUs = 1000;
    twtRequest.maxWakeDurationUs = 10000;
    twtRequest.minWakeIntervalUs = 10000;
    twtRequest.maxWakeIntervalUs = 100000;

    auto status = wifi_sta_iface_->twtSessionUpdate(1, 10, twtRequest);
    if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        GTEST_SKIP() << "TwtSessionUpdate is not supported";
    }
    // Expecting a IWifiStaIfaceEventCallback.onTwtFailure() with INVALID_PARAMS
    // as the error code, or that the call returns WifiStatusCode::ERROR_INVALID_ARGS.
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
}

/**
 * TwtSessionSuspend
 */
TEST_P(WifiStaIfaceAidlTest, TwtSessionSuspend) {
    if (interface_version_ < 2) {
        GTEST_SKIP() << "TwtSessionSuspend is available as of sta_iface V2";
    }
    if (!isTwtSupported()) {
        GTEST_SKIP() << "TWT is not supported";
    }

    auto status = wifi_sta_iface_->twtSessionSuspend(1, 10);
    if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        GTEST_SKIP() << "TwtSessionSuspend is not supported";
    }
    // Expecting a IWifiStaIfaceEventCallback.onTwtFailure() with INVALID_PARAMS
    // as the error code, or that the call returns WifiStatusCode::ERROR_INVALID_ARGS.
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
}

/**
 * TwtSessionResume
 */
TEST_P(WifiStaIfaceAidlTest, TwtSessionResume) {
    if (interface_version_ < 2) {
        GTEST_SKIP() << "TwtSessionResume is available as of sta_iface V2";
    }
    if (!isTwtSupported()) {
        GTEST_SKIP() << "TWT is not supported";
    }

    auto status = wifi_sta_iface_->twtSessionResume(1, 10);
    if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        GTEST_SKIP() << "TwtSessionResume is not supported";
    }
    // Expecting a IWifiStaIfaceEventCallback.onTwtFailure() with INVALID_PARAMS
    // as the error code, or that the call returns WifiStatusCode::ERROR_INVALID_ARGS.
    EXPECT_TRUE(status.isOk() || checkStatusCode(&status, WifiStatusCode::ERROR_INVALID_ARGS));
}

/*
 * SetDtimMultiplier
 */
TEST_P(WifiStaIfaceAidlTest, SetDtimMultiplier) {
    // Multiplied value
    auto status = wifi_sta_iface_->setDtimMultiplier(2);
    if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
        GTEST_SKIP() << "SetDtimMultiplier is not supported";
    }
    EXPECT_TRUE(status.isOk());

    // Driver default value
    EXPECT_TRUE(wifi_sta_iface_->setDtimMultiplier(0).isOk());
}

/*
 * Start/Stop Background Scan
 */
TEST_P(WifiStaIfaceAidlTest, StartAndStopBackgroundScan) {
    if (!isFeatureSupported(IWifiStaIface::FeatureSetMask::BACKGROUND_SCAN)) {
        GTEST_SKIP() << "Background scan is not supported";
    }
    StaBackgroundScanParameters scanParams;
    EXPECT_TRUE(wifi_sta_iface_->startBackgroundScan(kTestCmdId, scanParams).isOk());
    EXPECT_TRUE(wifi_sta_iface_->stopBackgroundScan(kTestCmdId).isOk());
}

/*
 * Start/Stop Sending Keep-Alive Packets
 */
TEST_P(WifiStaIfaceAidlTest, StartAndStopSendingKeepAlivePackets) {
    std::vector<uint8_t> ipPacketData(20);
    uint16_t etherType = 0x0800;  // IPv4
    uint32_t periodInMs = 1000;   // 1 sec

    auto status = wifi_sta_iface_->startSendingKeepAlivePackets(
            kTestCmdId, ipPacketData, etherType, kTestMacAddr1, kTestMacAddr2, periodInMs);
    if (!status.isOk()) {
        // The device may not support this operation or the specific test values
        GTEST_SKIP() << "StartAndStopSendingKeepAlivePackets is not supported"
                     << ", status=" << status.getServiceSpecificError();
    }
    EXPECT_TRUE(status.isOk());

    // If start was successful, then stop should also work
    EXPECT_TRUE(wifi_sta_iface_->stopSendingKeepAlivePackets(kTestCmdId).isOk());
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
