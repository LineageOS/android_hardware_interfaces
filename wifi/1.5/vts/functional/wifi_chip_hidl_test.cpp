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

#include <VtsHalHidlTargetCallbackBase.h>
#include <android-base/logging.h>

#undef NAN  // NAN is defined in bionic/libc/include/math.h:38

#include <android/hardware/wifi/1.4/IWifiChipEventCallback.h>
#include <android/hardware/wifi/1.5/IWifi.h>
#include <android/hardware/wifi/1.5/IWifiChip.h>
#include <android/hardware/wifi/1.5/IWifiStaIface.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::V1_0::ChipModeId;
using ::android::hardware::wifi::V1_0::IfaceType;
using ::android::hardware::wifi::V1_0::IWifiIface;
using ::android::hardware::wifi::V1_0::IWifiStaIface;
using ::android::hardware::wifi::V1_0::WifiDebugRingBufferStatus;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_4::IWifiChipEventCallback;
using ::android::hardware::wifi::V1_5::IWifiChip;
using ::android::hardware::wifi::V1_5::WifiBand;
using ::android::hardware::wifi::V1_5::WifiIfaceMode;

/**
 * Fixture to use for all Wifi chip HIDL interface tests.
 */
class WifiChipHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        // Make sure to start with a clean state
        stopWifi(GetInstanceName());

        wifi_chip_ = IWifiChip::castFrom(getWifiChip(GetInstanceName()));
        ASSERT_NE(nullptr, wifi_chip_.get());
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

   protected:
    // Helper function to configure the Chip in one of the supported modes.
    // Most of the non-mode-configuration-related methods require chip
    // to be first configured.
    ChipModeId configureChipForIfaceType(IfaceType type, bool expectSuccess) {
        ChipModeId mode_id;
        EXPECT_EQ(expectSuccess,
                  configureChipToSupportIfaceType(wifi_chip_, type, &mode_id));
        return mode_id;
    }

    WifiStatusCode createStaIface(sp<IWifiStaIface>* sta_iface) {
        const auto& status_and_iface = HIDL_INVOKE(wifi_chip_, createStaIface);
        *sta_iface = status_and_iface.second;
        return status_and_iface.first.code;
    }

    std::string getIfaceName(const sp<IWifiIface>& iface) {
        const auto& status_and_name = HIDL_INVOKE(iface, getName);
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_name.first.code);
        return status_and_name.second;
    }

    std::vector<sp<IWifiStaIface>> create2StaIfacesIfPossible() {
        configureChipForIfaceType(IfaceType::STA, true);
        sp<IWifiStaIface> iface1, iface2;
        EXPECT_EQ(WifiStatusCode::SUCCESS, createStaIface(&iface1));
        EXPECT_NE(nullptr, iface1.get());

        // Try to create 2nd iface
        auto status = createStaIface(&iface2);
        if (status != WifiStatusCode::SUCCESS) {
            return {iface1};
        }
        EXPECT_NE(nullptr, iface2.get());
        return {iface1, iface2};
    }

    sp<IWifiChip> wifi_chip_;

   private:
    std::string GetInstanceName() { return GetParam(); }
};

/*
 * setMultiStaPrimaryConnection
 *
 * Only run if device supports 2 STA ifaces.
 */
TEST_P(WifiChipHidlTest, setMultiStaPrimaryConnection) {
    auto ifaces = create2StaIfacesIfPossible();
    if (ifaces.size() < 2) {
        GTEST_SKIP() << "Device does not support more than 1 STA concurrently";
    }

    const auto& status = HIDL_INVOKE(wifi_chip_, setMultiStaPrimaryConnection,
                                     getIfaceName(ifaces.front()));
    if (status.code != WifiStatusCode::SUCCESS) {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
    }
}

/*
 * setMultiStaUseCase
 *
 * Only run if device supports 2 STA ifaces.
 */
TEST_P(WifiChipHidlTest, setMultiStaUseCase) {
    auto ifaces = create2StaIfacesIfPossible();
    if (ifaces.size() < 2) {
        GTEST_SKIP() << "Device does not support more than 1 STA concurrently";
    }

    const auto& status = HIDL_INVOKE(
        wifi_chip_, setMultiStaUseCase,
        IWifiChip::MultiStaUseCase::DUAL_STA_TRANSIENT_PREFER_PRIMARY);
    if (status.code != WifiStatusCode::SUCCESS) {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, status.code);
    }
}

/*
 * setCoexUnsafeChannels
 */
TEST_P(WifiChipHidlTest, setCoexUnsafeChannels) {
    configureChipForIfaceType(IfaceType::STA, true);
    // Test with empty vector of CoexUnsafeChannels
    std::vector<IWifiChip::CoexUnsafeChannel> vec;
    const auto& statusEmpty =
        HIDL_INVOKE(wifi_chip_, setCoexUnsafeChannels, vec, 0);
    if (statusEmpty.code != WifiStatusCode::SUCCESS) {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, statusEmpty.code);
    }

    // Test with non-empty vector of CoexUnsafeChannels
    IWifiChip::CoexUnsafeChannel unsafeChannel24Ghz;
    unsafeChannel24Ghz.band = WifiBand::BAND_24GHZ;
    unsafeChannel24Ghz.channel = 6;
    vec.push_back(unsafeChannel24Ghz);
    IWifiChip::CoexUnsafeChannel unsafeChannel5Ghz;
    unsafeChannel5Ghz.band = WifiBand::BAND_5GHZ;
    unsafeChannel5Ghz.channel = 36;
    vec.push_back(unsafeChannel5Ghz);
    uint32_t restrictions = IWifiChip::CoexRestriction::WIFI_AWARE |
                            IWifiChip::CoexRestriction::SOFTAP |
                            IWifiChip::CoexRestriction::WIFI_DIRECT;
    const auto& statusNonEmpty =
        HIDL_INVOKE(wifi_chip_, setCoexUnsafeChannels, vec, restrictions);
    if (statusNonEmpty.code != WifiStatusCode::SUCCESS) {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED, statusNonEmpty.code);
    }
}

/*
 * SetCountryCode:
 * Ensures that a call to set the country code will return with a success
 * status code.
 */
TEST_P(WifiChipHidlTest, setCountryCode) {
    const android::hardware::hidl_array<int8_t, 2> kCountryCode{
        std::array<int8_t, 2>{{0x55, 0x53}}};

    configureChipForIfaceType(IfaceType::STA, true);
    EXPECT_EQ(WifiStatusCode::SUCCESS,
              HIDL_INVOKE(wifi_chip_, setCountryCode, kCountryCode).code);
}

/* getUsableChannels:
 * Ensure that a call to getUsableChannels will return with a success
 * status for valid inputs.
 */
TEST_P(WifiChipHidlTest, getUsableChannels) {
    uint32_t ifaceModeMask =
        WifiIfaceMode::IFACE_MODE_P2P_CLIENT | WifiIfaceMode::IFACE_MODE_P2P_GO;
    uint32_t filterMask = IWifiChip::UsableChannelFilter::CELLULAR_COEXISTENCE |
                          IWifiChip::UsableChannelFilter::CONCURRENCY;
    configureChipForIfaceType(IfaceType::STA, true);
    WifiBand band = WifiBand::BAND_24GHZ_5GHZ_6GHZ;
    const auto& statusNonEmpty = HIDL_INVOKE(wifi_chip_, getUsableChannels,
                                             band, ifaceModeMask, filterMask);
    if (statusNonEmpty.first.code != WifiStatusCode::SUCCESS) {
        EXPECT_EQ(WifiStatusCode::ERROR_NOT_SUPPORTED,
                  statusNonEmpty.first.code);
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiChipHidlTest);
INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiChipHidlTest,
    testing::ValuesIn(android::hardware::getAllHalInstanceNames(
        ::android::hardware::wifi::V1_5::IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
