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
#include <android/binder_manager.h>

#include "radio_messaging_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioMessagingTest::SetUp() {
    RadioServiceTest::SetUp();
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
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping sendSms "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

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
}

/*
 * Test IRadioMessaging.sendSmsExpectMore() for the response returned.
 */
TEST_P(RadioMessagingTest, sendSmsExpectMore) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping sendSmsExpectMore "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

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
}

/*
 * Test IRadioMessaging.sendCdmaSms() for the response returned.
 */
TEST_P(RadioMessagingTest, sendCdmaSms) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping sendCdmaSms "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

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
}

/*
 * Test IRadioMessaging.sendCdmaSmsExpectMore() for the response returned.
 */
TEST_P(RadioMessagingTest, sendCdmaSmsExpectMore) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping sendCdmaSmsExpectMore "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

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

/*
 * Test IRadioMessaging.setGsmBroadcastConfig() for the response returned.
 */
TEST_P(RadioMessagingTest, setGsmBroadcastConfig) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping setGsmBroadcastConfig "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

    serial = GetRandomSerialNumber();

    // Create GsmBroadcastSmsConfigInfo #1
    GsmBroadcastSmsConfigInfo gbSmsConfig1;
    gbSmsConfig1.fromServiceId = 4352;
    gbSmsConfig1.toServiceId = 4354;
    gbSmsConfig1.fromCodeScheme = 0;
    gbSmsConfig1.toCodeScheme = 255;
    gbSmsConfig1.selected = true;

    // Create GsmBroadcastSmsConfigInfo #2
    GsmBroadcastSmsConfigInfo gbSmsConfig2;
    gbSmsConfig2.fromServiceId = 4356;
    gbSmsConfig2.toServiceId = 4356;
    gbSmsConfig2.fromCodeScheme = 0;
    gbSmsConfig2.toCodeScheme = 255;
    gbSmsConfig2.selected = true;

    // Create GsmBroadcastSmsConfigInfo #3
    GsmBroadcastSmsConfigInfo gbSmsConfig3;
    gbSmsConfig3.fromServiceId = 4370;
    gbSmsConfig3.toServiceId = 4379;
    gbSmsConfig3.fromCodeScheme = 0;
    gbSmsConfig3.toCodeScheme = 255;
    gbSmsConfig3.selected = true;

    // Create GsmBroadcastSmsConfigInfo #4
    GsmBroadcastSmsConfigInfo gbSmsConfig4;
    gbSmsConfig4.fromServiceId = 4383;
    gbSmsConfig4.toServiceId = 4391;
    gbSmsConfig4.fromCodeScheme = 0;
    gbSmsConfig4.toCodeScheme = 255;
    gbSmsConfig4.selected = true;

    // Create GsmBroadcastSmsConfigInfo #5
    GsmBroadcastSmsConfigInfo gbSmsConfig5;
    gbSmsConfig5.fromServiceId = 4392;
    gbSmsConfig5.toServiceId = 4392;
    gbSmsConfig5.fromCodeScheme = 0;
    gbSmsConfig5.toCodeScheme = 255;
    gbSmsConfig5.selected = true;

    std::vector<GsmBroadcastSmsConfigInfo> gsmBroadcastSmsConfigsInfoList = {
            gbSmsConfig1, gbSmsConfig2, gbSmsConfig3, gbSmsConfig4, gbSmsConfig5};

    radio_messaging->setGsmBroadcastConfig(serial, gsmBroadcastSmsConfigsInfoList);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                      RadioError::INVALID_MODEM_STATE, RadioError::INVALID_STATE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.getGsmBroadcastConfig() for the response returned.
 */
TEST_P(RadioMessagingTest, getGsmBroadcastConfig) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping getGsmBroadcastConfig "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

    serial = GetRandomSerialNumber();

    radio_messaging->getGsmBroadcastConfig(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_messaging->rspInfo.error,
                {RadioError::NONE, RadioError::INVALID_MODEM_STATE, RadioError::INVALID_STATE},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.setCdmaBroadcastConfig() for the response returned.
 */
TEST_P(RadioMessagingTest, setCdmaBroadcastConfig) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping setCdmaBroadcastConfig "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();

    CdmaBroadcastSmsConfigInfo cbSmsConfig;
    cbSmsConfig.serviceCategory = 4096;
    cbSmsConfig.language = 1;
    cbSmsConfig.selected = true;

    std::vector<CdmaBroadcastSmsConfigInfo> cdmaBroadcastSmsConfigInfoList = {cbSmsConfig};

    radio_messaging->setCdmaBroadcastConfig(serial, cdmaBroadcastSmsConfigInfoList);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_MODEM_STATE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.getCdmaBroadcastConfig() for the response returned.
 */
TEST_P(RadioMessagingTest, getCdmaBroadcastConfig) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping getCdmaBroadcastConfig "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();

    radio_messaging->getCdmaBroadcastConfig(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error, {RadioError::NONE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.setCdmaBroadcastActivation() for the response returned.
 */
TEST_P(RadioMessagingTest, setCdmaBroadcastActivation) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping setCdmaBroadcastActivation "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();
    bool activate = false;

    radio_messaging->setCdmaBroadcastActivation(serial, activate);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.setGsmBroadcastActivation() for the response returned.
 */
TEST_P(RadioMessagingTest, setGsmBroadcastActivation) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping setGsmBroadcastActivation "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

    serial = GetRandomSerialNumber();
    bool activate = false;

    radio_messaging->setGsmBroadcastActivation(serial, activate);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_messaging->rspInfo.error,
                {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::INVALID_MODEM_STATE,
                 RadioError::INVALID_STATE, RadioError::OPERATION_NOT_ALLOWED},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.acknowledgeLastIncomingGsmSms() for the response returned.
 */
TEST_P(RadioMessagingTest, acknowledgeLastIncomingGsmSms) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping acknowledgeLastIncomingGsmSms "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

    serial = GetRandomSerialNumber();
    bool success = true;

    radio_messaging->acknowledgeLastIncomingGsmSms(
            serial, success, SmsAcknowledgeFailCause::MEMORY_CAPACITY_EXCEEDED);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.acknowledgeIncomingGsmSmsWithPdu() for the response returned.
 */
TEST_P(RadioMessagingTest, acknowledgeIncomingGsmSmsWithPdu) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping acknowledgeIncomingGsmSmsWithPdu "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

    serial = GetRandomSerialNumber();
    bool success = true;
    std::string ackPdu = "";

    radio_messaging->acknowledgeIncomingGsmSmsWithPdu(serial, success, ackPdu);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::NO_SMS_TO_ACK},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.acknowledgeLastIncomingCdmaSms() for the response returned.
 */
TEST_P(RadioMessagingTest, acknowledgeLastIncomingCdmaSms) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping acknowledgeIncomingGsmSmsWithPdu "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();

    // Create a CdmaSmsAck
    CdmaSmsAck cdmaSmsAck;
    cdmaSmsAck.errorClass = false;
    cdmaSmsAck.smsCauseCode = 1;

    radio_messaging->acknowledgeLastIncomingCdmaSms(serial, cdmaSmsAck);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::NO_SMS_TO_ACK},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.sendImsSms() for the response returned.
 */
TEST_P(RadioMessagingTest, sendImsSms) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
            GTEST_SKIP() << "Skipping acknowledgeIncomingGsmSmsWithPdu "
                            "due to undefined FEATURE_TELEPHONY_IMS";
        }
    }

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

    // Create an ImsSmsMessage
    ImsSmsMessage msg;
    msg.tech = RadioTechnologyFamily::THREE_GPP2;
    msg.retry = false;
    msg.messageRef = 0;
    msg.cdmaMessage = (std::vector<CdmaSmsMessage>){cdmaSmsMessage};
    msg.gsmMessage = (std::vector<GsmSmsMessage>){};

    radio_messaging->sendImsSms(serial, msg);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS}, CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.getSmscAddress() for the response returned.
 */
TEST_P(RadioMessagingTest, getSmscAddress) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping getSmscAddress "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

    serial = GetRandomSerialNumber();

    radio_messaging->getSmscAddress(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error,
                                     {RadioError::INVALID_MODEM_STATE, RadioError::INVALID_STATE,
                                      RadioError::SIM_ABSENT},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.setSmscAddress() for the response returned.
 */
TEST_P(RadioMessagingTest, setSmscAddress) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping setSmscAddress "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

    serial = GetRandomSerialNumber();
    std::string address = std::string("smscAddress");

    radio_messaging->setSmscAddress(serial, address);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_SMS_FORMAT,
                                      RadioError::SIM_ABSENT},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.writeSmsToSim() for the response returned.
 */
TEST_P(RadioMessagingTest, writeSmsToSim) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping writeSmsToSim "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

    serial = GetRandomSerialNumber();
    SmsWriteArgs smsWriteArgs;
    smsWriteArgs.status = SmsWriteArgs::STATUS_REC_UNREAD;
    smsWriteArgs.smsc = "";
    smsWriteArgs.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio_messaging->writeSmsToSim(serial, smsWriteArgs);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_messaging->rspInfo.error,
                {RadioError::NONE, RadioError::ENCODING_ERR, RadioError::INVALID_ARGUMENTS,
                 RadioError::INVALID_SMSC_ADDRESS, RadioError::MODEM_ERR,
                 RadioError::NETWORK_NOT_READY, RadioError::NO_RESOURCES, RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.deleteSmsOnSim() for the response returned.
 */
TEST_P(RadioMessagingTest, deleteSmsOnSim) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping deleteSmsOnSim "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

    serial = GetRandomSerialNumber();
    int index = 1;

    radio_messaging->deleteSmsOnSim(serial, index);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_messaging->rspInfo.error,
                {RadioError::NONE, RadioError::ENCODING_ERR, RadioError::INVALID_ARGUMENTS,
                 RadioError::INVALID_MODEM_STATE, RadioError::NO_SUCH_ENTRY, RadioError::MODEM_ERR,
                 RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.writeSmsToRuim() for the response returned.
 */
TEST_P(RadioMessagingTest, writeSmsToRuim) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping writeSmsToRuim "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

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

    // Create a CdmaSmsWriteArgs
    CdmaSmsWriteArgs cdmaSmsWriteArgs;
    cdmaSmsWriteArgs.status = CdmaSmsWriteArgs::STATUS_REC_UNREAD;
    cdmaSmsWriteArgs.message = cdmaSmsMessage;

    radio_messaging->writeSmsToRuim(serial, cdmaSmsWriteArgs);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_messaging->rspInfo.error,
                {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::INVALID_SMS_FORMAT,
                 RadioError::INVALID_SMSC_ADDRESS, RadioError::INVALID_STATE, RadioError::MODEM_ERR,
                 RadioError::NO_SUCH_ENTRY, RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.deleteSmsOnRuim() for the response returned.
 */
TEST_P(RadioMessagingTest, deleteSmsOnRuim) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping deleteSmsOnRuim "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();
    int index = 1;

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

    // Create a CdmaSmsWriteArgs
    CdmaSmsWriteArgs cdmaSmsWriteArgs;
    cdmaSmsWriteArgs.status = CdmaSmsWriteArgs::STATUS_REC_UNREAD;
    cdmaSmsWriteArgs.message = cdmaSmsMessage;

    radio_messaging->deleteSmsOnRuim(serial, index);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_messaging->rspInfo.error,
                {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::INVALID_MODEM_STATE,
                 RadioError::MODEM_ERR, RadioError::NO_SUCH_ENTRY, RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioMessaging.reportSmsMemoryStatus() for the response returned.
 */
TEST_P(RadioMessagingTest, reportSmsMemoryStatus) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_MESSAGING)) {
            GTEST_SKIP() << "Skipping reportSmsMemoryStatus "
                            "due to undefined FEATURE_TELEPHONY_MESSAGING";
        }
    }

    serial = GetRandomSerialNumber();
    bool available = true;

    radio_messaging->reportSmsMemoryStatus(serial, available);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_messaging->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_messaging->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_messaging->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE,
                                      RadioError::MODEM_ERR, RadioError::SIM_ABSENT},
                                     CHECK_GENERAL_ERROR));
    }
}
