/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <VtsCoreUtil.h>

#include <android-base/logging.h>
#include <cutils/properties.h>

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/hostapd/1.3/IHostapd.h>

#include "hostapd_hidl_call_util.h"
#include "hostapd_hidl_test_utils.h"
#include "wifi_hidl_test_utils_1_5.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::hostapd::V1_2::DebugLevel;
using ::android::hardware::wifi::hostapd::V1_2::HostapdStatusCode;
using ::android::hardware::wifi::hostapd::V1_2::Ieee80211ReasonCode;
using ::android::hardware::wifi::hostapd::V1_3::IHostapd;
using ::android::hardware::wifi::V1_0::IWifi;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_5::IWifiApIface;

namespace {
constexpr unsigned char kNwSsid[] = {'t', 'e', 's', 't', '1',
                                     '2', '3', '4', '5'};
constexpr char kNwPassphrase[] = "test12345";
constexpr char kInvalidMaxPskNwPassphrase[] =
    "0123456789012345678901234567890123456789012345678901234567890123456789";
constexpr char kInvalidMinPskNwPassphrase[] = "test";
constexpr int kIfaceChannel = 6;
constexpr int kIfaceInvalidChannel = 567;
constexpr uint8_t kTestZeroMacAddr[] = {[0 ... 5] = 0x0};
constexpr Ieee80211ReasonCode kTestDisconnectReasonCode =
    Ieee80211ReasonCode::WLAN_REASON_UNSPECIFIED;
}  // namespace

class HostapdHidlTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {
   public:
    virtual void SetUp() override {
        wifi_instance_name_ = std::get<0>(GetParam());
        hostapd_instance_name_ = std::get<1>(GetParam());
        stopSupplicantIfNeeded(wifi_instance_name_);
        startHostapdAndWaitForHidlService(wifi_instance_name_,
                                          hostapd_instance_name_);
        hostapd_ = IHostapd::getService(hostapd_instance_name_);
        ASSERT_NE(hostapd_.get(), nullptr);
        HIDL_INVOKE(hostapd_, setDebugParams, DebugLevel::EXCESSIVE);
        isAcsSupport_ = testing::checkSubstringInCommandOutput(
            "/system/bin/cmd wifi get-softap-supported-features",
            "wifi_softap_acs_supported");
        isWpa3SaeSupport_ = testing::checkSubstringInCommandOutput(
            "/system/bin/cmd wifi get-softap-supported-features",
            "wifi_softap_wpa3_sae_supported");
        isBridgedSupport_ = testing::checkSubstringInCommandOutput(
            "/system/bin/cmd wifi get-softap-supported-features",
            "wifi_softap_bridged_ap_supported");
    }

    virtual void TearDown() override {
        HIDL_INVOKE_VOID_WITHOUT_ARGUMENTS(hostapd_, terminate);
        //  Wait 3 seconds to allow terminate processing before kill hostapd.
        sleep(3);
        stopHostapd(wifi_instance_name_);
    }

   protected:
    bool isWpa3SaeSupport_ = false;
    bool isAcsSupport_ = false;
    bool isBridgedSupport_ = false;

    std::string setupApIfaceAndGetName(bool isBridged) {
        sp<IWifiApIface> wifi_ap_iface;
        if (isBridged) {
            wifi_ap_iface = getBridgedWifiApIface_1_5(wifi_instance_name_);
        } else {
            wifi_ap_iface = getWifiApIface_1_5(wifi_instance_name_);
        }
        EXPECT_NE(nullptr, wifi_ap_iface.get());

        const auto& status_and_name = HIDL_INVOKE(wifi_ap_iface, getName);
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_name.first.code);
        return status_and_name.second;
    }

    IHostapd::IfaceParams getIfaceParamsWithoutAcs(std::string iface_name) {
        ::android::hardware::wifi::hostapd::V1_0::IHostapd::IfaceParams
            iface_params;
        ::android::hardware::wifi::hostapd::V1_1::IHostapd::IfaceParams
            iface_params_1_1;
        ::android::hardware::wifi::hostapd::V1_2::IHostapd::IfaceParams
            iface_params_1_2;
        IHostapd::IfaceParams iface_params_1_3;

        std::vector<
            ::android::hardware::wifi::hostapd::V1_3::IHostapd::ChannelParams>
            vec_channelParams;

        ::android::hardware::wifi::hostapd::V1_3::IHostapd::ChannelParams
            channelParams_1_3;

        iface_params.ifaceName = iface_name;
        iface_params.hwModeParams.enable80211N = true;
        iface_params.hwModeParams.enable80211AC = false;
        iface_params.channelParams.enableAcs = false;
        iface_params.channelParams.acsShouldExcludeDfs = false;
        iface_params.channelParams.channel = kIfaceChannel;
        iface_params_1_1.V1_0 = iface_params;
        iface_params_1_2.V1_1 = iface_params_1_1;
        // Newly added attributes in V1_2
        iface_params_1_2.hwModeParams.enable80211AX = false;
        iface_params_1_2.hwModeParams.enable6GhzBand = false;
        iface_params_1_2.channelParams.bandMask = 0;
        iface_params_1_2.channelParams.bandMask |=
            IHostapd::BandMask::BAND_2_GHZ;

        // Newly added attributes in V1_3
        channelParams_1_3.channel = iface_params.channelParams.channel;
        channelParams_1_3.enableAcs = iface_params.channelParams.enableAcs;
        channelParams_1_3.bandMask = iface_params_1_2.channelParams.bandMask;
        channelParams_1_3.V1_2 = iface_params_1_2.channelParams;

        vec_channelParams.push_back(channelParams_1_3);
        iface_params_1_3.V1_2 = iface_params_1_2;
        iface_params_1_3.channelParamsList = vec_channelParams;
        return iface_params_1_3;
    }

    IHostapd::IfaceParams getIfaceParamsWithBridgedModeACS(
        std::string iface_name) {
        // First get the settings for WithoutAcs and then make changes
        IHostapd::IfaceParams iface_params_1_3 =
            getIfaceParamsWithoutAcs(iface_name);
        iface_params_1_3.V1_2.V1_1.V1_0.channelParams.enableAcs = true;
        iface_params_1_3.V1_2.V1_1.V1_0.channelParams.acsShouldExcludeDfs =
            true;

        std::vector<
            ::android::hardware::wifi::hostapd::V1_3::IHostapd::ChannelParams>
            vec_channelParams;

        vec_channelParams.push_back(iface_params_1_3.channelParamsList[0]);

        ::android::hardware::wifi::hostapd::V1_3::IHostapd::ChannelParams
            second_channelParams_1_3;
        second_channelParams_1_3.channel = 0;
        second_channelParams_1_3.enableAcs = true;
        second_channelParams_1_3.bandMask = 0;
        second_channelParams_1_3.bandMask |= IHostapd::BandMask::BAND_5_GHZ;
        second_channelParams_1_3.V1_2 = iface_params_1_3.V1_2.channelParams;
        second_channelParams_1_3.V1_2.bandMask = 0;
        second_channelParams_1_3.V1_2.bandMask |=
            IHostapd::BandMask::BAND_5_GHZ;
        vec_channelParams.push_back(second_channelParams_1_3);

        iface_params_1_3.channelParamsList = vec_channelParams;
        return iface_params_1_3;
    }

    IHostapd::IfaceParams getIfaceParamsWithAcs(std::string iface_name) {
        // First get the settings for WithoutAcs and then make changes
        IHostapd::IfaceParams iface_params_1_3 =
            getIfaceParamsWithoutAcs(iface_name);
        iface_params_1_3.V1_2.V1_1.V1_0.channelParams.enableAcs = true;
        iface_params_1_3.V1_2.V1_1.V1_0.channelParams.acsShouldExcludeDfs =
            true;
        iface_params_1_3.V1_2.V1_1.V1_0.channelParams.channel = 0;
        iface_params_1_3.V1_2.channelParams.bandMask |=
            IHostapd::BandMask::BAND_5_GHZ;
        iface_params_1_3.channelParamsList[0].channel =
            iface_params_1_3.V1_2.V1_1.V1_0.channelParams.channel;
        iface_params_1_3.channelParamsList[0].enableAcs =
            iface_params_1_3.V1_2.V1_1.V1_0.channelParams.enableAcs;
        iface_params_1_3.channelParamsList[0].V1_2 =
            iface_params_1_3.V1_2.channelParams;
        iface_params_1_3.channelParamsList[0].bandMask =
            iface_params_1_3.V1_2.channelParams.bandMask;
        return iface_params_1_3;
    }

    IHostapd::IfaceParams getIfaceParamsWithAcsAndFreqRange(
        std::string iface_name) {
        IHostapd::IfaceParams iface_params_1_3 =
            getIfaceParamsWithAcs(iface_name);
        ::android::hardware::wifi::hostapd::V1_2::IHostapd::AcsFrequencyRange
            acsFrequencyRange;
        acsFrequencyRange.start = 2412;
        acsFrequencyRange.end = 2462;
        std::vector<::android::hardware::wifi::hostapd::V1_2::IHostapd::
                        AcsFrequencyRange>
            vec_acsFrequencyRange;
        vec_acsFrequencyRange.push_back(acsFrequencyRange);
        iface_params_1_3.V1_2.channelParams.acsChannelFreqRangesMhz =
            vec_acsFrequencyRange;
        iface_params_1_3.channelParamsList[0].V1_2 =
            iface_params_1_3.V1_2.channelParams;
        return iface_params_1_3;
    }

    IHostapd::IfaceParams getIfaceParamsWithAcsAndInvalidFreqRange(
        std::string iface_name) {
        IHostapd::IfaceParams iface_params_1_3 =
            getIfaceParamsWithAcsAndFreqRange(iface_name);
        iface_params_1_3.V1_2.channelParams.acsChannelFreqRangesMhz[0].start =
            222;
        iface_params_1_3.V1_2.channelParams.acsChannelFreqRangesMhz[0].end =
            999;
        iface_params_1_3.channelParamsList[0].V1_2 =
            iface_params_1_3.V1_2.channelParams;
        return iface_params_1_3;
    }

    IHostapd::NetworkParams getOpenNwParams() {
        IHostapd::NetworkParams nw_params_1_3;
        ::android::hardware::wifi::hostapd::V1_2::IHostapd::NetworkParams
            nw_params_1_2;
        ::android::hardware::wifi::hostapd::V1_0::IHostapd::NetworkParams
            nw_params_1_0;
        nw_params_1_0.ssid =
            std::vector<uint8_t>(kNwSsid, kNwSsid + sizeof(kNwSsid));
        nw_params_1_0.isHidden = false;
        nw_params_1_2.V1_0 = nw_params_1_0;
        nw_params_1_2.encryptionType = IHostapd::EncryptionType::NONE;
        nw_params_1_3.V1_2 = nw_params_1_2;
        nw_params_1_3.isMetered = true;
        return nw_params_1_3;
    }

    IHostapd::NetworkParams getPskNwParamsWithNonMetered() {
        IHostapd::NetworkParams nw_params_1_3 = getOpenNwParams();
        nw_params_1_3.V1_2.encryptionType = IHostapd::EncryptionType::WPA2;
        nw_params_1_3.V1_2.passphrase = kNwPassphrase;
        nw_params_1_3.isMetered = false;
        return nw_params_1_3;
    }

    IHostapd::NetworkParams getPskNwParams() {
        IHostapd::NetworkParams nw_params_1_3 = getOpenNwParams();
        nw_params_1_3.V1_2.encryptionType = IHostapd::EncryptionType::WPA2;
        nw_params_1_3.V1_2.passphrase = kNwPassphrase;
        return nw_params_1_3;
    }

    IHostapd::NetworkParams getInvalidPskNwParams() {
        IHostapd::NetworkParams nw_params_1_3 = getOpenNwParams();
        nw_params_1_3.V1_2.encryptionType = IHostapd::EncryptionType::WPA2;
        nw_params_1_3.V1_2.passphrase = kInvalidMaxPskNwPassphrase;

        return nw_params_1_3;
    }

    IHostapd::NetworkParams getSaeTransitionNwParams() {
        IHostapd::NetworkParams nw_params_1_3 = getOpenNwParams();
        nw_params_1_3.V1_2.encryptionType =
            IHostapd::EncryptionType::WPA3_SAE_TRANSITION;
        nw_params_1_3.V1_2.passphrase = kNwPassphrase;
        return nw_params_1_3;
    }

    IHostapd::NetworkParams getInvalidSaeTransitionNwParams() {
        IHostapd::NetworkParams nw_params_1_3 = getOpenNwParams();
        nw_params_1_3.V1_2.encryptionType = IHostapd::EncryptionType::WPA2;
        nw_params_1_3.V1_2.passphrase = kInvalidMinPskNwPassphrase;
        return nw_params_1_3;
    }

    IHostapd::NetworkParams getSaeNwParams() {
        IHostapd::NetworkParams nw_params_1_3 = getOpenNwParams();
        nw_params_1_3.V1_2.encryptionType = IHostapd::EncryptionType::WPA3_SAE;
        nw_params_1_3.V1_2.passphrase = kNwPassphrase;
        return nw_params_1_3;
    }

    IHostapd::NetworkParams getInvalidSaeNwParams() {
        IHostapd::NetworkParams nw_params_1_3 = getOpenNwParams();
        nw_params_1_3.V1_2.encryptionType = IHostapd::EncryptionType::WPA3_SAE;
        nw_params_1_3.V1_2.passphrase = "";
        return nw_params_1_3;
    }

    IHostapd::IfaceParams getIfaceParamsWithInvalidChannel(
        std::string iface_name) {
        IHostapd::IfaceParams iface_params_1_3 =
            getIfaceParamsWithoutAcs(iface_name);
        iface_params_1_3.V1_2.V1_1.V1_0.channelParams.channel =
            kIfaceInvalidChannel;
        iface_params_1_3.channelParamsList[0].channel =
            iface_params_1_3.V1_2.V1_1.V1_0.channelParams.channel;
        return iface_params_1_3;
    }

    // IHostapd object used for all tests in this fixture.
    sp<IHostapd> hostapd_;
    std::string wifi_instance_name_;
    std::string hostapd_instance_name_;
};

/**
 * Adds an access point with PSK network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddPskAccessPointWithAcs) {
    if (!isAcsSupport_) GTEST_SKIP() << "Missing ACS support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                              getIfaceParamsWithAcs(ifname), getPskNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with PSK network config, ACS enabled & frequency Range.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddPskAccessPointWithAcsAndFreqRange) {
    if (!isAcsSupport_) GTEST_SKIP() << "Missing ACS support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                              getIfaceParamsWithAcsAndFreqRange(ifname),
                              getPskNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid channel range.
 * Access point creation should fail.
 */
TEST_P(HostapdHidlTest, AddPskAccessPointWithAcsAndInvalidFreqRange) {
    if (!isAcsSupport_) GTEST_SKIP() << "Missing ACS support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                              getIfaceParamsWithAcsAndInvalidFreqRange(ifname),
                              getPskNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with Open network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddOpenAccessPointWithAcs) {
    if (!isAcsSupport_) GTEST_SKIP() << "Missing ACS support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                              getIfaceParamsWithAcs(ifname), getOpenNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with PSK network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddPskAccessPointWithoutAcs) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                    getIfaceParamsWithoutAcs(ifname), getPskNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with PSK network config, ACS disabled & Non metered.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddPskAccessPointWithoutAcsAndNonMetered) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                              getIfaceParamsWithoutAcs(ifname),
                              getPskNwParamsWithNonMetered());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with Open network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddOpenAccessPointWithoutAcs) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                    getIfaceParamsWithoutAcs(ifname), getOpenNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with SAE Transition network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddSaeTransitionAccessPointWithoutAcs) {
    if (!isWpa3SaeSupport_) GTEST_SKIP() << "Missing SAE support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                              getIfaceParamsWithoutAcs(ifname),
                              getSaeTransitionNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with SAE network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddSAEAccessPointWithoutAcs) {
    if (!isWpa3SaeSupport_) GTEST_SKIP() << "Missing SAE support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                    getIfaceParamsWithoutAcs(ifname), getSaeNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds & then removes an access point with PSK network config & ACS enabled.
 * Access point creation & removal should pass.
 */
TEST_P(HostapdHidlTest, RemoveAccessPointWithAcs) {
    if (!isAcsSupport_) GTEST_SKIP() << "Missing ACS support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status_1_2 =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_3, getIfaceParamsWithAcs(ifname),
                    getPskNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status_1_2.code);
    auto status = HIDL_INVOKE(hostapd_, removeAccessPoint, ifname);
    EXPECT_EQ(
        android::hardware::wifi::hostapd::V1_0::HostapdStatusCode::SUCCESS,
        status.code);
}

/**
 * Adds & then removes an access point with PSK network config & ACS disabled.
 * Access point creation & removal should pass.
 */
TEST_P(HostapdHidlTest, RemoveAccessPointWithoutAcs) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status_1_2 =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                    getIfaceParamsWithoutAcs(ifname), getPskNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status_1_2.code);
    auto status = HIDL_INVOKE(hostapd_, removeAccessPoint, ifname);
    EXPECT_EQ(
        android::hardware::wifi::hostapd::V1_0::HostapdStatusCode::SUCCESS,
        status.code);
}

/**
 * Adds an access point with invalid channel.
 * Access point creation should fail.
 */
TEST_P(HostapdHidlTest, AddPskAccessPointWithInvalidChannel) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                    getIfaceParamsWithInvalidChannel(ifname), getPskNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid PSK network config.
 * Access point creation should fail.
 */
TEST_P(HostapdHidlTest, AddInvalidPskAccessPointWithoutAcs) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                    getIfaceParamsWithoutAcs(ifname), getInvalidPskNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid SAE transition network config.
 * Access point creation should fail.
 */
TEST_P(HostapdHidlTest, AddInvalidSaeTransitionAccessPointWithoutAcs) {
    if (!isWpa3SaeSupport_) GTEST_SKIP() << "Missing SAE support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                              getIfaceParamsWithoutAcs(ifname),
                              getInvalidSaeTransitionNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid SAE network config.
 * Access point creation should fail.
 */
TEST_P(HostapdHidlTest, AddInvalidSaeAccessPointWithoutAcs) {
    if (!isWpa3SaeSupport_) GTEST_SKIP() << "Missing SAE support";
    std::string ifname = setupApIfaceAndGetName(false);
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                    getIfaceParamsWithoutAcs(ifname), getInvalidSaeNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * forceClientDisconnect should return FAILURE_CLIENT_UNKNOWN
 * when hotspot interface available.
 */
TEST_P(HostapdHidlTest, DisconnectClientWhenIfacAvailable) {
    std::string ifname = setupApIfaceAndGetName(false);
    auto status_1_2 =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                    getIfaceParamsWithoutAcs(ifname), getOpenNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status_1_2.code);

    status_1_2 = HIDL_INVOKE(hostapd_, forceClientDisconnect, ifname,
                             kTestZeroMacAddr, kTestDisconnectReasonCode);
    EXPECT_EQ(HostapdStatusCode::FAILURE_CLIENT_UNKNOWN, status_1_2.code);
}

/**
 * AddAccessPointWithDualBandConfig should pass
 */
TEST_P(HostapdHidlTest, AddAccessPointWithDualBandConfig) {
    if (!isBridgedSupport_) GTEST_SKIP() << "Missing Bridged AP support";
    std::string ifname = setupApIfaceAndGetName(true);
    auto status_1_2 = HIDL_INVOKE(hostapd_, addAccessPoint_1_3,
                                  getIfaceParamsWithBridgedModeACS(ifname),
                                  getOpenNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status_1_2.code);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HostapdHidlTest);
INSTANTIATE_TEST_CASE_P(
    PerInstance, HostapdHidlTest,
    testing::Combine(
        testing::ValuesIn(
            android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::hostapd::V1_3::IHostapd::descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
