/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "secure_element_hidl_hal_test"
#include <android-base/logging.h>

#include <android/hardware/secure_element/1.0/ISecureElement.h>
#include <android/hardware/secure_element/1.0/ISecureElementHalCallback.h>
#include <android/hardware/secure_element/1.0/types.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::hardware::secure_element::V1_0::ISecureElement;
using ::android::hardware::secure_element::V1_0::ISecureElementHalCallback;
using ::android::hardware::secure_element::V1_0::SecureElementStatus;
using ::android::hardware::secure_element::V1_0::LogicalChannelResponse;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::testing::VtsHalHidlTargetTestEnvBase;

#define DATA_APDU \
    { 0x00, 0x08, 0x00, 0x00, 0x00 }
#define ANDROID_TEST_AID                                                                          \
    {                                                                                             \
        0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, \
            0x31                                                                                  \
    }

constexpr char kCallbackNameOnStateChange[] = "onStateChange";

class SecureElementCallbackArgs {
   public:
    bool state_;
};

class SecureElementHalCallback
    : public ::testing::VtsHalHidlTargetCallbackBase<SecureElementCallbackArgs>,
      public ISecureElementHalCallback {
   public:
    virtual ~SecureElementHalCallback() = default;

    Return<void> onStateChange(bool state) override {
        SecureElementCallbackArgs args;
        args.state_ = state;
        NotifyFromCallback(kCallbackNameOnStateChange, args);
        return Void();
    };
};

class SecureElementHidlEnvironment : public VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static SecureElementHidlEnvironment* Instance() {
        static SecureElementHidlEnvironment* instance = new SecureElementHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<ISecureElement>(); }

   private:
    SecureElementHidlEnvironment() {}

    GTEST_DISALLOW_COPY_AND_ASSIGN_(SecureElementHidlEnvironment);
};

class SecureElementHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        std::string serviceName =
            SecureElementHidlEnvironment::Instance()->getServiceName<ISecureElement>("eSE1");
        LOG(INFO) << "get service with name:" << serviceName;
        ASSERT_FALSE(serviceName.empty());
        se_ = ::testing::VtsHalHidlTargetTestBase::getService<ISecureElement>(serviceName);
        ASSERT_NE(se_, nullptr);

        se_cb_ = new SecureElementHalCallback();
        ASSERT_NE(se_cb_, nullptr);
        se_->init(se_cb_);
        auto res = se_cb_->WaitForCallback(kCallbackNameOnStateChange);
        EXPECT_TRUE(res.no_timeout);
        EXPECT_TRUE(res.args->state_);
    }

    sp<ISecureElement> se_;
    sp<SecureElementHalCallback> se_cb_;
};

/*
 * isCardPresent:
 * Expects the card to be present
 */
TEST_F(SecureElementHidlTest, isCardPresent) {
    EXPECT_TRUE(se_->isCardPresent());
}

/*
 * transmit:
 * Check status word in the response
 */
TEST_F(SecureElementHidlTest, transmit) {
    std::vector<uint8_t> aid = ANDROID_TEST_AID;
    SecureElementStatus statusReturned;
    LogicalChannelResponse response;
    se_->openLogicalChannel(
        aid, 0x00,
        [&statusReturned, &response](LogicalChannelResponse channelResponse,
                                     SecureElementStatus status) {
            statusReturned = status;
            if (status == SecureElementStatus::SUCCESS) {
                response.channelNumber = channelResponse.channelNumber;
                response.selectResponse.resize(channelResponse.selectResponse.size());
                for (size_t i = 0; i < channelResponse.selectResponse.size(); i++) {
                    response.selectResponse[i] = channelResponse.selectResponse[i];
                }
            }
        });
    EXPECT_EQ(SecureElementStatus::SUCCESS, statusReturned);
    EXPECT_LE((unsigned int)2, response.selectResponse.size());
    EXPECT_LE(1, response.channelNumber);
    std::vector<uint8_t> command = DATA_APDU;
    command[0] |= response.channelNumber;
    std::vector<uint8_t> transmitResponse;
    se_->transmit(command, [&transmitResponse](std::vector<uint8_t> res) {
        transmitResponse.resize(res.size());
        for (size_t i = 0; i < res.size(); i++) {
            transmitResponse[i] = res[i];
        }
    });
    EXPECT_LE((unsigned int)3, transmitResponse.size());
    EXPECT_EQ(0x90, transmitResponse[transmitResponse.size() - 2]);
    EXPECT_EQ(0x00, transmitResponse[transmitResponse.size() - 1]);
    EXPECT_EQ(SecureElementStatus::SUCCESS, se_->closeChannel(response.channelNumber));
}

/*
 * OpenCloseBasicChannel:
 * If the secure element allows opening of basic channel:
 *  open channel, check the length of selectResponse and close the channel
 */
TEST_F(SecureElementHidlTest, openBasicChannel) {
    std::vector<uint8_t> aid = ANDROID_TEST_AID;
    SecureElementStatus statusReturned;
    std::vector<uint8_t> response;
    se_->openBasicChannel(aid, 0x00,
                          [&statusReturned, &response](std::vector<uint8_t> selectResponse,
                                                       SecureElementStatus status) {
                              statusReturned = status;
                              if (status == SecureElementStatus::SUCCESS) {
                                  response.resize(selectResponse.size());
                                  for (size_t i = 0; i < selectResponse.size(); i++) {
                                      response[i] = selectResponse[i];
                                  }
                              }
                          });
    if (statusReturned == SecureElementStatus::SUCCESS) {
        EXPECT_LE((unsigned int)2, response.size());
        se_->closeChannel(0);
        return;
    }
    EXPECT_EQ(SecureElementStatus::UNSUPPORTED_OPERATION, statusReturned);
}

/*
 * GetATR
 */
TEST_F(SecureElementHidlTest, getAtr) {
    std::vector<uint8_t> atr;
    se_->getAtr([&atr](std::vector<uint8_t> atrReturned) {
        atr.resize(atrReturned.size());
        for (size_t i = 0; i < atrReturned.size(); i++) {
            atr[i] = atrReturned[i];
        }
    });
    if (atr.size() == 0) {
        return;
    }
    EXPECT_GE((unsigned int)32, atr.size());
    EXPECT_LE((unsigned int)1, atr.size());
}

/*
 * OpenCloseLogicalChannel:
 * Open Channel
 * Check status
 * Close Channel
 */
TEST_F(SecureElementHidlTest, openCloseLogicalChannel) {
    std::vector<uint8_t> aid = ANDROID_TEST_AID;
    SecureElementStatus statusReturned;
    LogicalChannelResponse response;
    se_->openLogicalChannel(
        aid, 0x00,
        [&statusReturned, &response](LogicalChannelResponse channelResponse,
                                     SecureElementStatus status) {
            statusReturned = status;
            if (status == SecureElementStatus::SUCCESS) {
                response.channelNumber = channelResponse.channelNumber;
                response.selectResponse.resize(channelResponse.selectResponse.size());
                for (size_t i = 0; i < channelResponse.selectResponse.size(); i++) {
                    response.selectResponse[i] = channelResponse.selectResponse[i];
                }
            }
        });
    EXPECT_EQ(SecureElementStatus::SUCCESS, statusReturned);
    EXPECT_LE((unsigned int)2, response.selectResponse.size());
    EXPECT_LE(1, response.channelNumber);
    EXPECT_EQ(SecureElementStatus::SUCCESS, se_->closeChannel(response.channelNumber));
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(SecureElementHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    SecureElementHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}
