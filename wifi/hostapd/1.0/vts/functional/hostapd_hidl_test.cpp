/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <android/hardware/wifi/hostapd/1.0/IHostapd.h>

#include "hostapd_hidl_call_util.h"
#include "hostapd_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::hostapd::V1_0::IHostapd;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatus;
using ::android::hardware::wifi::hostapd::V1_0::HostapdStatusCode;

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
        hostapd_ = getHostapd();
        ASSERT_NE(hostapd_.get(), nullptr);
    }

    virtual void TearDown() override { stopHostapd(); }

   protected:
    std::string getPrimaryWlanIfaceName() {
        std::array<char, PROPERTY_VALUE_MAX> buffer;
        property_get("wifi.interface", buffer.data(), "wlan0");
        return buffer.data();
    }

    IHostapd::IfaceParams getIfaceParamsWithAcs() {
        IHostapd::IfaceParams iface_params;
        iface_params.ifaceName = getPrimaryWlanIfaceName();
        iface_params.hwModeParams.enable80211N = true;
        iface_params.hwModeParams.enable80211AC = false;
        iface_params.channelParams.enableAcs = true;
        iface_params.channelParams.acsShouldExcludeDfs = true;
        iface_params.channelParams.channel = 0;
        iface_params.channelParams.band = IHostapd::Band::BAND_ANY;
        return iface_params;
    }

    IHostapd::IfaceParams getIfaceParamsWithoutAcs() {
        IHostapd::IfaceParams iface_params;
        iface_params.ifaceName = getPrimaryWlanIfaceName();
        iface_params.hwModeParams.enable80211N = true;
        iface_params.hwModeParams.enable80211AC = false;
        iface_params.channelParams.enableAcs = false;
        iface_params.channelParams.acsShouldExcludeDfs = false;
        iface_params.channelParams.channel = kIfaceChannel;
        iface_params.channelParams.band = IHostapd::Band::BAND_2_4_GHZ;
        return iface_params;
    }

    IHostapd::IfaceParams getIfaceParamsWithInvalidChannel() {
        IHostapd::IfaceParams iface_params;
        iface_params.ifaceName = getPrimaryWlanIfaceName();
        iface_params.hwModeParams.enable80211N = true;
        iface_params.hwModeParams.enable80211AC = false;
        iface_params.channelParams.enableAcs = false;
        iface_params.channelParams.acsShouldExcludeDfs = false;
        iface_params.channelParams.channel = kIfaceInvalidChannel;
        iface_params.channelParams.band = IHostapd::Band::BAND_2_4_GHZ;
        return iface_params;
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

/*
 * Create:
 * Ensures that an instance of the IHostapd proxy object is
 * successfully created.
 */
TEST(HostapdHidlTestNoFixture, Create) {
    startHostapdAndWaitForHidlService();
    EXPECT_NE(nullptr, getHostapd().get());
    stopHostapd();
}

/**
 * Adds an access point with PSK network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_F(HostapdHidlTest, AddPskAccessPointWithAcs) {
    if (!is_1_1(hostapd_)) {
        auto status = HIDL_INVOKE(hostapd_, addAccessPoint,
                                  getIfaceParamsWithAcs(), getPskNwParams());
        // TODO: b/140172237, fix this in R
        // EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
    }
}

/**
 * Adds an access point with Open network config & ACS enabled.
 * Access point creation should pass.
 */
TEST_F(HostapdHidlTest, AddOpenAccessPointWithAcs) {
    if (!is_1_1(hostapd_)) {
        auto status = HIDL_INVOKE(hostapd_, addAccessPoint,
                                  getIfaceParamsWithAcs(), getOpenNwParams());
        // TODO: b/140172237, fix this in R
        // EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
    }
}

/**
 * Adds an access point with PSK network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_F(HostapdHidlTest, AddPskAccessPointWithoutAcs) {
    if (!is_1_1(hostapd_)) {
        auto status = HIDL_INVOKE(hostapd_, addAccessPoint,
                                  getIfaceParamsWithoutAcs(), getPskNwParams());
        EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
    }
}

/**
 * Adds an access point with Open network config & ACS disabled.
 * Access point creation should pass.
 */
TEST_F(HostapdHidlTest, AddOpenAccessPointWithoutAcs) {
    if (!is_1_1(hostapd_)) {
        auto status =
            HIDL_INVOKE(hostapd_, addAccessPoint, getIfaceParamsWithoutAcs(),
                        getOpenNwParams());
        EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
    }
}

/**
 * Adds & then removes an access point with PSK network config & ACS enabled.
 * Access point creation & removal should pass.
 */
TEST_F(HostapdHidlTest, RemoveAccessPointWithAcs) {
    if (!is_1_1(hostapd_)) {
        auto status = HIDL_INVOKE(hostapd_, addAccessPoint,
                                  getIfaceParamsWithAcs(), getPskNwParams());
        // TODO: b/140172237, fix this in R
        /*
        EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
        status =
            HIDL_INVOKE(hostapd_, removeAccessPoint, getPrimaryWlanIfaceName());
        EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
        */
    }
}

/**
 * Adds & then removes an access point with PSK network config & ACS disabled.
 * Access point creation & removal should pass.
 */
TEST_F(HostapdHidlTest, RemoveAccessPointWithoutAcs) {
    if (!is_1_1(hostapd_)) {
        auto status = HIDL_INVOKE(hostapd_, addAccessPoint,
                                  getIfaceParamsWithoutAcs(), getPskNwParams());
        EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
        status =
            HIDL_INVOKE(hostapd_, removeAccessPoint, getPrimaryWlanIfaceName());
        EXPECT_EQ(HostapdStatusCode::SUCCESS, status.code);
    }
}

/**
 * Adds an access point with invalid channel.
 * Access point creation should fail.
 */
TEST_F(HostapdHidlTest, AddPskAccessPointWithInvalidChannel) {
    if (!is_1_1(hostapd_)) {
        auto status =
            HIDL_INVOKE(hostapd_, addAccessPoint,
                        getIfaceParamsWithInvalidChannel(), getPskNwParams());
        EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
    }
}

/**
 * Adds an access point with invalid PSK network config.
 * Access point creation should fail.
 */
TEST_F(HostapdHidlTest, AddInvalidPskAccessPointWithoutAcs) {
    if (!is_1_1(hostapd_)) {
        auto status =
            HIDL_INVOKE(hostapd_, addAccessPoint, getIfaceParamsWithoutAcs(),
                        getInvalidPskNwParams());
        EXPECT_NE(HostapdStatusCode::SUCCESS, status.code);
    }
}

/*
 * Terminate
 * This terminates the service.
 */
TEST_F(HostapdHidlTest, Terminate) {
    hostapd_->terminate();
}
