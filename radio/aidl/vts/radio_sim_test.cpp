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

#include <aidl/android/hardware/radio/RadioConst.h>
#include <aidl/android/hardware/radio/config/IRadioConfig.h>
#include <android/binder_manager.h>

#include "radio_sim_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioSimTest::SetUp() {
    RadioServiceTest::SetUp();
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
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping setSimCardPower "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

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
        updateSimSlotStatus(cardStatus.slotMap.physicalSlotId);
        EXPECT_EQ(CardStatus::STATE_PRESENT, cardStatus.cardState);
        EXPECT_EQ(CardStatus::STATE_PRESENT, slotStatus.cardState);
        if (CardStatus::STATE_PRESENT == slotStatus.cardState) {
            ASSERT_TRUE(slotStatus.portInfo[0].portActive);
            EXPECT_EQ(0, cardStatus.slotMap.portId);
        }
    }
}

/*
 * Test IRadioSim.setCarrierInfoForImsiEncryption() for the response returned.
 */
TEST_P(RadioSimTest, setCarrierInfoForImsiEncryption) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping setCarrierInfoForImsiEncryption "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

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
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping getSimPhonebookRecords "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

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
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping getSimPhonebookCapacity "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

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
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping updateSimPhonebookRecords "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

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

/*
 * Test IRadioSim.enableUiccApplications() for the response returned.
 * For SIM ABSENT case.
 */
TEST_P(RadioSimTest, togglingUiccApplicationsSimAbsent) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping togglingUiccApplicationsSimAbsent "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    // This test case only test SIM ABSENT case.
    if (cardStatus.cardState != CardStatus::STATE_ABSENT) return;

    // Disable Uicc applications.
    serial = GetRandomSerialNumber();
    radio_sim->enableUiccApplications(serial, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    // As SIM is absent, RadioError::SIM_ABSENT should be thrown.
    EXPECT_EQ(RadioError::SIM_ABSENT, radioRsp_sim->rspInfo.error);

    // Query Uicc application enablement.
    serial = GetRandomSerialNumber();
    radio_sim->areUiccApplicationsEnabled(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    // As SIM is absent, RadioError::SIM_ABSENT should be thrown.
    EXPECT_EQ(RadioError::SIM_ABSENT, radioRsp_sim->rspInfo.error);
}

/*
 * Test IRadioSim.enableUiccApplications() for the response returned.
 * For SIM PRESENT case.
 */
TEST_P(RadioSimTest, togglingUiccApplicationsSimPresent) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping togglingUiccApplicationsSimPresent "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    // This test case only test SIM ABSENT case.
    if (cardStatus.cardState != CardStatus::STATE_PRESENT) return;
    if (cardStatus.applications.size() == 0) return;

    // Disable Uicc applications.
    serial = GetRandomSerialNumber();
    radio_sim->enableUiccApplications(serial, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    // As SIM is present, there shouldn't be error.
    EXPECT_EQ(RadioError::NONE, radioRsp_sim->rspInfo.error);

    // Query Uicc application enablement.
    serial = GetRandomSerialNumber();
    radio_sim->areUiccApplicationsEnabled(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    // As SIM is present, there shouldn't be error.
    EXPECT_EQ(RadioError::NONE, radioRsp_sim->rspInfo.error);
    ASSERT_FALSE(radioRsp_sim->areUiccApplicationsEnabled);

    // Enable Uicc applications.
    serial = GetRandomSerialNumber();
    radio_sim->enableUiccApplications(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    // As SIM is present, there shouldn't be error.
    EXPECT_EQ(RadioError::NONE, radioRsp_sim->rspInfo.error);

    // Query Uicc application enablement.
    serial = GetRandomSerialNumber();
    radio_sim->areUiccApplicationsEnabled(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    // As SIM is present, there shouldn't be error.
    EXPECT_EQ(RadioError::NONE, radioRsp_sim->rspInfo.error);
    ASSERT_TRUE(radioRsp_sim->areUiccApplicationsEnabled);
}

/*
 * Test IRadioSim.areUiccApplicationsEnabled() for the response returned.
 */
TEST_P(RadioSimTest, areUiccApplicationsEnabled) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping areUiccApplicationsEnabled "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    // Disable Uicc applications.
    serial = GetRandomSerialNumber();
    radio_sim->areUiccApplicationsEnabled(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    // If SIM is absent, RadioError::SIM_ABSENT should be thrown. Otherwise there shouldn't be any
    // error.
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        EXPECT_EQ(RadioError::SIM_ABSENT, radioRsp_sim->rspInfo.error);
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp_sim->rspInfo.error);
    }
}

/*
 * Test IRadioSim.getAllowedCarriers() for the response returned.
 */
TEST_P(RadioSimTest, getAllowedCarriers) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping getAllowedCarriers "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    radio_sim->getAllowedCarriers(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/**
 * Test IRadioSim.setAllowedCarriers() for the response returned.
 */
TEST_P(RadioSimTest, setAllowedCarriers) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping setAllowedCarriers "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    CarrierRestrictions carrierRestrictions;
    memset(&carrierRestrictions, 0, sizeof(carrierRestrictions));
    carrierRestrictions.allowedCarriers.resize(1);
    carrierRestrictions.excludedCarriers.resize(0);
    carrierRestrictions.allowedCarriers[0].mcc = std::string("123");
    carrierRestrictions.allowedCarriers[0].mnc = std::string("456");
    carrierRestrictions.allowedCarriers[0].matchType = Carrier::MATCH_TYPE_ALL;
    carrierRestrictions.allowedCarriers[0].matchData = std::string();
    carrierRestrictions.allowedCarriersPrioritized = true;
    SimLockMultiSimPolicy multisimPolicy = SimLockMultiSimPolicy::NO_MULTISIM_POLICY;

    radio_sim->setAllowedCarriers(serial, carrierRestrictions, multisimPolicy);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));

    if (radioRsp_sim->rspInfo.error == RadioError::NONE) {
        /* Verify the update of the SIM status. This might need some time */
        if (cardStatus.cardState != CardStatus::STATE_ABSENT) {
            updateSimCardStatus();
            auto startTime = std::chrono::system_clock::now();
            while (cardStatus.cardState != CardStatus::STATE_RESTRICTED &&
                   std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now() - startTime)
                                   .count() < 30) {
                /* Set 2 seconds as interval to check card status */
                sleep(2);
                updateSimCardStatus();
            }
            // TODO: uncomment once CF fully supports setAllowedCarriers
            // EXPECT_EQ(CardStatus::STATE_RESTRICTED, cardStatus.cardState);
        }

        /* Verify that configuration was set correctly, retrieving it from the modem */
        serial = GetRandomSerialNumber();

        radio_sim->getAllowedCarriers(serial);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
        EXPECT_EQ(RadioError::NONE, radioRsp_sim->rspInfo.error);

        EXPECT_EQ(1, radioRsp_sim->carrierRestrictionsResp.allowedCarriers.size());
        EXPECT_EQ(0, radioRsp_sim->carrierRestrictionsResp.excludedCarriers.size());
        ASSERT_TRUE(std::string("123") ==
                    radioRsp_sim->carrierRestrictionsResp.allowedCarriers[0].mcc);
        ASSERT_TRUE(std::string("456") ==
                    radioRsp_sim->carrierRestrictionsResp.allowedCarriers[0].mnc);
        EXPECT_EQ(Carrier::MATCH_TYPE_ALL,
                  radioRsp_sim->carrierRestrictionsResp.allowedCarriers[0].matchType);
        ASSERT_TRUE(radioRsp_sim->carrierRestrictionsResp.allowedCarriersPrioritized);
        EXPECT_EQ(SimLockMultiSimPolicy::NO_MULTISIM_POLICY, radioRsp_sim->multiSimPolicyResp);

        sleep(10);

        /**
         * Another test case of the API to cover to allow carrier.
         * If the API is supported, this is also used to reset to no carrier restriction
         * status for cardStatus.
         */
        memset(&carrierRestrictions, 0, sizeof(carrierRestrictions));
        carrierRestrictions.allowedCarriers.resize(0);
        carrierRestrictions.excludedCarriers.resize(0);
        carrierRestrictions.allowedCarriersPrioritized = false;

        serial = GetRandomSerialNumber();
        radio_sim->setAllowedCarriers(serial, carrierRestrictions, multisimPolicy);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

        EXPECT_EQ(RadioError::NONE, radioRsp_sim->rspInfo.error);

        if (cardStatus.cardState != CardStatus::STATE_ABSENT) {
            /* Resetting back to no carrier restriction needs some time */
            updateSimCardStatus();
            auto startTime = std::chrono::system_clock::now();
            while (cardStatus.cardState == CardStatus::STATE_RESTRICTED &&
                   std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now() - startTime)
                                   .count() < 10) {
                /* Set 2 seconds as interval to check card status */
                sleep(2);
                updateSimCardStatus();
            }
            EXPECT_NE(CardStatus::STATE_RESTRICTED, cardStatus.cardState);
            sleep(10);
        }
    }
}

/*
 * Test IRadioSim.getIccCardStatus() for the response returned.
 */
TEST_P(RadioSimTest, getIccCardStatus) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping getIccCardStatus "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    EXPECT_LE(cardStatus.applications.size(), RadioConst::CARD_MAX_APPS);
    EXPECT_LT(cardStatus.gsmUmtsSubscriptionAppIndex, RadioConst::CARD_MAX_APPS);
    EXPECT_LT(cardStatus.cdmaSubscriptionAppIndex, RadioConst::CARD_MAX_APPS);
    EXPECT_LT(cardStatus.imsSubscriptionAppIndex, RadioConst::CARD_MAX_APPS);
}

/*
 * Test IRadioSim.supplyIccPinForApp() for the response returned
 */
TEST_P(RadioSimTest, supplyIccPinForApp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping supplyIccPinForApp "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Pass wrong password and check PASSWORD_INCORRECT returned for 3GPP and
    // 3GPP2 apps only
    for (int i = 0; i < (int)cardStatus.applications.size(); i++) {
        if (cardStatus.applications[i].appType == AppStatus::APP_TYPE_SIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_USIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_RUIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_CSIM) {
            radio_sim->supplyIccPinForApp(serial, std::string("test1"),
                                          cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
            ASSERT_TRUE(CheckAnyOfErrors(
                    radioRsp_sim->rspInfo.error,
                    {RadioError::PASSWORD_INCORRECT, RadioError::REQUEST_NOT_SUPPORTED}));
        }
    }
}

/*
 * Test IRadioSim.supplyIccPukForApp() for the response returned.
 */
TEST_P(RadioSimTest, supplyIccPukForApp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping supplyIccPukForApp "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Pass wrong password and check PASSWORD_INCORRECT returned for 3GPP and
    // 3GPP2 apps only
    for (int i = 0; i < (int)cardStatus.applications.size(); i++) {
        if (cardStatus.applications[i].appType == AppStatus::APP_TYPE_SIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_USIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_RUIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_CSIM) {
            radio_sim->supplyIccPukForApp(serial, std::string("test1"), std::string("test2"),
                                          cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
            ASSERT_TRUE(CheckAnyOfErrors(
                    radioRsp_sim->rspInfo.error,
                    {RadioError::PASSWORD_INCORRECT, RadioError::INVALID_SIM_STATE}));
        }
    }
}

/*
 * Test IRadioSim.supplyIccPin2ForApp() for the response returned.
 */
TEST_P(RadioSimTest, supplyIccPin2ForApp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping supplyIccPin2ForApp "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Pass wrong password and check PASSWORD_INCORRECT returned for 3GPP and
    // 3GPP2 apps only
    for (int i = 0; i < (int)cardStatus.applications.size(); i++) {
        if (cardStatus.applications[i].appType == AppStatus::APP_TYPE_SIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_USIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_RUIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_CSIM) {
            radio_sim->supplyIccPin2ForApp(serial, std::string("test1"),
                                           cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
            ASSERT_TRUE(
                    CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                     {RadioError::PASSWORD_INCORRECT,
                                      RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_PUK2}));
        }
    }
}

/*
 * Test IRadioSim.supplyIccPuk2ForApp() for the response returned.
 */
TEST_P(RadioSimTest, supplyIccPuk2ForApp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping supplyIccPuk2ForApp "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Pass wrong password and check PASSWORD_INCORRECT returned for 3GPP and
    // 3GPP2 apps only
    for (int i = 0; i < (int)cardStatus.applications.size(); i++) {
        if (cardStatus.applications[i].appType == AppStatus::APP_TYPE_SIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_USIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_RUIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_CSIM) {
            radio_sim->supplyIccPuk2ForApp(serial, std::string("test1"), std::string("test2"),
                                           cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
            ASSERT_TRUE(CheckAnyOfErrors(
                    radioRsp_sim->rspInfo.error,
                    {RadioError::PASSWORD_INCORRECT, RadioError::INVALID_SIM_STATE}));
        }
    }
}

/*
 * Test IRadioSim.changeIccPinForApp() for the response returned.
 */
TEST_P(RadioSimTest, changeIccPinForApp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping changeIccPinForApp "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Pass wrong password and check PASSWORD_INCORRECT returned for 3GPP and
    // 3GPP2 apps only
    for (int i = 0; i < (int)cardStatus.applications.size(); i++) {
        if (cardStatus.applications[i].appType == AppStatus::APP_TYPE_SIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_USIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_RUIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_CSIM) {
            radio_sim->changeIccPinForApp(serial, std::string("test1"), std::string("test2"),
                                          cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
            ASSERT_TRUE(CheckAnyOfErrors(
                    radioRsp_sim->rspInfo.error,
                    {RadioError::PASSWORD_INCORRECT, RadioError::REQUEST_NOT_SUPPORTED}));
        }
    }
}

/*
 * Test IRadioSim.changeIccPin2ForApp() for the response returned.
 */
TEST_P(RadioSimTest, changeIccPin2ForApp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping changeIccPin2ForApp "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Pass wrong password and check PASSWORD_INCORRECT returned for 3GPP and
    // 3GPP2 apps only
    for (int i = 0; i < (int)cardStatus.applications.size(); i++) {
        if (cardStatus.applications[i].appType == AppStatus::APP_TYPE_SIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_USIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_RUIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_CSIM) {
            radio_sim->changeIccPin2ForApp(serial, std::string("test1"), std::string("test2"),
                                           cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
            ASSERT_TRUE(
                    CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                     {RadioError::PASSWORD_INCORRECT,
                                      RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_PUK2}));
        }
    }
}

/*
 * Test IRadioSim.getImsiForApp() for the response returned.
 */
TEST_P(RadioSimTest, getImsiForApp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping getImsiForApp "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Check success returned while getting imsi for 3GPP and 3GPP2 apps only
    for (int i = 0; i < (int)cardStatus.applications.size(); i++) {
        if (cardStatus.applications[i].appType == AppStatus::APP_TYPE_SIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_USIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_RUIM ||
            cardStatus.applications[i].appType == AppStatus::APP_TYPE_CSIM) {
            radio_sim->getImsiForApp(serial, cardStatus.applications[i].aidPtr);
            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
            EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
            ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error, {RadioError::NONE},
                                         CHECK_GENERAL_ERROR));

            // IMSI (MCC+MNC+MSIN) is at least 6 digits, but not more than 15
            if (radioRsp_sim->rspInfo.error == RadioError::NONE) {
                EXPECT_NE(radioRsp_sim->imsi, std::string());
                EXPECT_GE((int)(radioRsp_sim->imsi).size(), 6);
                EXPECT_LE((int)(radioRsp_sim->imsi).size(), 15);
            }
        }
    }
}

/*
 * Test IRadioSim.iccIoForApp() for the response returned.
 */
TEST_P(RadioSimTest, iccIoForApp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping iccIoForApp "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    for (int i = 0; i < (int)cardStatus.applications.size(); i++) {
        IccIo iccIo;
        iccIo.command = 0xc0;
        iccIo.fileId = 0x6f11;
        iccIo.path = std::string("3F007FFF");
        iccIo.p1 = 0;
        iccIo.p2 = 0;
        iccIo.p3 = 0;
        iccIo.data = std::string();
        iccIo.pin2 = std::string();
        iccIo.aid = cardStatus.applications[i].aidPtr;

        radio_sim->iccIoForApp(serial, iccIo);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
    }
}

/*
 * Test IRadioSim.iccTransmitApduBasicChannel() for the response returned.
 */
TEST_P(RadioSimTest, iccTransmitApduBasicChannel) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping iccTransmitApduBasicChannel "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    SimApdu msg;
    memset(&msg, 0, sizeof(msg));
    msg.data = std::string();

    radio_sim->iccTransmitApduBasicChannel(serial, msg);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
}

/*
 * Test IRadioSim.iccOpenLogicalChannel() for the response returned.
 */
TEST_P(RadioSimTest, iccOpenLogicalChannel) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping iccOpenLogicalChannel "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    int p2 = 0x04;
    // Specified in ISO 7816-4 clause 7.1.1 0x04 means that FCP template is requested.
    for (int i = 0; i < (int)cardStatus.applications.size(); i++) {
        radio_sim->iccOpenLogicalChannel(serial, cardStatus.applications[i].aidPtr, p2);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    }
}

/*
 * Test IRadioSim.iccCloseLogicalChannel() for the response returned.
 */
TEST_P(RadioSimTest, iccCloseLogicalChannel) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping iccCloseLogicalChannel "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    // Try closing invalid channel and check INVALID_ARGUMENTS returned as error
    radio_sim->iccCloseLogicalChannel(serial, 0);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    EXPECT_EQ(RadioError::INVALID_ARGUMENTS, radioRsp_sim->rspInfo.error);
}

/*
 * Test IRadioSim.iccCloseLogicalChannelWithSessionInfo() for the response returned.
 */
TEST_P(RadioSimTest, iccCloseLogicalChannelWithSessionInfo) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping iccCloseLogicalChannelWithSessionInfo "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_sim->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 2) {
        ALOGI("Skipped the test since"
              " iccCloseLogicalChannelWithSessionInfo is not supported on version < 2");
        GTEST_SKIP();
    }
    serial = GetRandomSerialNumber();
    SessionInfo info;
    memset(&info, 0, sizeof(info));
    info.sessionId = 0;
    info.isEs10 = false;

    // Try closing invalid channel and check INVALID_ARGUMENTS returned as error
    radio_sim->iccCloseLogicalChannelWithSessionInfo(serial, info);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    EXPECT_EQ(RadioError::INVALID_ARGUMENTS, radioRsp_sim->rspInfo.error);
}

/*
 * Test IRadioSim.iccTransmitApduLogicalChannel() for the response returned.
 */
TEST_P(RadioSimTest, iccTransmitApduLogicalChannel) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping iccTransmitApduLogicalChannel "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    SimApdu msg;
    memset(&msg, 0, sizeof(msg));
    msg.data = std::string();

    radio_sim->iccTransmitApduLogicalChannel(serial, msg);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
}

/*
 * Test IRadioSim.requestIccSimAuthentication() for the response returned.
 */
TEST_P(RadioSimTest, requestIccSimAuthentication) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping requestIccSimAuthentication "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Pass wrong challenge string and check RadioError::INVALID_ARGUMENTS
    // or REQUEST_NOT_SUPPORTED returned as error.
    for (int i = 0; i < (int)cardStatus.applications.size(); i++) {
        radio_sim->requestIccSimAuthentication(serial, 0, std::string("test"),
                                               cardStatus.applications[i].aidPtr);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_sim->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadioSim.getFacilityLockForApp() for the response returned.
 */
TEST_P(RadioSimTest, getFacilityLockForApp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping getFacilityLockForApp "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    std::string facility = "";
    std::string password = "";
    int32_t serviceClass = 1;
    std::string appId = "";

    radio_sim->getFacilityLockForApp(serial, facility, password, serviceClass, appId);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioSim.setFacilityLockForApp() for the response returned.
 */
TEST_P(RadioSimTest, setFacilityLockForApp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping setFacilityLockForApp "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    std::string facility = "";
    bool lockState = false;
    std::string password = "";
    int32_t serviceClass = 1;
    std::string appId = "";

    radio_sim->setFacilityLockForApp(serial, facility, lockState, password, serviceClass, appId);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioSim.getCdmaSubscription() for the response returned.
 */
TEST_P(RadioSimTest, getCdmaSubscription) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping getCdmaSubscription "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();

    radio_sim->getCdmaSubscription(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_sim->rspInfo.error,
                {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadioSim.getCdmaSubscriptionSource() for the response returned.
 */
TEST_P(RadioSimTest, getCdmaSubscriptionSource) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping getCdmaSubscriptionSource "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();

    radio_sim->getCdmaSubscriptionSource(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_sim->rspInfo.error,
                {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadioSim.setCdmaSubscriptionSource() for the response returned.
 */
TEST_P(RadioSimTest, setCdmaSubscriptionSource) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping setCdmaSubscriptionSource "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();

    radio_sim->setCdmaSubscriptionSource(serial, CdmaSubscriptionSource::RUIM_SIM);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_sim->rspInfo.error,
                {RadioError::NONE, RadioError::SIM_ABSENT, RadioError::SUBSCRIPTION_NOT_AVAILABLE},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioSim.setUiccSubscription() for the response returned.
 */
TEST_P(RadioSimTest, setUiccSubscription) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping setUiccSubscription "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    SelectUiccSub item;
    memset(&item, 0, sizeof(item));

    radio_sim->setUiccSubscription(serial, item);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                 {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                  RadioError::MODEM_ERR, RadioError::SUBSCRIPTION_NOT_SUPPORTED},
                                 CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioSim.sendEnvelope() for the response returned.
 */
TEST_P(RadioSimTest, sendEnvelope) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping sendEnvelope "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Test with sending empty string
    std::string content = "";

    radio_sim->sendEnvelope(serial, content);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                      RadioError::MODEM_ERR, RadioError::SIM_ABSENT},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioSim.sendTerminalResponseToSim() for the response returned.
 */
TEST_P(RadioSimTest, sendTerminalResponseToSim) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping sendTerminalResponseToSim "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Test with sending empty string
    std::string commandResponse = "";

    radio_sim->sendTerminalResponseToSim(serial, commandResponse);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_sim->rspInfo.error,
                {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioSim.reportStkServiceIsRunning() for the response returned.
 */
TEST_P(RadioSimTest, reportStkServiceIsRunning) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping reportStkServiceIsRunning "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    radio_sim->reportStkServiceIsRunning(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_sim->rspInfo.error, {RadioError::NONE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioSim.sendEnvelopeWithStatus() for the response returned with empty
 * string.
 */
TEST_P(RadioSimTest, sendEnvelopeWithStatus) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping sendEnvelopeWithStatus "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    // Test with sending empty string
    std::string contents = "";

    radio_sim->sendEnvelopeWithStatus(serial, contents);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_sim->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_sim->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_sim->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::MODEM_ERR, RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
}
