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
#include <VtsCoreUtil.h>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android/hardware/wifi/hostapd/BnHostapd.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

using android::ProcessState;
using android::sp;
using android::String16;
using android::hardware::wifi::hostapd::BandMask;
using android::hardware::wifi::hostapd::ChannelParams;
using android::hardware::wifi::hostapd::DebugLevel;
using android::hardware::wifi::hostapd::EncryptionType;
using android::hardware::wifi::hostapd::FrequencyRange;
using android::hardware::wifi::hostapd::Ieee80211ReasonCode;
using android::hardware::wifi::hostapd::IfaceParams;
using android::hardware::wifi::hostapd::IHostapd;
using android::hardware::wifi::hostapd::NetworkParams;

namespace {
const unsigned char kNwSsid[] = {'t', 'e', 's', 't', '1', '2', '3', '4', '5'};
const String16 kIfaceName = String16("wlan0");
const String16 kPassphrase = String16("test12345");
const String16 kInvalidMinPassphrase = String16("test");
const String16 kInvalidMaxPassphrase = String16(
    "0123456789012345678901234567890123456789012345678901234567890123456789");
const int kIfaceChannel = 6;
const int kIfaceInvalidChannel = 567;
const std::vector<uint8_t> kTestZeroMacAddr(6, 0x0);
const Ieee80211ReasonCode kTestDisconnectReasonCode =
    Ieee80211ReasonCode::WLAN_REASON_UNSPECIFIED;

inline BandMask operator|(BandMask a, BandMask b) {
    return static_cast<BandMask>(static_cast<int32_t>(a) |
                                 static_cast<int32_t>(b));
}
}  // namespace

class HostapdAidl : public testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        hostapd = android::waitForDeclaredService<IHostapd>(
            String16(GetParam().c_str()));
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
    }

    sp<IHostapd> hostapd;
    bool isAcsSupport;
    bool isWpa3SaeSupport;
    bool isBridgedSupport;

    IfaceParams getIfaceParamsWithoutAcs(String16 iface_name) {
        IfaceParams iface_params;
        ChannelParams channelParams;
        std::vector<ChannelParams> vec_channelParams;

        iface_params.name = iface_name;
        iface_params.hwModeParams.enable80211N = true;
        iface_params.hwModeParams.enable80211AC = false;
        iface_params.hwModeParams.enable80211AX = false;
        iface_params.hwModeParams.enable6GhzBand = false;

        channelParams.enableAcs = false;
        channelParams.acsShouldExcludeDfs = false;
        channelParams.channel = kIfaceChannel;
        channelParams.bandMask = BandMask::BAND_2_GHZ;

        vec_channelParams.push_back(channelParams);
        iface_params.channelParams = vec_channelParams;
        return iface_params;
    }

    IfaceParams getIfaceParamsWithBridgedModeACS(String16 iface_name) {
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

    IfaceParams getIfaceParamsWithAcs(String16 iface_name) {
        IfaceParams iface_params = getIfaceParamsWithoutAcs(iface_name);
        iface_params.channelParams[0].enableAcs = true;
        iface_params.channelParams[0].acsShouldExcludeDfs = true;
        iface_params.channelParams[0].channel = 0;
        iface_params.channelParams[0].bandMask =
            iface_params.channelParams[0].bandMask | BandMask::BAND_5_GHZ;
        return iface_params;
    }

    IfaceParams getIfaceParamsWithAcsAndFreqRange(String16 iface_name) {
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

    IfaceParams getIfaceParamsWithAcsAndInvalidFreqRange(String16 iface_name) {
        IfaceParams iface_params =
            getIfaceParamsWithAcsAndFreqRange(iface_name);
        iface_params.channelParams[0].acsChannelFreqRangesMhz[0].startMhz =
            222;
        iface_params.channelParams[0].acsChannelFreqRangesMhz[0].endMhz =
            999;
        return iface_params;
    }

    IfaceParams getIfaceParamsWithInvalidChannel(String16 iface_name) {
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
        nw_params.passphrase = String16("");
        return nw_params;
    }
};

/**
 * Adds an access point with PSK network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithAcs) {
    if (!isAcsSupport) GTEST_SKIP() << "Missing ACS support";
    auto status = hostapd->addAccessPoint(getIfaceParamsWithAcs(kIfaceName),
                                          getPskNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with PSK network config, ACS enabled & frequency Range.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithAcsAndFreqRange) {
    if (!isAcsSupport) GTEST_SKIP() << "Missing ACS support";
    auto status = hostapd->addAccessPoint(
        getIfaceParamsWithAcsAndFreqRange(kIfaceName), getPskNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with invalid channel range.
 * Access point creation should fail.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithAcsAndInvalidFreqRange) {
    if (!isAcsSupport) GTEST_SKIP() << "Missing ACS support";
    auto status = hostapd->addAccessPoint(
        getIfaceParamsWithAcsAndInvalidFreqRange(kIfaceName), getPskNwParams());
    EXPECT_FALSE(status.isOk());
}

/**
 * Adds an access point with Open network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddOpenAccessPointWithAcs) {
    if (!isAcsSupport) GTEST_SKIP() << "Missing ACS support";
    auto status = hostapd->addAccessPoint(getIfaceParamsWithAcs(kIfaceName),
                                          getOpenNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with PSK network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithoutAcs) {
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(kIfaceName),
                                          getPskNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with PSK network config, ACS disabled & Non metered.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithoutAcsAndNonMetered) {
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(kIfaceName),
                                          getPskNwParamsWithNonMetered());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with Open network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddOpenAccessPointWithoutAcs) {
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(kIfaceName),
                                          getOpenNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with SAE Transition network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddSaeTransitionAccessPointWithoutAcs) {
    if (!isWpa3SaeSupport) GTEST_SKIP() << "Missing SAE support";
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(kIfaceName),
                                          getSaeTransitionNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds an access point with SAE network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdAidl, AddSAEAccessPointWithoutAcs) {
    if (!isWpa3SaeSupport) GTEST_SKIP() << "Missing SAE support";
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(kIfaceName),
                                          getSaeNwParams());
    EXPECT_TRUE(status.isOk());
}

/**
 * Adds & then removes an access point with PSK network config & ACS enabled.
 * Access point creation & removal should pass.
 */
TEST_P(HostapdAidl, RemoveAccessPointWithAcs) {
    if (!isAcsSupport) GTEST_SKIP() << "Missing ACS support";
    auto status = hostapd->addAccessPoint(getIfaceParamsWithAcs(kIfaceName),
                                          getPskNwParams());
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(hostapd->removeAccessPoint(kIfaceName).isOk());
}

/**
 * Adds & then removes an access point with PSK network config & ACS disabled.
 * Access point creation & removal should pass.
 */
TEST_P(HostapdAidl, RemoveAccessPointWithoutAcs) {
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(kIfaceName),
                                          getPskNwParams());
    EXPECT_TRUE(status.isOk());
    EXPECT_TRUE(hostapd->removeAccessPoint(kIfaceName).isOk());
}

/**
 * Adds an access point with invalid channel.
 * Access point creation should fail.
 */
TEST_P(HostapdAidl, AddPskAccessPointWithInvalidChannel) {
    auto status = hostapd->addAccessPoint(
        getIfaceParamsWithInvalidChannel(kIfaceName), getPskNwParams());
    EXPECT_FALSE(status.isOk());
}

/**
 * Adds an access point with invalid PSK network config.
 * Access point creation should fail.
 */
TEST_P(HostapdAidl, AddInvalidPskAccessPointWithoutAcs) {
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(kIfaceName),
                                          getInvalidPskNwParams());
    EXPECT_FALSE(status.isOk());
}

/**
 * Adds an access point with invalid SAE transition network config.
 * Access point creation should fail.
 */
TEST_P(HostapdAidl, AddInvalidSaeTransitionAccessPointWithoutAcs) {
    if (!isWpa3SaeSupport) GTEST_SKIP() << "Missing SAE support";
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(kIfaceName),
                                          getInvalidSaeTransitionNwParams());
    EXPECT_FALSE(status.isOk());
}

/**
 * Adds an access point with invalid SAE network config.
 * Access point creation should fail.
 */
TEST_P(HostapdAidl, AddInvalidSaeAccessPointWithoutAcs) {
    if (!isWpa3SaeSupport) GTEST_SKIP() << "Missing SAE support";
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(kIfaceName),
                                          getInvalidSaeNwParams());
    EXPECT_FALSE(status.isOk());
}

/**
 * forceClientDisconnect should fail when hotspot interface available.
 */
TEST_P(HostapdAidl, DisconnectClientWhenIfacAvailable) {
    auto status = hostapd->addAccessPoint(getIfaceParamsWithoutAcs(kIfaceName),
                                          getOpenNwParams());
    EXPECT_TRUE(status.isOk());

    status = hostapd->forceClientDisconnect(kIfaceName, kTestZeroMacAddr,
                                            kTestDisconnectReasonCode);
    EXPECT_FALSE(status.isOk());
}

/**
 * AddAccessPointWithDualBandConfig should pass
 */
TEST_P(HostapdAidl, AddAccessPointWithDualBandConfig) {
    if (!isBridgedSupport) GTEST_SKIP() << "Missing Bridged AP support";
    auto status = hostapd->addAccessPoint(
        getIfaceParamsWithBridgedModeACS(kIfaceName), getOpenNwParams());
    EXPECT_TRUE(status.isOk());
}

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
