/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include <android/hardware/wifi/1.6/IWifi.h>
#include <android/hardware/wifi/1.6/IWifiChip.h>
#include <android/hardware/wifi/1.6/IWifiStaIface.h>
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
using ::android::hardware::wifi::V1_5::WifiBand;
using ::android::hardware::wifi::V1_5::WifiIfaceMode;
using ::android::hardware::wifi::V1_6::IWifiChip;

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
        EXPECT_EQ(expectSuccess, configureChipToSupportIfaceType(wifi_chip_, type, &mode_id));
        return mode_id;
    }

    sp<IWifiChip> wifi_chip_;

  private:
    std::string GetInstanceName() { return GetParam(); }
};

/* getUsableChannels_1_6:
 * Ensure that a call to getUsableChannels_1_6 will return with a success
 * status for valid inputs.
 */
TEST_P(WifiChipHidlTest, getUsableChannels_1_6) {
    uint32_t ifaceModeMask =
            WifiIfaceMode::IFACE_MODE_P2P_CLIENT | WifiIfaceMode::IFACE_MODE_P2P_GO;
    uint32_t filterMask = IWifiChip::UsableChannelFilter::CELLULAR_COEXISTENCE |
                          IWifiChip::UsableChannelFilter::CONCURRENCY;
    configureChipForIfaceType(IfaceType::STA, true);
    WifiBand band = WifiBand::BAND_24GHZ_5GHZ_6GHZ;
    const auto& statusNonEmpty =
            HIDL_INVOKE(wifi_chip_, getUsableChannels_1_6, band, ifaceModeMask, filterMask);
    if (statusNonEmpty.first.code == WifiStatusCode::ERROR_NOT_SUPPORTED) {
        GTEST_SKIP() << "Skipping this test since getUsableChannels() is not supported by vendor.";
    }

    EXPECT_EQ(WifiStatusCode::SUCCESS, statusNonEmpty.first.code);
}

/* getAvailableModes_1_6:
 * Ensures that a call to getAvailableModes_1_6 will return with a success status code.
 */
TEST_P(WifiChipHidlTest, getAvailableModes_1_6) {
    const auto& status_and_modes = HIDL_INVOKE(wifi_chip_, getAvailableModes_1_6);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_modes.first.code);
    EXPECT_LT(0u, status_and_modes.second.size());
}

/*
 * getSupportedRadioCombinationsMatrix:
 * Ensure that a call to getSupportedRadioCombinationsMatrix will return
 * with a success status code.
 */
TEST_P(WifiChipHidlTest, getSupportedRadioCombinationsMatrix) {
    configureChipForIfaceType(IfaceType::STA, true);
    const auto& statusNonEmpty = HIDL_INVOKE(wifi_chip_, getSupportedRadioCombinationsMatrix);
    if (statusNonEmpty.first.code == WifiStatusCode::ERROR_NOT_SUPPORTED) {
        GTEST_SKIP() << "Skipping this test since getSupportedRadioCombinationsMatrix() is not "
                        "supported by vendor.";
    }

    EXPECT_EQ(WifiStatusCode::SUCCESS, statusNonEmpty.first.code);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiChipHidlTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, WifiChipHidlTest,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 ::android::hardware::wifi::V1_6::IWifi::descriptor)),
                         android::hardware::PrintInstanceNameToString);
