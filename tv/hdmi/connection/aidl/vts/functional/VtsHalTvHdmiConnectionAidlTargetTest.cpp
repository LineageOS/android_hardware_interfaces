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

#define LOG_TAG "Hdmi_Connection_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/tv/hdmi/connection/BnHdmiConnection.h>
#include <aidl/android/hardware/tv/hdmi/connection/BnHdmiConnectionCallback.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>
#include <log/log.h>
#include <sstream>
#include <vector>

using ::aidl::android::hardware::tv::hdmi::connection::BnHdmiConnectionCallback;
using ::aidl::android::hardware::tv::hdmi::connection::HdmiPortInfo;
using ::aidl::android::hardware::tv::hdmi::connection::HdmiPortType;
using ::aidl::android::hardware::tv::hdmi::connection::HpdSignal;
using ::aidl::android::hardware::tv::hdmi::connection::IHdmiConnection;
using ::aidl::android::hardware::tv::hdmi::connection::IHdmiConnectionCallback;
using ::ndk::SpAIBinder;

#define INCORRECT_VENDOR_ID 0x00
#define TV_PHYSICAL_ADDRESS 0x0000

// The main test class for TV HDMI Connection HAL.
class HdmiConnectionTest : public ::testing::TestWithParam<std::string> {
    static void serviceDied(void* /* cookie */) {
        ALOGE("VtsHalTvHdmiConnectionAidlTargetTest died");
    }

  public:
    void SetUp() override {
        hdmiConnection = IHdmiConnection::fromBinder(
                SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(hdmiConnection, nullptr);
        ALOGI("%s: getService() for hdmiConnection is %s", __func__,
              hdmiConnection->isRemote() ? "remote" : "local");

        hdmiConnectionCallback = ::ndk::SharedRefBase::make<HdmiConnectionCallback>();
        ASSERT_NE(hdmiConnectionCallback, nullptr);
        hdmiConnectionDeathRecipient =
                ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(&serviceDied));
        ASSERT_EQ(AIBinder_linkToDeath(hdmiConnection->asBinder().get(),
                                       hdmiConnectionDeathRecipient.get(), 0),
                  STATUS_OK);
    }

    class HdmiConnectionCallback : public BnHdmiConnectionCallback {
      public:
        ::ndk::ScopedAStatus onHotplugEvent(bool connected __unused, int32_t portId __unused) {
            return ::ndk::ScopedAStatus::ok();
        };
    };

    std::shared_ptr<IHdmiConnection> hdmiConnection;
    std::shared_ptr<IHdmiConnectionCallback> hdmiConnectionCallback;
    ::ndk::ScopedAIBinder_DeathRecipient hdmiConnectionDeathRecipient;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HdmiConnectionTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, HdmiConnectionTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IHdmiConnection::descriptor)),
        android::PrintInstanceNameToString);

TEST_P(HdmiConnectionTest, SetCallback) {
    ASSERT_TRUE(hdmiConnection->setCallback(::ndk::SharedRefBase::make<HdmiConnectionCallback>())
                        .isOk());
}

TEST_P(HdmiConnectionTest, GetPortInfo) {
    std::vector<HdmiPortInfo> ports;
    ASSERT_TRUE(hdmiConnection->getPortInfo(&ports).isOk());

    bool cecSupportedOnDevice = false;
    for (size_t i = 0; i < ports.size(); ++i) {
        EXPECT_TRUE((ports[i].type == HdmiPortType::OUTPUT) ||
                    (ports[i].type == HdmiPortType::INPUT));
        if (ports[i].type == HdmiPortType::OUTPUT && ports[i].portId <= 0) {
            ALOGW("%s: Port id for output ports should start from 1", __func__);
        }
        cecSupportedOnDevice = cecSupportedOnDevice | ports[i].cecSupported;
    }
    EXPECT_NE(cecSupportedOnDevice, false) << "At least one port should support CEC";
}

TEST_P(HdmiConnectionTest, IsConnected) {
    std::vector<HdmiPortInfo> ports;
    ASSERT_TRUE(hdmiConnection->getPortInfo(&ports).isOk());
    for (size_t i = 0; i < ports.size(); ++i) {
        bool connected;
        ASSERT_TRUE(hdmiConnection->isConnected(ports[i].portId, &connected).isOk());
    }
}

TEST_P(HdmiConnectionTest, HdpSignal) {
    std::vector<HdmiPortInfo> ports;
    ASSERT_TRUE(hdmiConnection->getPortInfo(&ports).isOk());
    HpdSignal originalSignal;
    HpdSignal signal = HpdSignal::HDMI_HPD_STATUS_BIT;
    for (size_t i = 0; i < ports.size(); ++i) {
        int32_t portId = ports[i].portId;
        HpdSignal readSignal;
        ASSERT_TRUE(hdmiConnection->getHpdSignal(portId, &originalSignal).isOk());
        ASSERT_TRUE(hdmiConnection->setHpdSignal(signal, portId).isOk());
        ASSERT_TRUE(hdmiConnection->getHpdSignal(portId, &readSignal).isOk());
        EXPECT_EQ(readSignal, signal);
        signal = HpdSignal::HDMI_HPD_PHYSICAL;
        ASSERT_TRUE(hdmiConnection->setHpdSignal(signal, portId).isOk());
        ASSERT_TRUE(hdmiConnection->getHpdSignal(portId, &readSignal).isOk());
        EXPECT_EQ(readSignal, signal);
        ASSERT_TRUE(hdmiConnection->setHpdSignal(originalSignal, portId).isOk());
    }
}
