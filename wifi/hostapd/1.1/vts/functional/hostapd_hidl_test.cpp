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

#include <VtsHalHidlTargetTestBase.h>

#include <android/hardware/wifi/hostapd/1.1/IHostapd.h>

#include "hostapd_hidl_call_util.h"
#include "hostapd_hidl_test_utils.h"
#include "hostapd_hidl_test_utils_1_1.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatus;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatusCode;
using ::android::hardware::wifi::hostapd::V1_1::IHostapd;
using ::android::hardware::wifi::hostapd::V1_1::IHostapdCallback;

namespace {
constexpr unsigned char kNwSsid[] = {'t', 'e', 's', 't', '1',
                                     '2', '3', '4', '5'};
constexpr char kNwPassphrase[] = "test12345";
constexpr int kIfaceChannel = 6;
constexpr int kIfaceInvalidChannel = 567;
}  // namespace

class HostapdHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        stopSupplicantIfNeeded();
        startHostapdAndWaitForHidlService();
        hostapd_ = getHostapd_1_1();
        ASSERT_NE(hostapd_.get(), nullptr);
    }

    virtual void TearDown() override { stopHostapd(); }

   protected:
    std::string getPrimaryWlanIfaceName() {
        std::array<char, PROPERTY_VALUE_MAX> buffer;
        auto res = property_get("ro.vendor.wifi.sap.interface",
                                buffer.data(), nullptr);
        if (res > 0) return buffer.data();
        property_get("wifi.interface", buffer.data(), "wlan0");
        return buffer.data();
    }

    IHostapd::IfaceParams getIfaceParamsWithAcs() {
        ::android::hardware::wifi::hostapd::V1_0::IHostapd::IfaceParams
            iface_params;
        IHostapd::IfaceParams iface_params_1_1;

        iface_params.ifaceName = getPrimaryWlanIfaceName();
        iface_params.hwModeParams.enable80211N = true;
        iface_params.hwModeParams.enable80211AC = false;
        iface_params.channelParams.enableAcs = true;
        iface_params.channelParams.acsShouldExcludeDfs = true;
        iface_params.channelParams.channel = 0;
        iface_params.channelParams.band = IHostapd::Band::BAND_ANY;
        iface_params_1_1.V1_0 = iface_params;
        return iface_params_1_1;
    }

    IHostapd::IfaceParams getIfaceParamsWithAcsAndChannelRange() {
        IHostapd::IfaceParams iface_params_1_1 = getIfaceParamsWithAcs();
        IHostapd::ChannelParams channelParams;
        IHostapd::AcsChannelRange acsChannelRange;
        acsChannelRange.start = 1;
        acsChannelRange.end = 11;
        std::vector<IHostapd::AcsChannelRange> vec_acsChannelRange;
        vec_acsChannelRange.push_back(acsChannelRange);
        channelParams.acsChannelRanges = vec_acsChannelRange;
        iface_params_1_1.channelParams = channelParams;
        return iface_params_1_1;
    }

    IHostapd::IfaceParams getIfaceParamsWithAcsAndInvalidChannelRange() {
        IHostapd::IfaceParams iface_params_1_1 =
            getIfaceParamsWithAcsAndChannelRange();
        iface_params_1_1.channelParams.acsChannelRanges[0].start = 222;
        iface_params_1_1.channelParams.acsChannelRanges[0].end = 999;
        return iface_params_1_1;
    }

    IHostapd::IfaceParams getIfaceParamsWithoutAcs() {
        ::android::hardware::wifi::hostapd::V1_0::IHostapd::IfaceParams
            iface_params;
        IHostapd::IfaceParams iface_params_1_1;

        iface_params.ifaceName = getPrimaryWlanIfaceName();
        iface_params.hwModeParams.enable80211N = true;
        iface_params.hwModeParams.enable80211AC = false;
        iface_params.channelParams.enableAcs = false;
        iface_params.channelParams.acsShouldExcludeDfs = false;
        iface_params.channelParams.channel = kIfaceChannel;
        iface_params.channelParams.band = IHostapd::Band::BAND_2_4_GHZ;
        iface_params_1_1.V1_0 = iface_params;
        return iface_params_1_1;
    }

    IHostapd::IfaceParams getIfaceParamsWithInvalidChannel() {
        IHostapd::IfaceParams iface_params_1_1 = getIfaceParamsWithoutAcs();
        iface_params_1_1.V1_0.channelParams.channel = kIfaceInvalidChannel;
        return iface_params_1_1;
    }

    IHostapd::NetworkParams getPskNwParams() {
        IHostapd::NetworkParams nw_params;
        nw_params.ssid =
            std::vector<uint8_t>(kNwSsid, kNwSsid + sizeof(kNwSsid));
        nw_params.isHidden = false;
        nw_params.encryptionType = IHostapd::EncryptionType::WPA2;
        nw_params.pskPassphrase = kNwPassphrase;
        return nw_params;
    }

    IHostapd::NetworkParams getInvalidPskNwParams() {
        IHostapd::NetworkParams nw_params;
        nw_params.ssid =
            std::vector<uint8_t>(kNwSsid, kNwSsid + sizeof(kNwSsid));
        nw_params.isHidden = false;
        nw_params.encryptionType = IHostapd::EncryptionType::WPA2;
        return nw_params;
    }

    IHostapd::NetworkParams getOpenNwParams() {
        IHostapd::NetworkParams nw_params;
        nw_params.ssid =
            std::vector<uint8_t>(kNwSsid, kNwSsid + sizeof(kNwSsid));
        nw_params.isHidden = false;
        nw_params.encryptionType = IHostapd::EncryptionType::NONE;
        return nw_params;
    }

    // IHostapd object used for all tests in this fixture.
    sp<IHostapd> hostapd_;
};

class IfaceCallback : public IHostapdCallback {
    Return<void> onFailure(
        const hidl_string& /* Name of the interface */) override {
        return Void();
    }
};

/*
 * RegisterCallback
 */
TEST_F(HostapdHidlTest, registerCallback) {
    hostapd_->registerCallback(
        new IfaceCallback(), [](const HostapdStatus& status) {
            EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
        });
}

/**
 * Adds an access point with PSK network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_F(HostapdHidlTest, AddPskAccessPointWithAcs) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_1,
                              getIfaceParamsWithAcs(), getPskNwParams());
    // TODO: b/140172237, fix this in R.
    // EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with PSK network config, ACS enabled & channel Range.
 * Access point creation should pass.
 */
TEST_F(HostapdHidlTest, AddPskAccessPointWithAcsAndChannelRange) {
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_1,
                    getIfaceParamsWithAcsAndChannelRange(), getPskNwParams());
    // TODO: b/140172237, fix this in R
    // EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid channel range.
 * Access point creation should fail.
 */
TEST_F(HostapdHidlTest, AddPskAccessPointWithAcsAndInvalidChannelRange) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_1,
                              getIfaceParamsWithAcsAndInvalidChannelRange(),
                              getPskNwParams());
    // TODO: b/140172237, fix this in R
    // EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with Open network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_F(HostapdHidlTest, AddOpenAccessPointWithAcs) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_1,
                              getIfaceParamsWithAcs(), getOpenNwParams());
    // TODO: b/140172237, fix this in R
    // EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with PSK network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_F(HostapdHidlTest, AddPskAccessPointWithoutAcs) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_1,
                              getIfaceParamsWithoutAcs(), getPskNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with Open network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_F(HostapdHidlTest, AddOpenAccessPointWithoutAcs) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_1,
                              getIfaceParamsWithoutAcs(), getOpenNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds & then removes an access point with PSK network config & ACS enabled.
 * Access point creation & removal should pass.
 */
TEST_F(HostapdHidlTest, RemoveAccessPointWithAcs) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_1,
                              getIfaceParamsWithAcs(), getPskNwParams());
    // TODO: b/140172237, fix this in R
    /*
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
    status =
        HIDL_INVOKE(hostapd_, removeAccessPoint, getPrimaryWlanIfaceName());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
    */
}

/**
 * Adds & then removes an access point with PSK network config & ACS disabled.
 * Access point creation & removal should pass.
 */
TEST_F(HostapdHidlTest, RemoveAccessPointWithoutAcs) {
    auto status = HIDL_INVOKE(hostapd_, addAccessPoint_1_1,
                              getIfaceParamsWithoutAcs(), getPskNwParams());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
    status =
        HIDL_INVOKE(hostapd_, removeAccessPoint, getPrimaryWlanIfaceName());
    EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid channel.
 * Access point creation should fail.
 */
TEST_F(HostapdHidlTest, AddPskAccessPointWithInvalidChannel) {
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_1,
                    getIfaceParamsWithInvalidChannel(), getPskNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}

/**
 * Adds an access point with invalid PSK network config.
 * Access point creation should fail.
 */
TEST_F(HostapdHidlTest, AddInvalidPskAccessPointWithoutAcs) {
    auto status =
        HIDL_INVOKE(hostapd_, addAccessPoint_1_1, getIfaceParamsWithoutAcs(),
                    getInvalidPskNwParams());
    EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
}
