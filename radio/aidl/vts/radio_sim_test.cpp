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

#include "radio_sim_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioSimTest::SetUp() {
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_sim = IRadioSim::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_sim.get());

    radioRsp_sim = ndk::SharedRefBase::make<RadioSimResponse>(*this);
    ASSERT_NE(nullptr, radioRsp_sim.get());

    count_ = 0;

    radioInd_sim = ndk::SharedRefBase::make<RadioSimIndication>(*this);
    ASSERT_NE(nullptr, radioInd_sim.get());

    radio_sim->setResponseFunctions(radioRsp_sim, radioInd_sim);
    // Assert SIM is present before testing
    updateSimCardStatus();
    EXPECT_EQ(CardStatus::STATE_PRESENT, cardStatus.cardState);

    // Assert IRadioConfig exists before testing
    radio_config = config::IRadioConfig::fromBinder(ndk::SpAIBinder(
            AServiceManager_waitForService("android.hardware.radio.config.IRadioConfig/default")));
    ASSERT_NE(nullptr, radio_config.get());
}

void RadioSimTest::updateSimCardStatus() {
    serial = GetRandomSerialNumber();
    radio_sim->getIccCardStatus(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_sim->rspInfo.error);
}

/*
 * Test IRadioSim.setSimCardPower() for the response returned.
 */
TEST_P(RadioSimTest, setSimCardPower) {
    /* Test setSimCardPower power down */
    serial = GetRandomSerialNumber();
    radio_sim->setSimCardPower(serial, CardPowerState::POWER_DOWN);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                 {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                  RadioError::RADIO_NOT_AVAILABLE, RadioError::SIM_ERR}));

    // setSimCardPower does not return  until the request is handled, and should not trigger
    // CardStatus::STATE_ABSENT when turning off power
    if (radioRsp_sim->rspInfo.error == RadioError::NONE) {
        /* Wait some time for setting sim power down and then verify it */
        updateSimCardStatus();
        // We cannot assert the consistency of CardState here due to b/203031664
        // EXPECT_EQ(CardStatus::STATE_PRESENT, cardStatus.cardState);
        // applications should be an empty vector of AppStatus
        EXPECT_EQ(0, cardStatus.applications.size());
    }

    // Give some time for modem to fully power down the SIM card
    sleep(MODEM_SET_SIM_POWER_DELAY_IN_SECONDS);

    /* Test setSimCardPower power up */
    serial = GetRandomSerialNumber();
    radio_sim->setSimCardPower(serial, CardPowerState::POWER_UP);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                 {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                  RadioError::RADIO_NOT_AVAILABLE, RadioError::SIM_ERR}));

    // Give some time for modem to fully power up the SIM card
    sleep(MODEM_SET_SIM_POWER_DELAY_IN_SECONDS);

    // setSimCardPower does not return  until the request is handled. Just verify that we still
    // have CardStatus::STATE_PRESENT after turning the power back on
    if (radioRsp_sim->rspInfo.error == RadioError::NONE) {
        updateSimCardStatus();
        EXPECT_EQ(CardStatus::STATE_PRESENT, cardStatus.cardState);
    }
}

/*
 * Test IRadioSim.setCarrierInfoForImsiEncryption() for the response returned.
 */
TEST_P(RadioSimTest, setCarrierInfoForImsiEncryption) {
    serial = GetRandomSerialNumber();
    ImsiEncryptionInfo imsiInfo;
    imsiInfo.mcc = "310";
    imsiInfo.mnc = "004";
    imsiInfo.carrierKey = (std::vector<uint8_t>){1, 2, 3, 4, 5, 6};
    imsiInfo.keyIdentifier = "Test";
    imsiInfo.expirationTime = 20180101;
    imsiInfo.keyType = ImsiEncryptionInfo::PUBLIC_KEY_TYPE_EPDG;

    radio_sim->setCarrierInfoForImsiEncryption(serial, imsiInfo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadioSim.getSimPhonebookRecords() for the response returned.
 */
TEST_P(RadioSimTest, getSimPhonebookRecords) {
    serial = GetRandomSerialNumber();
    radio_sim->getSimPhonebookRecords(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                 {RadioError::INVALID_SIM_STATE, RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::MODEM_ERR, RadioError::INVALID_ARGUMENTS,
                                  RadioError::REQUEST_NOT_SUPPORTED},
                                 CHECK_GENERAL_ERROR));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioSim.getSimPhonebookCapacity for the response returned.
 */
TEST_P(RadioSimTest, getSimPhonebookCapacity) {
    serial = GetRandomSerialNumber();
    radio_sim->getSimPhonebookCapacity(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                 {RadioError::INVALID_SIM_STATE, RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::MODEM_ERR, RadioError::INVALID_ARGUMENTS,
                                  RadioError::REQUEST_NOT_SUPPORTED},
                                 CHECK_GENERAL_ERROR));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED},
                                     CHECK_GENERAL_ERROR));

        PhonebookCapacity pbCapacity = radioRsp_sim->capacity;
        if (pbCapacity.maxAdnRecords > 0) {
            EXPECT_TRUE(pbCapacity.maxNameLen > 0 && pbCapacity.maxNumberLen > 0);
            EXPECT_TRUE(pbCapacity.usedAdnRecords <= pbCapacity.maxAdnRecords);
        }

        if (pbCapacity.maxEmailRecords > 0) {
            EXPECT_TRUE(pbCapacity.maxEmailLen > 0);
            EXPECT_TRUE(pbCapacity.usedEmailRecords <= pbCapacity.maxEmailRecords);
        }

        if (pbCapacity.maxAdditionalNumberRecords > 0) {
            EXPECT_TRUE(pbCapacity.maxAdditionalNumberLen > 0);
            EXPECT_TRUE(pbCapacity.usedAdditionalNumberRecords <=
                        pbCapacity.maxAdditionalNumberRecords);
        }
    }
}

/*
 * Test IRadioSim.updateSimPhonebookRecords() for the response returned.
 */
TEST_P(RadioSimTest, updateSimPhonebookRecords) {
    serial = GetRandomSerialNumber();
    radio_sim->getSimPhonebookCapacity(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                 {RadioError::INVALID_SIM_STATE, RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::MODEM_ERR, RadioError::INVALID_ARGUMENTS,
                                  RadioError::REQUEST_NOT_SUPPORTED},
                                 CHECK_GENERAL_ERROR));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED},
                                     CHECK_GENERAL_ERROR));
        PhonebookCapacity pbCapacity = radioRsp_sim->capacity;

        serial = GetRandomSerialNumber();
        radio_sim->getSimPhonebookRecords(serial);

        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED},
                                     CHECK_GENERAL_ERROR));

        if (pbCapacity.maxAdnRecords > 0 && pbCapacity.usedAdnRecords < pbCapacity.maxAdnRecords) {
            // Add a phonebook record
            PhonebookRecordInfo recordInfo;
            recordInfo.recordId = 0;
            recordInfo.name = "ABC";
            recordInfo.number = "1234567890";
            serial = GetRandomSerialNumber();
            radio_sim->updateSimPhonebookRecords(serial, recordInfo);

            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
            EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
            EXPECT_EQ(RadioError::NONE, radioRsp_sim->rspInfo.error);
            int index = radioRsp_sim->updatedRecordIndex;
            EXPECT_TRUE(index > 0);

            // Deleted a phonebook record
            recordInfo.recordId = index;
            recordInfo.name = "";
            recordInfo.number = "";
            serial = GetRandomSerialNumber();
            radio_sim->updateSimPhonebookRecords(serial, recordInfo);

            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
            EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
            EXPECT_EQ(RadioError::NONE, radioRsp_sim->rspInfo.error);
        }
    }
}
