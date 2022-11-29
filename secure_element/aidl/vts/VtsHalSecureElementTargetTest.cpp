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
#include <aidl/android/hardware/secure_element/BnSecureElementCallback.h>
#include <aidl/android/hardware/secure_element/ISecureElement.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <mutex>

using namespace std::chrono_literals;

using aidl::android::hardware::secure_element::BnSecureElementCallback;
using aidl::android::hardware::secure_element::ISecureElement;
using aidl::android::hardware::secure_element::LogicalChannelResponse;
using ndk::ScopedAStatus;
using ndk::SharedRefBase;
using ndk::SpAIBinder;
using testing::ElementsAre;
using testing::ElementsAreArray;

#define EXPECT_OK(status)                                                \
    do {                                                                 \
        auto status_impl = (status);                                     \
        EXPECT_TRUE(status_impl.isOk()) << status_impl.getDescription(); \
    } while (false)

static const std::vector<uint8_t> kDataApdu = {0x00, 0x08, 0x00, 0x00, 0x00};
static const std::vector<uint8_t> kAndroidTestAid = {0xA0, 0x00, 0x00, 0x04, 0x76, 0x41,
                                                     0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64,
                                                     0x43, 0x54, 0x53, 0x31};

class MySecureElementCallback : public BnSecureElementCallback {
  public:
    ScopedAStatus onStateChange(bool state, const std::string& debugReason) override {
        {
            std::unique_lock<std::mutex> l(m);
            (void)debugReason;
            history.push_back(state);
        }
        cv.notify_one();
        return ScopedAStatus::ok();
    };

    void expectCallbackHistory(std::vector<bool>&& want) {
        std::unique_lock<std::mutex> l(m);
        cv.wait_for(l, 2s, [&]() { return history.size() >= want.size(); });
        EXPECT_THAT(history, ElementsAreArray(want));
    }

  private:
    std::mutex m;  // guards history
    std::condition_variable cv;
    std::vector<bool> history;
};

class SecureElementAidl : public ::testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        SpAIBinder binder = SpAIBinder(AServiceManager_waitForService(GetParam().c_str()));
        se = ISecureElement::fromBinder(binder);
        ASSERT_NE(se, nullptr);

        cb = SharedRefBase::make<MySecureElementCallback>();
        EXPECT_OK(se->init(cb));

        cb->expectCallbackHistory({true});
    }

    std::shared_ptr<ISecureElement> se;
    std::shared_ptr<MySecureElementCallback> cb;
};

TEST_P(SecureElementAidl, isCardPresent) {
    bool res = false;
    EXPECT_OK(se->isCardPresent(&res));
    EXPECT_TRUE(res);
}

TEST_P(SecureElementAidl, transmit) {
    LogicalChannelResponse response;
    EXPECT_OK(se->openLogicalChannel(kAndroidTestAid, 0x00, &response));

    EXPECT_GE(response.selectResponse.size(), 2u);
    EXPECT_GE(response.channelNumber, 1);

    std::vector<uint8_t> command = kDataApdu;
    command[0] |= response.channelNumber;

    std::vector<uint8_t> transmitResponse;
    EXPECT_OK(se->transmit(command, &transmitResponse));

    EXPECT_LE(transmitResponse.size(), 3);
    EXPECT_GE(transmitResponse.size(), 2);
    EXPECT_EQ(transmitResponse[transmitResponse.size() - 1], 0x00);
    EXPECT_EQ(transmitResponse[transmitResponse.size() - 2], 0x90);

    EXPECT_OK(se->closeChannel(response.channelNumber));
}

TEST_P(SecureElementAidl, openBasicChannel) {
    std::vector<uint8_t> response;
    auto status = se->openBasicChannel(kAndroidTestAid, 0x00, &response);

    if (!status.isOk()) {
        EXPECT_EQ(status.getServiceSpecificError(), ISecureElement::CHANNEL_NOT_AVAILABLE)
                << status.getDescription();
        return;
    }

    EXPECT_GE(response.size(), 2u);
    EXPECT_OK(se->closeChannel(0));
}

TEST_P(SecureElementAidl, getAtr) {
    std::vector<uint8_t> atr;
    EXPECT_OK(se->getAtr(&atr));
    if (atr.size() == 0) {
        return;
    }
    EXPECT_LE(atr.size(), 32u);
    EXPECT_GE(atr.size(), 1u);
}

TEST_P(SecureElementAidl, openCloseLogicalChannel) {
    LogicalChannelResponse response;
    EXPECT_OK(se->openLogicalChannel(kAndroidTestAid, 0x00, &response));
    EXPECT_GE(response.selectResponse.size(), 2u);
    EXPECT_GE(response.channelNumber, 1);
    EXPECT_OK(se->closeChannel(response.channelNumber));
}

TEST_P(SecureElementAidl, openInvalidAid) {
    LogicalChannelResponse response;
    auto status = se->openLogicalChannel({0x42}, 0x00, &response);
    EXPECT_EQ(status.getServiceSpecificError(), ISecureElement::NO_SUCH_ELEMENT_ERROR)
            << status.getDescription();
}

TEST_P(SecureElementAidl, Reset) {
    cb->expectCallbackHistory({true});
    EXPECT_OK(se->reset());
    cb->expectCallbackHistory({true, false, true});
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SecureElementAidl);
INSTANTIATE_TEST_SUITE_P(
        SecureElement, SecureElementAidl,
        testing::ValuesIn(android::getAidlHalInstanceNames(ISecureElement::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
