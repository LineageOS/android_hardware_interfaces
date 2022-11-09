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

#define LOG_TAG "Hdmi_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/tv/hdmi/BnHdmi.h>
#include <aidl/android/hardware/tv/hdmi/BnHdmiCallback.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>
#include <log/log.h>
#include <sstream>
#include <vector>

using ::aidl::android::hardware::tv::hdmi::BnHdmiCallback;
using ::aidl::android::hardware::tv::hdmi::HdmiPortInfo;
using ::aidl::android::hardware::tv::hdmi::HdmiPortType;
using ::aidl::android::hardware::tv::hdmi::HpdSignal;
using ::aidl::android::hardware::tv::hdmi::IHdmi;
using ::aidl::android::hardware::tv::hdmi::IHdmiCallback;
using ::ndk::SpAIBinder;

#define INCORRECT_VENDOR_ID 0x00
#define TV_PHYSICAL_ADDRESS 0x0000

// The main test class for TV HDMI HAL.
class HdmiTest : public ::testing::TestWithParam<std::string> {
    static void serviceDied(void* /* cookie */) { ALOGE("VtsHalTvCecAidlTargetTest died"); }

  public:
    void SetUp() override {
        hdmi = IHdmi::fromBinder(SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(hdmi, nullptr);
        ALOGI("%s: getService() for hdmi is %s", __func__, hdmi->isRemote() ? "remote" : "local");

        hdmiCallback = ::ndk::SharedRefBase::make<HdmiCallback>();
        ASSERT_NE(hdmiCallback, nullptr);
        hdmiDeathRecipient =
                ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(&serviceDied));
        ASSERT_EQ(AIBinder_linkToDeath(hdmi->asBinder().get(), hdmiDeathRecipient.get(), 0),
                  STATUS_OK);
    }

    class HdmiCallback : public BnHdmiCallback {
      public:
        ::ndk::ScopedAStatus onHotplugEvent(bool connected __unused, int32_t portId __unused) {
            return ::ndk::ScopedAStatus::ok();
        };
    };

    std::shared_ptr<IHdmi> hdmi;
    std::shared_ptr<IHdmiCallback> hdmiCallback;
    ::ndk::ScopedAIBinder_DeathRecipient hdmiDeathRecipient;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HdmiTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, HdmiTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IHdmi::descriptor)),
                         android::PrintInstanceNameToString);

TEST_P(HdmiTest, SetCallback) {
    ASSERT_TRUE(hdmi->setCallback(::ndk::SharedRefBase::make<HdmiCallback>()).isOk());
}

TEST_P(HdmiTest, GetPortInfo) {
    std::vector<HdmiPortInfo> ports;
    ASSERT_TRUE(hdmi->getPortInfo(&ports).isOk());

    bool cecSupportedOnDevice = false;
    for (size_t i = 0; i < ports.size(); ++i) {
        EXPECT_TRUE((ports[i].type == HdmiPortType::OUTPUT) ||
                    (ports[i].type == HdmiPortType::INPUT));
        if (ports[i].portId == 0) {
            ALOGW("%s: Port id should start from 1", __func__);
        }
        cecSupportedOnDevice = cecSupportedOnDevice | ports[i].cecSupported;
    }
    EXPECT_NE(cecSupportedOnDevice, false) << "At least one port should support CEC";
}

TEST_P(HdmiTest, IsConnected) {
    std::vector<HdmiPortInfo> ports;
    ASSERT_TRUE(hdmi->getPortInfo(&ports).isOk());
    for (size_t i = 0; i < ports.size(); ++i) {
        bool connected;
        ASSERT_TRUE(hdmi->isConnected(ports[i].portId, &connected).isOk());
    }
}

TEST_P(HdmiTest, HdpSignal) {
    HpdSignal originalSignal;
    HpdSignal signal = HpdSignal::HDMI_HPD_STATUS_BIT;
    HpdSignal readSignal;
    ASSERT_TRUE(hdmi->getHpdSignal(&originalSignal).isOk());
    ASSERT_TRUE(hdmi->setHpdSignal(signal).isOk());
    ASSERT_TRUE(hdmi->getHpdSignal(&readSignal).isOk());
    EXPECT_EQ(readSignal, signal);
    signal = HpdSignal::HDMI_HPD_PHYSICAL;
    ASSERT_TRUE(hdmi->setHpdSignal(signal).isOk());
    ASSERT_TRUE(hdmi->getHpdSignal(&readSignal).isOk());
    EXPECT_EQ(readSignal, signal);
    ASSERT_TRUE(hdmi->setHpdSignal(originalSignal).isOk());
}
