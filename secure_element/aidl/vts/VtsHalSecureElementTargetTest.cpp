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
#include <android-base/logging.h>
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

#define EXPECT_ERR(status)                                                \
    do {                                                                  \
        auto status_impl = (status);                                      \
        EXPECT_FALSE(status_impl.isOk()) << status_impl.getDescription(); \
    } while (false)

// APDU defined in CTS tests.
// The applet selected with kSelectableAid will return 256 bytes of data
// in response.
static const std::vector<uint8_t> kDataApdu = {
        0x00, 0x08, 0x00, 0x00, 0x00,
};

// Selectable test AID defined in CTS tests.
static const std::vector<uint8_t> kSelectableAid = {
        0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
        0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x31,
};
// Non-selectable test AID defined in CTS tests.
static const std::vector<uint8_t> kNonSelectableAid = {
        0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
        0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0xFF,
};

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
        cv.wait_for(l, 5s, [&]() { return history.size() >= want.size(); });
        EXPECT_THAT(history, ElementsAreArray(want));
    }

    void resetCallbackHistory() {
        std::unique_lock<std::mutex> l(m);
        history.clear();
    }

  private:
    std::mutex m;  // guards history
    std::condition_variable cv;
    std::vector<bool> history;
};

class SecureElementAidl : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        SpAIBinder binder = SpAIBinder(AServiceManager_waitForService(GetParam().c_str()));

        secure_element_ = ISecureElement::fromBinder(binder);
        ASSERT_NE(secure_element_, nullptr);

        secure_element_callback_ = SharedRefBase::make<MySecureElementCallback>();
        ASSERT_NE(secure_element_callback_, nullptr);

        EXPECT_OK(secure_element_->init(secure_element_callback_));
        secure_element_callback_->expectCallbackHistory({true});

        // Check if the basic channel is supported by the bound SE.
        std::vector<uint8_t> basic_channel_response;
        auto status =
                secure_element_->openBasicChannel(kSelectableAid, 0x00, &basic_channel_response);
        if (status.isOk()) {
            basic_channel_supported_ = true;
            secure_element_->closeChannel(0);
        }
    }

    void TearDown() override {
        secure_element_callback_->resetCallbackHistory();
        EXPECT_OK(secure_element_->reset());
        secure_element_callback_->expectCallbackHistory({false, true});
        secure_element_ = nullptr;
        secure_element_callback_ = nullptr;
    }

    // Call transmit with kDataApdu and the selected channel number.
    // Return the response sstatus code.
    uint16_t transmit(uint8_t channel_number) {
        std::vector<uint8_t> apdu = kDataApdu;
        std::vector<uint8_t> response;

        // Edit the channel number into the CLA header byte.
        if (channel_number < 4) {
            apdu[0] |= channel_number;
        } else {
            apdu[0] |= (channel_number - 4) | 0x40;
        }

        // transmit() will return an empty response with the error
        // code CHANNEL_NOT_AVAILABLE when the SE cannot be
        // communicated with.
        auto status = secure_element_->transmit(apdu, &response);
        if (!status.isOk()) {
            return 0x6881;
        }

        // transmit() will return a response containing at least
        // the APDU response status otherwise.
        EXPECT_GE(response.size(), 2u);
        uint16_t apdu_status =
                (response[response.size() - 2] << 8) | (response[response.size() - 1] << 0);

        // When the command is successful the response
        // must contain 256 bytes of data.
        if (apdu_status == 0x9000) {
            EXPECT_EQ(response.size(), 258);
        }

        return apdu_status;
    }

    std::shared_ptr<ISecureElement> secure_element_;
    std::shared_ptr<MySecureElementCallback> secure_element_callback_;
    bool basic_channel_supported_{false};
};

TEST_P(SecureElementAidl, init) {
    // init(nullptr) shall fail.
    EXPECT_ERR(secure_element_->init(nullptr));

    // init with a valid callback pointer shall succeed.
    EXPECT_OK(secure_element_->init(secure_element_callback_));
    secure_element_callback_->expectCallbackHistory({true, true});
}

TEST_P(SecureElementAidl, reset) {
    std::vector<uint8_t> basic_channel_response;
    LogicalChannelResponse logical_channel_response;

    // reset called after init shall succeed.
    if (basic_channel_supported_) {
        EXPECT_OK(secure_element_->openBasicChannel(kSelectableAid, 0x00, &basic_channel_response));
    }
    EXPECT_OK(secure_element_->openLogicalChannel(kSelectableAid, 0x00, &logical_channel_response));

    EXPECT_OK(secure_element_->reset());
    secure_element_callback_->expectCallbackHistory({true, false, true});

    // All opened channels must be closed.
    if (basic_channel_supported_) {
        EXPECT_NE(transmit(0), 0x9000);
    }
    EXPECT_NE(transmit(logical_channel_response.channelNumber), 0x9000);
}

TEST_P(SecureElementAidl, isCardPresent) {
    bool res = false;

    // isCardPresent called after init shall succeed.
    EXPECT_OK(secure_element_->isCardPresent(&res));
    EXPECT_TRUE(res);
}

TEST_P(SecureElementAidl, getAtr) {
    std::vector<uint8_t> atr;

    // getAtr called after init shall succeed.
    // The ATR has size between 0 and 32 bytes.
    EXPECT_OK(secure_element_->getAtr(&atr));
    EXPECT_LE(atr.size(), 32u);
}

TEST_P(SecureElementAidl, openBasicChannel) {
    std::vector<uint8_t> response;

    if (!basic_channel_supported_) {
        return;
    }

    // openBasicChannel called with an invalid AID shall fail.
    EXPECT_ERR(secure_element_->openBasicChannel(kNonSelectableAid, 0x00, &response));

    // openBasicChannel called after init shall succeed.
    // The response size must be larger than 2 bytes as it includes the
    // status code.
    EXPECT_OK(secure_element_->openBasicChannel(kSelectableAid, 0x00, &response));
    EXPECT_GE(response.size(), 2u);

    // transmit called on the basic channel should succeed.
    EXPECT_EQ(transmit(0), 0x9000);

    // openBasicChannel called a second time shall fail.
    // The basic channel can only be opened once.
    EXPECT_ERR(secure_element_->openBasicChannel(kSelectableAid, 0x00, &response));

    // openBasicChannel called after closing the basic channel shall succeed.
    EXPECT_OK(secure_element_->closeChannel(0));
    EXPECT_OK(secure_element_->openBasicChannel(kSelectableAid, 0x00, &response));
}

TEST_P(SecureElementAidl, openLogicalChannel) {
    LogicalChannelResponse response;

    // openLogicalChannel called with an invalid AID shall fail.
    EXPECT_ERR(secure_element_->openLogicalChannel(kNonSelectableAid, 0x00, &response));

    // openLogicalChannel called after init shall succeed.
    // The response size must be larger than 2 bytes as it includes the
    // status code. The channel number must be in the range 1-19.
    EXPECT_OK(secure_element_->openLogicalChannel(kSelectableAid, 0x00, &response));
    EXPECT_GE(response.selectResponse.size(), 2u);
    EXPECT_GE(response.channelNumber, 1u);
    EXPECT_LE(response.channelNumber, 19u);

    // transmit called on the logical channel should succeed.
    EXPECT_EQ(transmit(response.channelNumber), 0x9000);
}

TEST_P(SecureElementAidl, closeChannel) {
    std::vector<uint8_t> basic_channel_response;
    LogicalChannelResponse logical_channel_response;

    // closeChannel called on non-existing basic or logical channel
    // shall fail.
    EXPECT_ERR(secure_element_->closeChannel(0));
    EXPECT_ERR(secure_element_->closeChannel(1));

    // closeChannel called on basic channel closes the basic channel.
    if (basic_channel_supported_) {
        EXPECT_OK(secure_element_->openBasicChannel(kSelectableAid, 0x00, &basic_channel_response));
        EXPECT_OK(secure_element_->closeChannel(0));

        // transmit called on the basic channel should fail.
        EXPECT_NE(transmit(0), 0x9000);
    }

    // closeChannel called on logical channel closes the logical channel.
    EXPECT_OK(secure_element_->openLogicalChannel(kSelectableAid, 0x00, &logical_channel_response));
    EXPECT_OK(secure_element_->closeChannel(logical_channel_response.channelNumber));

    // transmit called on the logical channel should fail.
    EXPECT_NE(transmit(logical_channel_response.channelNumber), 0x9000);
}

TEST_P(SecureElementAidl, transmit) {
    std::vector<uint8_t> response;
    LogicalChannelResponse logical_channel_response;

    /* Temporaly disable this check to clarify Basic Channel behavior (b/300502872)
    // Note: no channel is opened for this test
    // transmit() will return an empty response with the error
    // code CHANNEL_NOT_AVAILABLE when the SE cannot be
    // communicated with.
    EXPECT_ERR(secure_element_->transmit(kDataApdu, &response));
    */

    EXPECT_OK(secure_element_->openLogicalChannel(kSelectableAid, 0x00, &logical_channel_response));
    EXPECT_GE(logical_channel_response.selectResponse.size(), 2u);
    EXPECT_GE(logical_channel_response.channelNumber, 1u);
    EXPECT_LE(logical_channel_response.channelNumber, 19u);

    // transmit called on the logical channel should succeed.
    EXPECT_EQ(transmit(logical_channel_response.channelNumber), 0x9000);
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
