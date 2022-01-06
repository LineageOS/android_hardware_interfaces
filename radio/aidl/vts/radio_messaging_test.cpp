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

#include <aidl/android/hardware/radio/config/IRadioConfig.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>

#include "radio_messaging_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioMessagingTest::SetUp() {
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_messaging = IRadioMessaging::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_messaging.get());

    radioRsp_messaging = ndk::SharedRefBase::make<RadioMessagingResponse>(*this);
    ASSERT_NE(nullptr, radioRsp_messaging.get());

    count_ = 0;

    radioInd_messaging = ndk::SharedRefBase::make<RadioMessagingIndication>(*this);
    ASSERT_NE(nullptr, radioInd_messaging.get());

    radio_messaging->setResponseFunctions(radioRsp_messaging, radioInd_messaging);

    // Assert IRadioSim exists and SIM is present before testing
    radio_sim = sim::IRadioSim::fromBinder(ndk::SpAIBinder(
            AServiceManager_waitForService("android.hardware.radio.sim.IRadioSim/slot1")));
    ASSERT_NE(nullptr, radio_sim.get());
    updateSimCardStatus();
    EXPECT_EQ(CardStatus::STATE_PRESENT, cardStatus.cardState);

    // Assert IRadioConfig exists before testing
    radio_config = config::IRadioConfig::fromBinder(ndk::SpAIBinder(
            AServiceManager_waitForService("android.hardware.radio.config.IRadioConfig/default")));
    ASSERT_NE(nullptr, radio_config.get());
}

/*
 * Test IRadioMessaging.sendSms() for the response returned.
 */
TEST_P(RadioMessagingTest, sendSms) {
    LOG(DEBUG) << "sendSms";
    serial = GetRandomSerialNumber();
    GsmSmsMessage msg;
    msg.smscPdu = "";
    msg.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio_messaging->sendSms(serial, msg);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_messaging->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
        EXPECT_EQ(0, radioRsp_messaging->sendSmsResult.errorCode);
    }
    LOG(DEBUG) << "sendSms finished";
}

/*
 * Test IRadioMessaging.sendSmsExpectMore() for the response returned.
 */
TEST_P(RadioMessagingTest, sendSmsExpectMore) {
    LOG(DEBUG) << "sendSmsExpectMore";
    serial = GetRandomSerialNumber();
    GsmSmsMessage msg;
    msg.smscPdu = "";
    msg.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio_messaging->sendSmsExpectMore(serial, msg);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_messaging->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "sendSmsExpectMore finished";
}

/*
 * Test IRadioMessaging.sendCdmaSms() for the response returned.
 */
TEST_P(RadioMessagingTest, sendCdmaSms) {
    LOG(DEBUG) << "sendCdmaSms";
    serial = GetRandomSerialNumber();

    // Create a CdmaSmsAddress
    CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = CdmaSmsAddress::DIGIT_MODE_FOUR_BIT;
    cdmaSmsAddress.isNumberModeDataNetwork = false;
    cdmaSmsAddress.numberType = CdmaSmsAddress::NUMBER_TYPE_UNKNOWN;
    cdmaSmsAddress.numberPlan = CdmaSmsAddress::NUMBER_PLAN_UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>){11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3};

    // Create a CdmaSmsSubAddress
    CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = CdmaSmsSubaddress::SUBADDRESS_TYPE_NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>){};

    // Create a CdmaSmsMessage
    CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData =
            (std::vector<uint8_t>){15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    radio_messaging->sendCdmaSms(serial, cdmaSmsMessage);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_messaging->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "sendCdmaSms finished";
}

/*
 * Test IRadioMessaging.sendCdmaSmsExpectMore() for the response returned.
 */
TEST_P(RadioMessagingTest, sendCdmaSmsExpectMore) {
    serial = GetRandomSerialNumber();

    // Create a CdmaSmsAddress
    CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = CdmaSmsAddress::DIGIT_MODE_FOUR_BIT;
    cdmaSmsAddress.isNumberModeDataNetwork = false;
    cdmaSmsAddress.numberType = CdmaSmsAddress::NUMBER_TYPE_UNKNOWN;
    cdmaSmsAddress.numberPlan = CdmaSmsAddress::NUMBER_PLAN_UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>){11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3};

    // Create a CdmaSmsSubAddress
    CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = CdmaSmsSubaddress::SUBADDRESS_TYPE_NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>){};

    // Create a CdmaSmsMessage
    CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData =
            (std::vector<uint8_t>){15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    radio_messaging->sendCdmaSmsExpectMore(serial, cdmaSmsMessage);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_messaging->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
}
