/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <android-base/logging.h>
#include <cutils/properties.h>

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <android/hardware/wifi/1.0/IWifi.h>
#include <android/hardware/wifi/hostapd/1.2/IHostapd.h>

#include "hostapd_hidl_call_util.h"
#include "hostapd_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::hostapd::V1_2::DebugLevel;
using ::android::hardware::wifi::hostapd::V1_2::HostapdStatusCode;
using ::android::hardware::wifi::hostapd::V1_2::Ieee80211ReasonCode;
using ::android::hardware::wifi::hostapd::V1_2::IHostapd;
using ::android::hardware::wifi::V1_0::IWifi;

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
    }

    virtual void TearDown() override { stopHostapd(hostapd_instance_name_); }

   protected:
    std::string getPrimaryWlanIfaceName() {
        std::array<char, PROPERTY_VALUE_MAX> buffer;
        auto res = property_get("ro.vendor.wifi.sap.interface", buffer.data(),
                                nullptr);
        if (res > 0) return buffer.data();
        property_get("wifi.interface", buffer.data(), "wlan0");
        return buffer.data();
    }

    IHostapd::IfaceParams getIfaceParamsWithoutAcs() {
        ::android::hardware::wifi::hostapd::V1_0::IHostapd::IfaceParams
            iface_params;
        ::android::hardware::wifi::hostapd::V1_1::IHostapd::IfaceParams
            iface_params_1_1;
        IHostapd::IfaceParams iface_params_1_2;

        iface_params.ifaceName = getPrimaryWlanIfaceName();
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
        return iface_params_1_2;
    }

    IHostapd::IfaceParams getIfaceParamsWithAcs() {
        // First get the settings for WithoutAcs and then make changes
        IHostapd::IfaceParams iface_params_1_2 = getIfaceParamsWithoutAcs();
        iface_params_1_2.V1_1.V1_0.channelParams.enableAcs = true;
        iface_params_1_2.V1_1.V1_0.channelParams.acsShouldExcludeDfs = true;
        iface_params_1_2.V1_1.V1_0.channelParams.channel = 0;
        iface_params_1_2.channelParams.bandMask |=
            IHostapd::BandMask::BAND_5_GHZ;

        return iface_params_1_2;
    }

    IHostapd::IfaceParams getIfaceParamsWithAcsAndFreqRange() {
        IHostapd::IfaceParams iface_params_1_2 = getIfaceParamsWithAcs();
        ::android::hardware::wifi::hostapd::V1_2::IHostapd::AcsFrequencyRange
            acsFrequencyRange;
        acsFrequencyRange.start = 2412;
        acsFrequencyRange.end = 2462;
        std::vector<::android::hardware::wifi::hostapd::V1_2::IHostapd::
                        AcsFrequencyRange>
            vec_acsFrequencyRange;
        vec_acsFrequencyRange.push_back(acsFrequencyRange);
        iface_params_1_2.channelParams.acsChannelFreqRangesMhz =
            vec_acsFrequencyRange;
        return iface_params_1_2;
    }

    IHostapd::IfaceParams getIfaceParamsWithAcsAndInvalidFreqRange() {
        IHostapd::IfaceParams iface_params_1_2 =
            getIfaceParamsWithAcsAndFreqRange();
        iface_params_1_2.channelParams.acsChannelFreqRangesMhz[0].start = 222;
        iface_params_1_2.channelParams.acsChannelFreqRangesMhz[0].end = 999;
        return iface_params_1_2;
    }

    IHostapd::NetworkParams getOpenNwParams() {
        IHostapd::NetworkParams nw_params_1_2;
        ::android::hardware::wifi::hostapd::V1_0::IHostapd::NetworkParams
            nw_params_1_0;
        nw_params_1_0.ssid =
            std::vector<uint8_t>(kNwSsid, kNwSsid + sizeof(kNwSsid));
        nw_params_1_0.isHidden = false;
        nw_params_1_2.V1_0 = nw_params_1_0;
        nw_params_1_2.encryptionType = IHostapd::EncryptionType::NONE;
        return nw_params_1_2;
    }

    IHostapd::NetworkParams getPskNwParams() {
        IHostapd::NetworkParams nw_params_1_2 = getOpenNwParams();
        nw_params_1_2.encryptionType = IHostapd::EncryptionType::WPA2;
        nw_params_1_2.passphrase = kNwPassphrase;
        return nw_params_1_2;
    }

    IHostapd::NetworkParams getInvalidPskNwParams() {
        IHostapd::NetworkParams nw_params_1_2 = getOpenNwParams();
        nw_params_1_2.encryptionType = IHostapd::EncryptionType::WPA2;
        nw_params_1_2.passphrase = kInvalidMaxPskNwPassphrase;

        return nw_params_1_2;
    }

    IHostapd::NetworkParams getSaeTransitionNwParams() {
        IHostapd::NetworkParams nw_params_1_2 = getOpenNwParams();
        nw_params_1_2.encryptionType =
            IHostapd::EncryptionType::WPA3_SAE_TRANSITION;
        nw_params_1_2.passphrase = kNwPassphrase;
        return nw_params_1_2;
    }

    IHostapd::NetworkParams getInvalidSaeTransitionNwParams() {
        IHostapd::NetworkParams nw_params_1_2 = getOpenNwParams();
        nw_params_1_2.encryptionType = IHostapd::EncryptionType::WPA2;
        nw_params_1_2.passphrase = kInvalidMinPskNwPassphrase;
        return nw_params_1_2;
    }

    IHostapd::NetworkParams getSaeNwParams() {
        IHostapd::NetworkParams nw_params_1_2 = getOpenNwParams();
        nw_params_1_2.encryptionType = IHostapd::EncryptionType::WPA3_SAE;
        nw_params_1_2.passphrase = kNwPassphrase;
        return nw_params_1_2;
    }

    IHostapd::NetworkParams getInvalidSaeNwParams() {
        IHostapd::NetworkParams nw_params_1_2 = getOpenNwParams();
        nw_params_1_2.encryptionType = IHostapd::EncryptionType::WPA3_SAE;
        nw_params_1_2.passphrase = "";
        return nw_params_1_2;
    }

    IHostapd::IfaceParams getIfaceParamsWithInvalidChannel() {
        IHostapd::IfaceParams iface_params_1_2 = getIfaceParamsWithoutAcs();
        iface_params_1_2.V1_1.V1_0.channelParams.channel = kIfaceInvalidChannel;
        return iface_params_1_2;
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
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_2,
                              getIfaceParamsWithAcs(), getPskNwParams());
    // TODO: b/140172237, fix this in R.
    // EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with PSK network config, ACS enabled & frequency Range.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddPskAccessPointWithAcsAndFreqRange) {
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_2,
                    getIfaceParamsWithAcsAndFreqRange(), getPskNwParams());
    // TODO: b/140172237, fix this in R
    // EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid channel range.
 * Access point creation should fail.
 */
TEST_P(HostapdHidlTest, AddPskAccessPointWithAcsAndInvalidFreqRange) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_2,
                              getIfaceParamsWithAcsAndInvalidFreqRange(),
                              getPskNwParams());
    // TODO: b/140172237, fix this in R
    // EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with Open network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddOpenAccessPointWithAcs) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_2,
                              getIfaceParamsWithAcs(), getOpenNwParams());
    // TODO: b/140172237, fix this in R
    // EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with PSK network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddPskAccessPointWithoutAcs) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_2,
                              getIfaceParamsWithoutAcs(), getPskNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with Open network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddOpenAccessPointWithoutAcs) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_2,
                              getIfaceParamsWithoutAcs(), getOpenNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with SAE Transition network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddSaeTransitionAccessPointWithoutAcs) {
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_2, getIfaceParamsWithoutAcs(),
                    getSaeTransitionNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with SAE network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_P(HostapdHidlTest, AddSAEAccessPointWithoutAcs) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_2,
                              getIfaceParamsWithoutAcs(), getSaeNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds & then removes an access point with PSK network config & ACS enabled.
 * Access point creation & removal should pass.
 */
TEST_P(HostapdHidlTest, RemoveAccessPointWithAcs) {
    auto status_1_2 = HIDL_INVOKE(hostapd_, addAccessPoint_1_2,
                                  getIfaceParamsWithAcs(), getPskNwParams());
    // TODO: b/140172237, fix this in R
    /*
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status_1_2.code);
    auto status =
        HIDL_INVOKE(hostapd_, removeAccessPoint, getPrimaryWlanIfaceName());
    EXPECT_EQ(android::hardware::wifi::hostapd::V1_0::HostapdStatusCode::SUCCESS,
    status.code);
    */
}

/**
 * Adds & then removes an access point with PSK network config & ACS disabled.
 * Access point creation & removal should pass.
 */
TEST_P(HostapdHidlTest, RemoveAccessPointWithoutAcs) {
    auto status_1_2 = HIDL_INVOKE(hostapd_, addAccessPoint_1_2,
                                  getIfaceParamsWithoutAcs(), getPskNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status_1_2.code);
    auto status =
        HIDL_INVOKE(hostapd_, removeAccessPoint, getPrimaryWlanIfaceName());
    EXPECT_EQ(
        android::hardware::wifi::hostapd::V1_0::HostapdStatusCode::SUCCESS,
        status.code);
}

/**
 * Adds an access point with invalid channel.
 * Access point creation should fail.
 */
TEST_P(HostapdHidlTest, AddPskAccessPointWithInvalidChannel) {
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_2,
                    getIfaceParamsWithInvalidChannel(), getPskNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid PSK network config.
 * Access point creation should fail.
 */
TEST_P(HostapdHidlTest, AddInvalidPskAccessPointWithoutAcs) {
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_2, getIfaceParamsWithoutAcs(),
                    getInvalidPskNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid SAE transition network config.
 * Access point creation should fail.
 */
TEST_P(HostapdHidlTest, AddInvalidSaeTransitionAccessPointWithoutAcs) {
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_2, getIfaceParamsWithoutAcs(),
                    getInvalidSaeTransitionNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid SAE network config.
 * Access point creation should fail.
 */
TEST_P(HostapdHidlTest, AddInvalidSaeAccessPointWithoutAcs) {
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_2, getIfaceParamsWithoutAcs(),
                    getInvalidSaeNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * forceClientDisconnect should return FAILURE_IFACE_UNKNOWN
 * when hotspot interface doesn't init..
 */
TEST_P(HostapdHidlTest, DisconnectClientWhenIfaceNotAvailable) {
    auto status =
        HIDL_INVOKE(hostapd_, forceClientDisconnect, getPrimaryWlanIfaceName(),
                    kTestZeroMacAddr, kTestDisconnectReasonCode);
    EXPECT_EQ(HostapdStatusCode::FAILURE_IFACE_UNKNOWN, status.code);
}

/**
 * forceClientDisconnect should return FAILURE_CLIENT_UNKNOWN
 * when hotspot interface available.
 */
TEST_P(HostapdHidlTest, DisconnectClientWhenIfacAvailable) {
    auto status_1_2 =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_2, getIfaceParamsWithoutAcs(),
                    getOpenNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status_1_2.code);

    status_1_2 =
        HIDL_INVOKE(hostapd_, forceClientDisconnect, getPrimaryWlanIfaceName(),
                    kTestZeroMacAddr, kTestDisconnectReasonCode);
    EXPECT_EQ(HostapdStatusCode::FAILURE_CLIENT_UNKNOWN, status_1_2.code);
}

/*
 * SetDebugParams
 */
TEST_P(HostapdHidlTest, SetDebugParams) {
    auto status = HIDL_INVOKE(hostapd_, setDebugParams, DebugLevel::EXCESSIVE);
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

INSTANTIATE_TEST_CASE_P(
    PerInstance, HostapdHidlTest,
    testing::Combine(
        testing::ValuesIn(
            android::hardware::getAllHalInstanceNames(IWifi::descriptor)),
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(
            android::hardware::wifi::hostapd::V1_2::IHostapd::descriptor))),
    android::hardware::PrintInstanceTupleNameToString<>);
