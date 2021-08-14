/*
 * Copyright (C) 2021 The Android Open Source Project
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
#include <android-base/logging.h>

#include <android/hardware/tv/cec/1.1/IHdmiCec.h>
#include <android/hardware/tv/cec/1.1/types.h>
#include <utils/Log.h>
#include <sstream>
#include <vector>

#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using ::android::sp;
using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::tv::cec::V1_0::CecDeviceType;
using ::android::hardware::tv::cec::V1_0::HdmiPortInfo;
using ::android::hardware::tv::cec::V1_0::HdmiPortType;
using ::android::hardware::tv::cec::V1_0::HotplugEvent;
using ::android::hardware::tv::cec::V1_0::OptionKey;
using ::android::hardware::tv::cec::V1_0::Result;
using ::android::hardware::tv::cec::V1_0::SendMessageResult;
using ::android::hardware::tv::cec::V1_1::CecLogicalAddress;
using ::android::hardware::tv::cec::V1_1::CecMessage;
using ::android::hardware::tv::cec::V1_1::IHdmiCec;
using ::android::hardware::tv::cec::V1_1::IHdmiCecCallback;

#define CEC_VERSION 0x05
#define INCORRECT_VENDOR_ID 0x00
#define TV_PHYSICAL_ADDRESS 0x0000

// The main test class for TV CEC HAL.
class HdmiCecTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        hdmiCec = IHdmiCec::getService(GetParam());
        ASSERT_NE(hdmiCec, nullptr);
        ALOGI("%s: getService() for hdmiCec is %s", __func__,
              hdmiCec->isRemote() ? "remote" : "local");

        hdmiCec_death_recipient = new HdmiCecDeathRecipient();
        hdmiCecCallback = new CecCallback();
        ASSERT_NE(hdmiCec_death_recipient, nullptr);
        ASSERT_TRUE(hdmiCec->linkToDeath(hdmiCec_death_recipient, 0).isOk());
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

    class CecCallback : public IHdmiCecCallback {
      public:
        Return<void> onCecMessage(
                const ::android::hardware::tv::cec::V1_0::CecMessage& /* message */) {
            return Void();
        }
        Return<void> onCecMessage_1_1(
                const ::android::hardware::tv::cec::V1_1::CecMessage& /* message */) {
            return Void();
        }
        Return<void> onHotplugEvent(
                const ::android::hardware::tv::cec::V1_0::HotplugEvent& /* event */) {
            return Void();
        }
    };

    class HdmiCecDeathRecipient : public hidl_death_recipient {
      public:
        void serviceDied(uint64_t /*cookie*/,
                         const android::wp<::android::hidl::base::V1_0::IBase>& /*who*/) override {
            FAIL();
        }
    };

    sp<IHdmiCec> hdmiCec;
    sp<IHdmiCecCallback> hdmiCecCallback;
    sp<HdmiCecDeathRecipient> hdmiCec_death_recipient;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HdmiCecTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, HdmiCecTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IHdmiCec::descriptor)),
        android::hardware::PrintInstanceNameToString);

TEST_P(HdmiCecTest, ClearAddLogicalAddress) {
    hdmiCec->clearLogicalAddress();
    Return<Result> ret = hdmiCec->addLogicalAddress_1_1(CecLogicalAddress::PLAYBACK_3);
    EXPECT_EQ(ret, Result::SUCCESS);
}

TEST_P(HdmiCecTest, PhysicalAddress) {
    Result result;
    uint16_t addr;
    Return<void> ret = hdmiCec->getPhysicalAddress([&result, &addr](Result res, uint16_t paddr) {
        result = res;
        addr = paddr;
    });
    EXPECT_EQ(result, Result::SUCCESS);
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
    SendMessageResult ret = hdmiCec->sendMessage_1_1(message);
    EXPECT_EQ(ret, SendMessageResult::SUCCESS);
}

TEST_P(HdmiCecTest, CecVersion) {
    Return<int32_t> ret = hdmiCec->getCecVersion();
    EXPECT_GE(ret, CEC_VERSION);
}

TEST_P(HdmiCecTest, SetCallback) {
    Return<void> ret = hdmiCec->setCallback_1_1(new CecCallback());
    ASSERT_TRUE(ret.isOk());
}

TEST_P(HdmiCecTest, VendorId) {
    Return<uint32_t> ret = hdmiCec->getVendorId();
    EXPECT_NE(ret, INCORRECT_VENDOR_ID);
}

TEST_P(HdmiCecTest, GetPortInfo) {
    hidl_vec<HdmiPortInfo> ports;
    Return<void> ret =
            hdmiCec->getPortInfo([&ports](hidl_vec<HdmiPortInfo> list) { ports = list; });
    ASSERT_TRUE(ret.isOk());
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

TEST_P(HdmiCecTest, SetOption) {
    Return<void> wakeup = hdmiCec->setOption(OptionKey::WAKEUP, false);
    ASSERT_TRUE(wakeup.isOk());
    Return<void> enableCec = hdmiCec->setOption(OptionKey::ENABLE_CEC, false);
    ASSERT_TRUE(enableCec.isOk());
    Return<void> systemCecControl = hdmiCec->setOption(OptionKey::SYSTEM_CEC_CONTROL, true);
    ASSERT_TRUE(systemCecControl.isOk());
    // Restore option keys to their default values
    hdmiCec->setOption(OptionKey::WAKEUP, true);
    hdmiCec->setOption(OptionKey::ENABLE_CEC, true);
    hdmiCec->setOption(OptionKey::SYSTEM_CEC_CONTROL, false);
}

TEST_P(HdmiCecTest, SetLanguage) {
    Return<void> ret = hdmiCec->setLanguage("eng");
    ASSERT_TRUE(ret.isOk());
}

TEST_P(HdmiCecTest, EnableAudioReturnChannel) {
    hidl_vec<HdmiPortInfo> ports;
    Return<void> ret =
            hdmiCec->getPortInfo([&ports](hidl_vec<HdmiPortInfo> list) { ports = list; });
    for (size_t i = 0; i < ports.size(); ++i) {
        if (ports[i].arcSupported) {
            Return<void> ret = hdmiCec->enableAudioReturnChannel(ports[i].portId, true);
            ASSERT_TRUE(ret.isOk());
        }
    }
}

TEST_P(HdmiCecTest, IsConnected) {
    hidl_vec<HdmiPortInfo> ports;
    Return<void> ret =
            hdmiCec->getPortInfo([&ports](hidl_vec<HdmiPortInfo> list) { ports = list; });
    for (size_t i = 0; i < ports.size(); ++i) {
        Return<bool> ret = hdmiCec->isConnected(ports[i].portId);
        EXPECT_TRUE(ret.isOk());
    }
}
