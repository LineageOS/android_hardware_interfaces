/*
 * Copyright (C) 2021 The Android Open Source Project
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
#include <aidl/android/hardware/wifi/IWifi.h>
#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/hostapd/1.3/IHostapd.h>

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/wifi/hostapd/BnHostapd.h>
#include <aidl/android/hardware/wifi/hostapd/BnHostapdCallback.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <hidl/ServiceManagement.h>
#include <hostapd_hidl_call_util.h>
#include <hostapd_hidl_test_utils.h>
#include <wifi_hidl_test_utils.h>
#include <wifi_hidl_test_utils_1_5.h>
#include <wifi_hidl_test_utils_1_6.h>

#include "hostapd_test_utils.h"
#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::hostapd::BandMask;
using aidl::android::hardware::wifi::hostapd::BnHostapdCallback;
using aidl::android::hardware::wifi::hostapd::ChannelBandwidth;
using aidl::android::hardware::wifi::hostapd::ChannelParams;
using aidl::android::hardware::wifi::hostapd::DebugLevel;
using aidl::android::hardware::wifi::hostapd::EncryptionType;
using aidl::android::hardware::wifi::hostapd::FrequencyRange;
using aidl::android::hardware::wifi::hostapd::Ieee80211ReasonCode;
using aidl::android::hardware::wifi::hostapd::IfaceParams;
using aidl::android::hardware::wifi::hostapd::IHostapd;
using aidl::android::hardware::wifi::hostapd::NetworkParams;
using android::ProcessState;

namespace {
const unsigned char kNwSsid[] = {'t', 'e', 's', 't', '1', '2', '3', '4', '5'};
const std::string kPassphrase = "test12345";
const std::string kInvalidMinPassphrase = "test";
const std::string kInvalidMaxPassphrase =
    "0123456789012345678901234567890123456789012345678901234567890123456789";
const int kIfaceChannel = 6;
const int kIfaceInvalidChannel = 567;
const std::vector<uint8_t> kTestZeroMacAddr(6, 0x0);
const Ieee80211ReasonCode kTestDisconnectReasonCode = Ieee80211ReasonCode::WLAN_REASON_UNSPECIFIED;

inline BandMask operator|(BandMask a, BandMask b) {
    return static_cast<BandMask>(static_cast<int32_t>(a) |
                                 static_cast<int32_t>(b));
}
}  // namespace

class HostapdAidl : public testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        disableHalsAndFramework();
        initializeHostapdAndVendorHal(GetParam());

        hostapd = getHostapd(GetParam());
        ASSERT_NE(hostapd, nullptr);
        EXPECT_TRUE(hostapd->setDebugParams(DebugLevel::EXCESSIVE).isOk());

        isAcsSupport = testing::checkSubstringInCommandOutput(
            "/system/bin/cmd wifi get-softap-supported-features",
            "wifi_softap_acs_supported");
        isWpa3SaeSupport = testing::checkSubstringInCommandOutput(
            "/system/bin/cmd wifi get-softap-supported-features",
            "wifi_softap_wpa3_sae_supported");
        isBridgedSupport = testing::checkSubstringInCommandOutput(
                "/system/bin/cmd wifi get-softap-supported-features",
                "wifi_softap_bridged_ap_supported");
    }

    virtual void TearDown() override {
        hostapd->terminate();
        //  Wait 3 seconds to allow terminate to complete
        sleep(3);
        stopHostapdAndVendorHal();
        startWifiFramework();
    }

    std::shared_ptr<IHostapd> hostapd;
    bool isAcsSupport;
    bool isWpa3SaeSupport;
    bool isBridgedSupport;

    IfaceParams getIfaceParamsWithoutAcs(std::string iface_name) {
        IfaceParams iface_params;
        ChannelParams channelParams;
        std::vector<ChannelParams> vec_channelParams;

        iface_params.name = iface_name;
        iface_params.hwModeParams.enable80211N = true;
        iface_params.hwModeParams.enable80211AC = false;
        iface_params.hwModeParams.enable80211AX = false;
        iface_params.hwModeParams.enable6GhzBand = false;
        iface_params.hwModeParams.maximumChannelBandwidth = ChannelBandwidth::BANDWIDTH_20;

        channelParams.enableAcs = false;
        channelParams.acsShouldExcludeDfs = false;
        channelParams.channel = kIfaceChannel;
        channelParams.bandMask = BandMask::BAND_2_GHZ;

        vec_channelParams.push_back(channelParams);
        iface_params.channelParams = vec_channelParams;
        return iface_params;
    }

    IfaceParams getIfaceParamsWithBridgedModeACS(std::string iface_name) {
        IfaceParams iface_params = getIfaceParamsWithoutAcs(iface_name);
        iface_params.channelParams[0].enableAcs = true;
        iface_params.channelParams[0].acsShouldExcludeDfs = true;

        std::vector<ChannelParams> vec_channelParams;
        vec_channelParams.push_back(iface_params.channelParams[0]);

        ChannelParams second_channelParams;
        second_channelParams.channel = 0;
        second_channelParams.enableAcs = true;
        second_channelParams.bandMask = BandMask::BAND_5_GHZ;
        vec_channelParams.push_back(second_channelParams);

        iface_params.channelParams = vec_channelParams;
        return iface_params;
    }

    IfaceParams getIfaceParamsWithAcs(std::string iface_name) {
        IfaceParams iface_params = getIfaceParamsWithoutAcs(iface_name);
        iface_params.channelParams[0].enableAcs = true;
        iface_params.channelParams[0].acsShouldExcludeDfs = true;
        iface_params.channelParams[0].channel = 0;
        iface_params.channelParams[0].bandMask =
            iface_params.channelParams[0].bandMask | BandMask::BAND_5_GHZ;
        return iface_params;
    }

    IfaceParams getIfaceParamsWithAcsAndFreqRange(std::string iface_name) {
        IfaceParams iface_params = getIfaceParamsWithAcs(iface_name);
        FrequencyRange freqRange;
        freqRange.startMhz = 2412;
        freqRange.endMhz = 2462;
        std::vector<FrequencyRange> vec_FrequencyRange;
        vec_FrequencyRange.push_back(freqRange);
        iface_params.channelParams[0].acsChannelFreqRangesMhz =
            vec_FrequencyRange;
        return iface_params;
    }

    IfaceParams getIfaceParamsWithAcsAndInvalidFreqRange(
        std::string iface_name) {
        IfaceParams iface_params =
            getIfaceParamsWithAcsAndFreqRange(iface_name);
        iface_params.channelParams[0].acsChannelFreqRangesMhz[0].startMhz =
            222;
        iface_params.channelParams[0].acsChannelFreqRangesMhz[0].endMhz =
            999;
        return iface_params;
    }

    IfaceParams getIfaceParamsWithInvalidChannel(std::string iface_name) {
        IfaceParams iface_params = getIfaceParamsWithoutAcs(iface_name);
        iface_params.channelParams[0].channel = kIfaceInvalidChannel;
        return iface_params;
    }

    NetworkParams getOpenNwParams() {
        NetworkParams nw_params;
        nw_params.ssid =
            std::vector<uint8_t>(kNwSsid, kNwSsid + sizeof(kNwSsid));
        nw_params.isHidden = false;
        nw_params.encryptionType = EncryptionType::NONE;
        nw_params.isMetered = true;
        return nw_params;
    }

    NetworkParams getPskNwParamsWithNonMetered() {
        NetworkParams nw_params = getOpenNwParams();
        nw_params.encryptionType = EncryptionType::WPA2;
        nw_params.passphrase = kPassphrase;
        nw_params.isMetered = false;
        return nw_params;
    }

    NetworkParams getPskNwParams() {
        NetworkParams nw_params = getOpenNwParams();
        nw_params.encryptionType = EncryptionType::WPA2;
        nw_params.passphrase = kPassphrase;
        return nw_params;
    }

    NetworkParams getInvalidPskNwParams() {
        NetworkParams nw_params = getOpenNwParams();
        nw_params.encryptionType = EncryptionType::WPA2;
        nw_params.passphrase = kInvalidMaxPassphrase;
        return nw_params;
    }

    NetworkParams getSaeTransitionNwParams() {
        NetworkParams nw_params = getOpenNwParams();
        nw_params.encryptionType = EncryptionType::WPA3_SAE_TRANSITION;
        nw_params.passphrase = kPassphrase;
        return nw_params;
    }

    NetworkParams getInvalidSaeTransitionNwParams() {
        NetworkParams nw_params = getOpenNwParams();
        nw_params.encryptionType = EncryptionType::WPA2;
        nw_params.passphrase = kInvalidMinPassphrase;
        return nw_params;
    }

    NetworkParams getSaeNwParams() {
        NetworkParams nw_params = getOpenNwParams();
        nw_params.encryptionType = EncryptionType::WPA3_SAE;
        nw_params.passphrase = kPassphrase;
        return nw_params;
    }

    NetworkParams getInvalidSaeNwParams() {
        NetworkParams nw_params = getOpenNwParams();
        nw_params.encryptionType = EncryptionType::WPA3_SAE;
        nw_params.passphrase = "";
        return nw_params;
    }
};

class HostapdCallback : public BnHostapdCallback {
   public:
    HostapdCallback() = default;
    ::ndk::ScopedAStatus onApInstanceInfoChanged(
        const ::aidl::android::hardware::wifi::hostapd::ApInfo &) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onConnectedClientsChanged(
        const ::aidl::android::hardware::wifi::hostapd::ClientInfo &) override {
        return ndk::ScopedAStatus::ok();
    }
    ::ndk::ScopedAStatus onFailure(const std::string&, const std::string&) override {
        return ndk::ScopedAStatus::ok();
    }
};

/**
 * Register callback
 */
TEST_P(HostapdAidl, RegisterCallback) {
    std::shared_ptr<HostapdCallback> callback =
        ndk::SharedRefBase::make<HostapdCallback>();
    ASSERT_NE(callback, nullptr);
    EXPECT_TRUE(hostapd->registerCallback(callback).isOk());
}

/**
 * Adds an access point with PSK network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithAcs) {
    if (!isAcsSupport) GTEST_SKIP() << "Missing ACS support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = hostapd->addAccessPoint(getIfaceParamsWithAcs(ifname), getPskNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with PSK network config, ACS enabled & frequency Range.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithAcsAndFreqRange) {
    if (!isAcsSupport) GTEST_SKIP() << "Missing ACS support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status =
            hostapd->addAccessPoint(getIfaceParamsWithAcsAndFreqRange(ifname), getPskNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with invalid channel range.
 * Access point creation should fail.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithAcsAndInvalidFreqRange) {
    if (!isAcsSupport) GTEST_SKIP() << "Missing ACS support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = hostapd->addAccessPoint(getIfaceParamsWithAcsAndInvalidFreqRange(ifname),
                                          getPskNwParams());
    EXPECT_FALSE(status.isOk());
}

/**
 * Adds an access point with Open network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddOpenAccessPointWithAcs) {
    if (!isAcsSupport) GTEST_SKIP() << "Missing ACS support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = hostapd->addAccessPoint(getIfaceParamsWithAcs(ifname), getOpenNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with PSK network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithoutAcs) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(ifname), getPskNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with PSK network config, ACS disabled & Non metered.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithoutAcsAndNonMetered) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(ifname),
                                          getPskNwParamsWithNonMetered());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with Open network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddOpenAccessPointWithoutAcs) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(ifname), getOpenNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with SAE Transition network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddSaeTransitionAccessPointWithoutAcs) {
    if (!isWpa3SaeSupport) GTEST_SKIP() << "Missing SAE support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status =
            hostapd->addAccessPoint(getIfaceParamsWithoutAcs(ifname), getSaeTransitionNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with SAE network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddSAEAccessPointWithoutAcs) {
    if (!isWpa3SaeSupport) GTEST_SKIP() << "Missing SAE support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(ifname), getSaeNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds & then removes an access point with PSK network config & ACS enabled.
 * Access point creation & removal should pass.
 */
TEST_P(HostapdAidl, RemoveAccessPointWithAcs) {
    if (!isAcsSupport) GTEST_SKIP() << "Missing ACS support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = hostapd->addAccessPoint(getIfaceParamsWithAcs(ifname), getPskNwParams());
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(hostapd->removeAccessPoint(ifname).isOk());
}

/**
 * Adds & then removes an access point with PSK network config & ACS disabled.
 * Access point creation & removal should pass.
 */
TEST_P(HostapdAidl, RemoveAccessPointWithoutAcs) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(ifname), getPskNwParams());
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(hostapd->removeAccessPoint(ifname).isOk());
}

/**
 * Adds an access point with invalid channel.
 * Access point creation should fail.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithInvalidChannel) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status =
            hostapd->addAccessPoint(getIfaceParamsWithInvalidChannel(ifname), getPskNwParams());
    EXPECT_FALSE(status.isOk());
}

/**
 * Adds an access point with invalid PSK network config.
 * Access point creation should fail.
 */
TEST_P(HostapdAidl, AddInvalidPskAccessPointWithoutAcs) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status =
            hostapd->addAccessPoint(getIfaceParamsWithoutAcs(ifname), getInvalidPskNwParams());
    EXPECT_FALSE(status.isOk());
}

/**
 * Adds an access point with invalid SAE transition network config.
 * Access point creation should fail.
 */
TEST_P(HostapdAidl, AddInvalidSaeTransitionAccessPointWithoutAcs) {
    std::string ifname = setupApIfaceAndGetName(false);
    if (!isWpa3SaeSupport) GTEST_SKIP() << "Missing SAE support";
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(ifname),
                                          getInvalidSaeTransitionNwParams());
    EXPECT_FALSE(status.isOk());
}

/**
 * Adds an access point with invalid SAE network config.
 * Access point creation should fail.
 */
TEST_P(HostapdAidl, AddInvalidSaeAccessPointWithoutAcs) {
    std::string ifname = setupApIfaceAndGetName(false);
    if (!isWpa3SaeSupport) GTEST_SKIP() << "Missing SAE support";
    auto status =
            hostapd->addAccessPoint(getIfaceParamsWithoutAcs(ifname), getInvalidSaeNwParams());
    EXPECT_FALSE(status.isOk());
}

/**
 * forceClientDisconnect should fail when hotspot interface available.
 */
TEST_P(HostapdAidl, DisconnectClientWhenIfacAvailable) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(ifname), getOpenNwParams());
    EXPECT_TRUE(status.isOk());

    status = hostapd->forceClientDisconnect(ifname, kTestZeroMacAddr, kTestDisconnectReasonCode);
    EXPECT_FALSE(status.isOk());
}

/**
 * AddAccessPointWithDualBandConfig should pass
 */
TEST_P(HostapdAidl, AddAccessPointWithDualBandConfig) {
    if (!isBridgedSupport) GTEST_SKIP() << "Missing Bridged AP support";
    std::string ifname = setupApIfaceAndGetName(true);
    auto status =
            hostapd->addAccessPoint(getIfaceParamsWithBridgedModeACS(ifname), getOpenNwParams());
    EXPECT_TRUE(status.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HostapdAidl);
INSTANTIATE_TEST_SUITE_P(
    Hostapd, HostapdAidl,
    testing::ValuesIn(android::getAidlHalInstanceNames(IHostapd::descriptor)),
    android::PrintInstanceNameToString);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
