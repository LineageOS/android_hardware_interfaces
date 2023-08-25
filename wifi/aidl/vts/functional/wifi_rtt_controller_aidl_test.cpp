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

#include <vector>

#include <VtsCoreUtil.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/wifi/BnWifi.h>
#include <aidl/android/hardware/wifi/BnWifiRttControllerEventCallback.h>
#include <android/binder_manager.h>
#include <android/binder_status.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "wifi_aidl_test_utils.h"

using aidl::android::hardware::wifi::BnWifiRttControllerEventCallback;
using aidl::android::hardware::wifi::IWifiRttController;
using aidl::android::hardware::wifi::RttBw;
using aidl::android::hardware::wifi::RttCapabilities;
using aidl::android::hardware::wifi::RttConfig;
using aidl::android::hardware::wifi::RttPeerType;
using aidl::android::hardware::wifi::RttPreamble;
using aidl::android::hardware::wifi::RttResponder;
using aidl::android::hardware::wifi::RttResult;
using aidl::android::hardware::wifi::RttType;
using aidl::android::hardware::wifi::WifiChannelInfo;
using aidl::android::hardware::wifi::WifiChannelWidthInMhz;
using aidl::android::hardware::wifi::WifiStatusCode;

class WifiRttControllerAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        if (!::testing::deviceSupportsFeature("android.hardware.wifi.rtt"))
            GTEST_SKIP() << "Skipping this test since RTT is not supported.";
        stopWifiService(getInstanceName());
        wifi_rtt_controller_ = getWifiRttController();
        ASSERT_NE(nullptr, wifi_rtt_controller_.get());

        // Check RTT support before we run the test.
        RttCapabilities caps = {};
        auto status = wifi_rtt_controller_->getCapabilities(&caps);
        if (checkStatusCode(&status, WifiStatusCode::ERROR_NOT_SUPPORTED)) {
            GTEST_SKIP() << "Skipping this test since RTT is not supported.";
        }
    }

    void TearDown() override { stopWifiService(getInstanceName()); }

  protected:
    std::shared_ptr<IWifiRttController> getWifiRttController() {
        std::shared_ptr<IWifiChip> wifi_chip = getWifiChip(getInstanceName());
        EXPECT_NE(nullptr, wifi_chip.get());

        std::shared_ptr<IWifiStaIface> wifi_sta_iface = getWifiStaIface(getInstanceName());
        EXPECT_NE(nullptr, wifi_sta_iface.get());

        std::shared_ptr<IWifiRttController> rtt_controller;
        EXPECT_TRUE(wifi_chip->createRttController(wifi_sta_iface, &rtt_controller).isOk());
        EXPECT_NE(nullptr, rtt_controller.get());
        return rtt_controller;
    }

    RttCapabilities getCapabilities() {
        RttCapabilities caps = {};
        EXPECT_TRUE(wifi_rtt_controller_->getCapabilities(&caps).isOk());
        return caps;
    }

    std::shared_ptr<IWifiRttController> wifi_rtt_controller_;

  private:
    const char* getInstanceName() { return GetParam().c_str(); }
};

class WifiRttControllerEventCallback : public BnWifiRttControllerEventCallback {
  public:
    WifiRttControllerEventCallback() = default;

    ::ndk::ScopedAStatus onResults(int /* cmdId */,
                                   const std::vector<RttResult>& /* results */) override {
        return ndk::ScopedAStatus::ok();
    }
};

/*
 * RegisterEventCallback
 *
 * Note: it is not feasible to test the invocation of the callback function,
 * since events are triggered internally in the HAL implementation and cannot be
 * triggered from the test case.
 */
TEST_P(WifiRttControllerAidlTest, RegisterEventCallback) {
    std::shared_ptr<WifiRttControllerEventCallback> callback =
            ndk::SharedRefBase::make<WifiRttControllerEventCallback>();
    ASSERT_NE(nullptr, callback.get());
    EXPECT_TRUE(wifi_rtt_controller_->registerEventCallback(callback).isOk());
}

/*
 * GetCapabilities
 */
TEST_P(WifiRttControllerAidlTest, GetCapabilities) {
    RttCapabilities caps = {};
    EXPECT_TRUE(wifi_rtt_controller_->getCapabilities(&caps).isOk());
}

/*
 * GetResponderInfo
 */
TEST_P(WifiRttControllerAidlTest, GetResponderInfo) {
    RttCapabilities caps = getCapabilities();
    if (!caps.responderSupported) {
        GTEST_SKIP() << "Skipping because responder is not supported";
    }

    RttResponder responder = {};
    EXPECT_TRUE(wifi_rtt_controller_->getResponderInfo(&responder).isOk());
}

/*
 * EnableResponder
 */
TEST_P(WifiRttControllerAidlTest, EnableResponder) {
    RttCapabilities caps = getCapabilities();
    if (!caps.responderSupported) {
        GTEST_SKIP() << "Skipping because responder is not supported";
    }

    int cmdId = 55;
    WifiChannelInfo channelInfo;
    channelInfo.width = WifiChannelWidthInMhz::WIDTH_80;
    channelInfo.centerFreq = 5660;
    channelInfo.centerFreq0 = 5660;
    channelInfo.centerFreq1 = 0;

    RttResponder responder = {};
    EXPECT_TRUE(wifi_rtt_controller_->getResponderInfo(&responder).isOk());
    EXPECT_TRUE(wifi_rtt_controller_->enableResponder(cmdId, channelInfo, 10, responder).isOk());
}

/*
 * Request2SidedRangeMeasurement
 * Tests the two sided ranging - 802.11mc FTM protocol.
 */
TEST_P(WifiRttControllerAidlTest, Request2SidedRangeMeasurement) {
    RttCapabilities caps = getCapabilities();
    if (!caps.rttFtmSupported) {
        GTEST_SKIP() << "Skipping two sided RTT since driver/fw does not support";
    }

    RttConfig config;
    config.addr = {{0x00, 0x01, 0x02, 0x03, 0x04, 0x05}};
    config.type = RttType::TWO_SIDED;
    config.peer = RttPeerType::AP;
    config.channel.width = WifiChannelWidthInMhz::WIDTH_80;
    config.channel.centerFreq = 5180;
    config.channel.centerFreq0 = 5210;
    config.channel.centerFreq1 = 0;
    config.bw = RttBw::BW_20MHZ;
    config.preamble = RttPreamble::HT;
    config.mustRequestLci = false;
    config.mustRequestLcr = false;
    config.burstPeriod = 0;
    config.numBurst = 0;
    config.numFramesPerBurst = 8;
    config.numRetriesPerRttFrame = 0;
    config.numRetriesPerFtmr = 0;
    config.burstDuration = 9;

    int cmdId = 55;
    std::vector<RttConfig> configs = {config};
    EXPECT_TRUE(wifi_rtt_controller_->rangeRequest(cmdId, configs).isOk());

    // Sleep for 2 seconds to wait for driver/firmware to complete RTT.
    sleep(2);
}

/*
 * RangeRequest
 */
TEST_P(WifiRttControllerAidlTest, RangeRequest) {
    RttCapabilities caps = getCapabilities();
    if (!caps.rttOneSidedSupported) {
        GTEST_SKIP() << "Skipping one sided RTT since driver/fw does not support";
    }

    // Get the highest supported preamble.
    int preamble = 1;
    int caps_preamble_support = static_cast<int>(caps.preambleSupport);
    caps_preamble_support >>= 1;
    while (caps_preamble_support != 0) {
        caps_preamble_support >>= 1;
        preamble <<= 1;
    }

    RttConfig config;
    config.addr = {{0x00, 0x01, 0x02, 0x03, 0x04, 0x05}};
    config.type = RttType::ONE_SIDED;
    config.peer = RttPeerType::AP;
    config.channel.width = WifiChannelWidthInMhz::WIDTH_80;
    config.channel.centerFreq = 5765;
    config.channel.centerFreq0 = 5775;
    config.channel.centerFreq1 = 0;
    config.bw = RttBw::BW_80MHZ;
    config.preamble = static_cast<RttPreamble>(preamble);
    config.mustRequestLci = false;
    config.mustRequestLcr = false;
    config.burstPeriod = 0;
    config.numBurst = 0;
    config.numFramesPerBurst = 8;
    config.numRetriesPerRttFrame = 3;
    config.numRetriesPerFtmr = 3;
    config.burstDuration = 9;

    int cmdId = 55;
    std::vector<RttConfig> configs = {config};
    EXPECT_TRUE(wifi_rtt_controller_->rangeRequest(cmdId, configs).isOk());

    // Sleep for 2 seconds to wait for driver/firmware to complete RTT.
    sleep(2);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(WifiRttControllerAidlTest);
INSTANTIATE_TEST_SUITE_P(WifiTest, WifiRttControllerAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IWifi::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
    android::ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
