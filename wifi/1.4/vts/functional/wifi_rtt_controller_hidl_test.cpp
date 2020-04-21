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

#include <android/hardware/wifi/1.3/IWifiStaIface.h>
#include <android/hardware/wifi/1.4/IWifi.h>
#include <android/hardware/wifi/1.4/IWifiChip.h>
#include <android/hardware/wifi/1.4/IWifiRttController.h>
#include <android/hardware/wifi/1.4/IWifiRttControllerEventCallback.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::V1_0::CommandId;
using ::android::hardware::wifi::V1_0::RttBw;
using ::android::hardware::wifi::V1_0::RttPeerType;
using ::android::hardware::wifi::V1_0::RttType;
using ::android::hardware::wifi::V1_0::WifiChannelInfo;
using ::android::hardware::wifi::V1_0::WifiChannelWidthInMhz;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_3::IWifiStaIface;
using ::android::hardware::wifi::V1_4::IWifiChip;
using ::android::hardware::wifi::V1_4::IWifiRttController;
using ::android::hardware::wifi::V1_4::IWifiRttControllerEventCallback;
using ::android::hardware::wifi::V1_4::RttCapabilities;
using ::android::hardware::wifi::V1_4::RttConfig;
using ::android::hardware::wifi::V1_4::RttPreamble;
using ::android::hardware::wifi::V1_4::RttResponder;
using ::android::hardware::wifi::V1_4::RttResult;

/**
 * Fixture to use for all RTT controller HIDL interface tests.
 */
class WifiRttControllerHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        // Make sure to start with a clean state
        stopWifi(GetInstanceName());

        wifi_rtt_controller_ = getWifiRttController();
        ASSERT_NE(nullptr, wifi_rtt_controller_.get());

        // Check RTT support before we run the test.
        std::pair<WifiStatus, RttCapabilities> status_and_caps;
        status_and_caps =
            HIDL_INVOKE(wifi_rtt_controller_, getCapabilities_1_4);
        if (status_and_caps.first.code == WifiStatusCode::ERROR_NOT_SUPPORTED) {
            GTEST_SKIP() << "Skipping this test since RTT is not supported.";
        }
    }

    virtual void TearDown() override { stopWifi(GetInstanceName()); }

    // A simple test implementation of WifiChipEventCallback.
    class WifiRttControllerEventCallback
        : public ::testing::VtsHalHidlTargetCallbackBase<
              WifiRttControllerHidlTest>,
          public IWifiRttControllerEventCallback {
       public:
        WifiRttControllerEventCallback(){};

        virtual ~WifiRttControllerEventCallback() = default;

        Return<void> onResults(
            CommandId cmdId __unused,
            const hidl_vec<::android::hardware::wifi::V1_0::RttResult>& results
                __unused) {
            return Void();
        };

        Return<void> onResults_1_4(CommandId cmdId __unused,
                                   const hidl_vec<RttResult>& results
                                       __unused) {
            return Void();
        };
    };

   protected:
    sp<IWifiRttController> wifi_rtt_controller_;

   private:
    std::string GetInstanceName() { return GetParam(); }

    sp<IWifiRttController> getWifiRttController() {
        const std::string& instance_name = GetInstanceName();

        sp<IWifiChip> wifi_chip =
            IWifiChip::castFrom(getWifiChip(instance_name));
        EXPECT_NE(nullptr, wifi_chip.get());

        sp<IWifiStaIface> wifi_sta_iface =
            IWifiStaIface::castFrom(getWifiStaIface(instance_name));
        EXPECT_NE(nullptr, wifi_sta_iface.get());

        const auto& status_and_controller =
            HIDL_INVOKE(wifi_chip, createRttController_1_4, wifi_sta_iface);
        EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_controller.first.code);
        EXPECT_NE(nullptr, status_and_controller.second.get());

        return status_and_controller.second.get();
    }
};

/*
 * registerEventCallback_1_4
 * This test case tests the registerEventCallback_1_4() API which registers
 * a call back function with the hal implementation
 *
 * Note: it is not feasible to test the invocation of the call back function
 * since event is triggered internally in the HAL implementation, and can not be
 * triggered from the test case
 */
TEST_P(WifiRttControllerHidlTest, RegisterEventCallback_1_4) {
    sp<WifiRttControllerEventCallback> wifiRttControllerEventCallback =
        new WifiRttControllerEventCallback();
    const auto& status =
        HIDL_INVOKE(wifi_rtt_controller_, registerEventCallback_1_4,
                    wifiRttControllerEventCallback);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
}

/*
 * rangeRequest_1_4
 */
TEST_P(WifiRttControllerHidlTest, RangeRequest_1_4) {
    std::pair<WifiStatus, RttCapabilities> status_and_caps;

    // Get the Capabilities
    status_and_caps = HIDL_INVOKE(wifi_rtt_controller_, getCapabilities_1_4);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
    // Get the highest support preamble
    int preamble = 1;
    status_and_caps.second.preambleSupport >>= 1;
    while (status_and_caps.second.preambleSupport != 0) {
        status_and_caps.second.preambleSupport >>= 1;
        preamble <<= 1;
    }
    std::vector<RttConfig> configs;
    RttConfig config;
    int cmdId = 55;
    // Set the config with test data
    for (int i = 0; i < 6; i++) {
        config.addr[i] = i;
    }
    config.type = RttType::ONE_SIDED;
    config.peer = RttPeerType::AP;
    config.channel.width = WifiChannelWidthInMhz::WIDTH_80;
    config.channel.centerFreq = 5765;
    config.channel.centerFreq0 = 5775;
    config.channel.centerFreq1 = 0;
    config.bw = RttBw::BW_80MHZ;
    config.preamble = (RttPreamble)preamble;
    config.mustRequestLci = false;
    config.mustRequestLcr = false;
    config.burstPeriod = 0;
    config.numBurst = 0;
    config.numFramesPerBurst = 8;
    config.numRetriesPerRttFrame = 3;
    config.numRetriesPerFtmr = 3;
    config.burstDuration = 9;
    // Insert config in the vector
    configs.push_back(config);

    // Invoke the call
    const auto& status =
        HIDL_INVOKE(wifi_rtt_controller_, rangeRequest_1_4, cmdId, configs);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
}

/*
 * getCapabilities_1_4
 */
TEST_P(WifiRttControllerHidlTest, GetCapabilities_1_4) {
    std::pair<WifiStatus, RttCapabilities> status_and_caps;

    // Invoke the call
    status_and_caps = HIDL_INVOKE(wifi_rtt_controller_, getCapabilities_1_4);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_caps.first.code);
}

/*
 * getResponderInfo_1_4
 */
TEST_P(WifiRttControllerHidlTest, GetResponderInfo_1_4) {
    std::pair<WifiStatus, RttResponder> status_and_info;

    // Invoke the call
    status_and_info = HIDL_INVOKE(wifi_rtt_controller_, getResponderInfo_1_4);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_info.first.code);
}

/*
 * enableResponder_1_4
 */
TEST_P(WifiRttControllerHidlTest, EnableResponder_1_4) {
    std::pair<WifiStatus, RttResponder> status_and_info;
    int cmdId = 55;
    WifiChannelInfo channelInfo;
    channelInfo.width = WifiChannelWidthInMhz::WIDTH_80;
    channelInfo.centerFreq = 5690;
    channelInfo.centerFreq0 = 5690;
    channelInfo.centerFreq1 = 0;

    // Get the responder first
    status_and_info = HIDL_INVOKE(wifi_rtt_controller_, getResponderInfo_1_4);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status_and_info.first.code);

    // Invoke the call
    const auto& status =
        HIDL_INVOKE(wifi_rtt_controller_, enableResponder_1_4, cmdId,
                    channelInfo, 10, status_and_info.second);
    EXPECT_EQ(WifiStatusCode::SUCCESS, status.code);
}

INSTANTIATE_TEST_SUITE_P(
    PerInstance, WifiRttControllerHidlTest,
    testing::ValuesIn(android::hardware::getAllHalInstanceNames(
        ::android::hardware::wifi::V1_4::IWifi::descriptor)),
    android::hardware::PrintInstanceNameToString);
