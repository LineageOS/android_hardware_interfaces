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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/automotive/can/BusConfig.h>
#include <aidl/android/hardware/automotive/can/ICanController.h>
#include <aidl/android/hardware/automotive/can/Result.h>
#include <aidl/android/hardware/automotive/can/VirtualInterface.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <gtest/gtest.h>
#include <libnetdevice/libnetdevice.h>
#include <libnl++/MessageFactory.h>
#include <libnl++/Socket.h>
#include <libnl++/printer.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <chrono>
#include <thread>

using aidl::android::hardware::automotive::can::BusConfig;
using aidl::android::hardware::automotive::can::ICanController;
using aidl::android::hardware::automotive::can::VirtualInterface;
using namespace std::chrono_literals;
using namespace std::string_literals;

class CanControllerAidlTest : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        android::base::SetDefaultTag("CAN_HAL_VTS");
        android::base::SetMinimumLogSeverity(android::base::VERBOSE);
        const auto instance = ICanController::descriptor + "/default"s;
        mCanControllerService = ICanController::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(instance.c_str())));

        ASSERT_NE(mCanControllerService, nullptr);
    }
    virtual void TearDown() override {}

    static bool mTestCaseInitialized;
    std::shared_ptr<ICanController> mCanControllerService;
};

// we can't test a real bus, since we can't make any assumptions about hardware
// this checks upBus, getInterfaceName, and downBus
TEST_P(CanControllerAidlTest, ToggleBus) {
    const std::string_view canIface = "vcan50";
    const std::string busName = "VTS_CAN";

    std::string upBusReturn;  // should be vcan50
    BusConfig config = {};
    VirtualInterface iface = {};
    iface.ifname = canIface;
    config.interfaceId.set<BusConfig::InterfaceId::Tag::virtualif>(iface);
    config.name = busName;
    auto aidlStatus = mCanControllerService->upBus(config, &upBusReturn);
    ASSERT_TRUE(aidlStatus.isOk());
    EXPECT_EQ(upBusReturn, canIface);

    std::string ifaceName;
    aidlStatus = mCanControllerService->getInterfaceName(busName, &ifaceName);
    ASSERT_TRUE(aidlStatus.isOk());
    EXPECT_EQ(ifaceName, canIface);

    aidlStatus = mCanControllerService->downBus(busName);
    ASSERT_TRUE(aidlStatus.isOk());
}

TEST_P(CanControllerAidlTest, GetSupported) {
    LOG(VERBOSE) << "Get the supported iface types";
    std::vector<::aidl::android::hardware::automotive::can::InterfaceType> supportedTypes;
    auto aidlStatus = mCanControllerService->getSupportedInterfaceTypes(&supportedTypes);
    ASSERT_TRUE(aidlStatus.isOk());
    EXPECT_FALSE(supportedTypes.empty());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(CanControllerAidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, CanControllerAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(ICanController::descriptor)),
        android::PrintInstanceNameToString);
