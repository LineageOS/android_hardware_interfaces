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

#define LOG_TAG "HdmiCec_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/tv/hdmi/cec/BnHdmiCec.h>
#include <aidl/android/hardware/tv/hdmi/cec/BnHdmiCecCallback.h>
#include <aidl/android/hardware/tv/hdmi/cec/CecDeviceType.h>
#include <aidl/android/hardware/tv/hdmi/connection/BnHdmiConnection.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>
#include <log/log.h>
#include <sstream>
#include <vector>

using ::aidl::android::hardware::tv::hdmi::cec::BnHdmiCecCallback;
using ::aidl::android::hardware::tv::hdmi::cec::CecDeviceType;
using ::aidl::android::hardware::tv::hdmi::cec::CecLogicalAddress;
using ::aidl::android::hardware::tv::hdmi::cec::CecMessage;
using ::aidl::android::hardware::tv::hdmi::cec::IHdmiCec;
using ::aidl::android::hardware::tv::hdmi::cec::IHdmiCecCallback;
using ::aidl::android::hardware::tv::hdmi::cec::Result;
using ::aidl::android::hardware::tv::hdmi::cec::SendMessageResult;
using ::aidl::android::hardware::tv::hdmi::connection::HdmiPortInfo;
using ::ndk::SpAIBinder;

#define CEC_VERSION 0x05
#define INCORRECT_VENDOR_ID 0x00
#define TV_PHYSICAL_ADDRESS 0x0000

// The main test class for TV CEC HAL.
class HdmiCecTest : public ::testing::TestWithParam<std::string> {
    static void serviceDied(void* /* cookie */) { ALOGE("VtsHalTvCecAidlTargetTest died"); }

  public:
    void SetUp() override {
        hdmiCec = IHdmiCec::fromBinder(
                SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(hdmiCec, nullptr);
        ALOGI("%s: getService() for hdmiCec is %s", __func__,
              hdmiCec->isRemote() ? "remote" : "local");

        hdmiCecCallback = ::ndk::SharedRefBase::make<CecCallback>();
        ASSERT_NE(hdmiCecCallback, nullptr);
        hdmiCecDeathRecipient =
                ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(&serviceDied));
        ASSERT_EQ(AIBinder_linkToDeath(hdmiCec->asBinder().get(), hdmiCecDeathRecipient.get(), 0),
                  STATUS_OK);
    }

    std::vector<int> getDeviceTypes() {
        std::vector<int> deviceTypes;
        FILE* p = popen("getprop ro.hdmi.device_type", "re");
        if (p) {
            char* line = NULL;
            size_t len = 0;
            if (getline(&line, &len, p) > 0) {
                std::istringstream stream(line);
                std::string number{};
                while (std::getline(stream, number, ',')) {
                    deviceTypes.push_back(stoi(number));
                }
            }
            pclose(p);
        }
        return deviceTypes;
    }

    bool hasDeviceType(CecDeviceType type) {
        std::vector<int> deviceTypes = getDeviceTypes();
        return std::find(deviceTypes.begin(), deviceTypes.end(), (int)type) != deviceTypes.end();
    }

    class CecCallback : public BnHdmiCecCallback {
      public:
        ::ndk::ScopedAStatus onCecMessage(const CecMessage& message __unused) {
            return ::ndk::ScopedAStatus::ok();
        };
    };

    std::shared_ptr<IHdmiCec> hdmiCec;
    std::shared_ptr<IHdmiCecCallback> hdmiCecCallback;
    ::ndk::ScopedAIBinder_DeathRecipient hdmiCecDeathRecipient;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HdmiCecTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, HdmiCecTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IHdmiCec::descriptor)),
                         android::PrintInstanceNameToString);

TEST_P(HdmiCecTest, ClearAddLogicalAddress) {
    Result addLaResult;
    ASSERT_TRUE(hdmiCec->clearLogicalAddress().isOk());
    ASSERT_TRUE(hdmiCec->addLogicalAddress(CecLogicalAddress::PLAYBACK_3, &addLaResult).isOk());
    EXPECT_EQ(addLaResult, Result::SUCCESS);
}

TEST_P(HdmiCecTest, PhysicalAddress) {
    int32_t addr;
    ASSERT_TRUE(hdmiCec->getPhysicalAddress(&addr).isOk());
    if (!hasDeviceType(CecDeviceType::TV)) {
        EXPECT_NE(addr, TV_PHYSICAL_ADDRESS);
    }
}

TEST_P(HdmiCecTest, SendMessage) {
    CecMessage message;
    message.initiator = CecLogicalAddress::PLAYBACK_1;
    message.destination = CecLogicalAddress::BROADCAST;
    message.body.resize(1);
    message.body[0] = 131;
    SendMessageResult result;
    ASSERT_TRUE(hdmiCec->sendMessage(message, &result).isOk());
    EXPECT_EQ(result, SendMessageResult::SUCCESS);
}

TEST_P(HdmiCecTest, CecVersion) {
    int32_t version;
    ASSERT_TRUE(hdmiCec->getCecVersion(&version).isOk());
    EXPECT_GE(version, CEC_VERSION);
}

TEST_P(HdmiCecTest, SetCallback) {
    ASSERT_TRUE(hdmiCec->setCallback(::ndk::SharedRefBase::make<CecCallback>()).isOk());
}

TEST_P(HdmiCecTest, VendorId) {
    int32_t vendorId;
    ASSERT_TRUE(hdmiCec->getVendorId(&vendorId).isOk());
    EXPECT_NE(vendorId, INCORRECT_VENDOR_ID);
}

TEST_P(HdmiCecTest, EnableWakeupByOtp) {
    ASSERT_TRUE(hdmiCec->enableWakeupByOtp(false).isOk());
    // Restore option to its default value
    ASSERT_TRUE(hdmiCec->enableWakeupByOtp(true).isOk());
}

TEST_P(HdmiCecTest, EnableCec) {
    ASSERT_TRUE(hdmiCec->enableCec(false).isOk());
    // Restore option to its default value
    ASSERT_TRUE(hdmiCec->enableCec(true).isOk());
}

TEST_P(HdmiCecTest, EnableSystemCecControl) {
    ASSERT_TRUE(hdmiCec->enableSystemCecControl(true).isOk());
    // Restore option to its default value
    ASSERT_TRUE(hdmiCec->enableSystemCecControl(false).isOk());
}

TEST_P(HdmiCecTest, SetLanguage) {
    ASSERT_TRUE(hdmiCec->setLanguage("eng").isOk());
}
