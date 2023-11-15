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

#include "radio_modem_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioModemTest::SetUp() {
    RadioServiceTest::SetUp();
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_modem = IRadioModem::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_modem.get());

    radioRsp_modem = ndk::SharedRefBase::make<RadioModemResponse>(*this);
    ASSERT_NE(nullptr, radioRsp_modem.get());

    radioInd_modem = ndk::SharedRefBase::make<RadioModemIndication>(*this);
    ASSERT_NE(nullptr, radioInd_modem.get());

    radio_modem->setResponseFunctions(radioRsp_modem, radioInd_modem);

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
 * Test IRadioModem.setRadioPower() for the response returned.
 */
TEST_P(RadioModemTest, setRadioPower_emergencyCall_cancelled) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setRadioPower_emergencyCall_cancelled "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    // Set radio power to off.
    serial = GetRandomSerialNumber();
    radio_modem->setRadioPower(serial, false, false, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_modem->rspInfo.error);

    // Set radio power to on with forEmergencyCall being true. This should put modem to only scan
    // emergency call bands.
    serial = GetRandomSerialNumber();
    radio_modem->setRadioPower(serial, true, true, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_modem->rspInfo.error);

    // Set radio power to on with forEmergencyCall being false. This should put modem in regular
    // operation modem.
    serial = GetRandomSerialNumber();
    radio_modem->setRadioPower(serial, true, false, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_modem->rspInfo.error);
}

/*
 * Test IRadioModem.enableModem() for the response returned.
 */
TEST_P(RadioModemTest, enableModem) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY)) {
            GTEST_SKIP() << "Skipping enableModem "
                            "due to undefined FEATURE_TELEPHONY";
        }
    }

    serial = GetRandomSerialNumber();

    if (isSsSsEnabled()) {
        ALOGI("enableModem, no need to test in single SIM mode");
        return;
    }

    bool responseToggle = radioRsp_modem->enableModemResponseToggle;
    ndk::ScopedAStatus res = radio_modem->enableModem(serial, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);
    ALOGI("getModemStackStatus, rspInfo.error = %s\n",
          toString(radioRsp_modem->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error,
                                 {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::MODEM_ERR, RadioError::INVALID_STATE}));

    // checking if getModemStackStatus returns true, as modem was enabled above
    if (RadioError::NONE == radioRsp_modem->rspInfo.error) {
        // wait until modem enabling is finished
        while (responseToggle == radioRsp_modem->enableModemResponseToggle) {
            sleep(1);
        }
        ndk::ScopedAStatus resEnabled = radio_modem->getModemStackStatus(serial);
        ASSERT_OK(resEnabled);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);
        ALOGI("getModemStackStatus, rspInfo.error = %s\n",
              toString(radioRsp_modem->rspInfo.error).c_str());
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::MODEM_ERR, RadioError::INVALID_STATE}));
        // verify that enableModem did set isEnabled correctly
        EXPECT_EQ(true, radioRsp_modem->isModemEnabled);
    }
}

/*
 * Test IRadioModem.getModemStackStatus() for the response returned.
 */
TEST_P(RadioModemTest, getModemStackStatus) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY)) {
            GTEST_SKIP() << "Skipping getModemStackStatus "
                            "due to undefined FEATURE_TELEPHONY";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_modem->getModemStackStatus(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);
    ALOGI("getModemStackStatus, rspInfo.error = %s\n",
          toString(radioRsp_modem->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_modem->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));
}

/*
 * Test IRadioModem.getBasebandVersion() for the response returned.
 */
TEST_P(RadioModemTest, getBasebandVersion) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY)) {
            GTEST_SKIP() << "Skipping getBasebandVersion "
                            "due to undefined FEATURE_TELEPHONY";
        }
    }

    serial = GetRandomSerialNumber();

    radio_modem->getBasebandVersion(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp_modem->rspInfo.error);
    }
}

/*
 * Test IRadioModem.getDeviceIdentity() for the response returned.
 */
TEST_P(RadioModemTest, getDeviceIdentity) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY)) {
            GTEST_SKIP() << "Skipping getDeviceIdentity "
                            "due to undefined FEATURE_TELEPHONY";
        }
    }

    serial = GetRandomSerialNumber();

    radio_modem->getDeviceIdentity(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error,
                                     {RadioError::NONE, RadioError::EMPTY_RECORD}));
    }
}

/*
 * Test IRadioModem.getImei() for the response returned.
 */
TEST_P(RadioModemTest, getImei) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_GSM)) {
            GTEST_SKIP() << "Skipping getImei "
                            "due to undefined FEATURE_TELEPHONY_GSM";
        }
    }

    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_modem->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 2) {
        ALOGI("Skipped the test since getImei is not supported on version < 2");
        GTEST_SKIP();
    }
    serial = GetRandomSerialNumber();

    radio_modem->getImei(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error,
                                     {RadioError::NONE, RadioError::EMPTY_RECORD}));
    }
}

/*
 * Test IRadioModem.nvReadItem() for the response returned.
 */
TEST_P(RadioModemTest, nvReadItem) {
    serial = GetRandomSerialNumber();

    radio_modem->nvReadItem(serial, NvItem::LTE_BAND_ENABLE_25);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error, {RadioError::NONE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioModem.nvWriteItem() for the response returned.
 */
TEST_P(RadioModemTest, nvWriteItem) {
    serial = GetRandomSerialNumber();
    NvWriteItem item;
    memset(&item, 0, sizeof(item));
    item.value = std::string();

    radio_modem->nvWriteItem(serial, item);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error, {RadioError::NONE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioModem.nvWriteCdmaPrl() for the response returned.
 */
TEST_P(RadioModemTest, nvWriteCdmaPrl) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping nvWriteCdmaPrl "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();
    std::vector<uint8_t> prl = {1, 2, 3, 4, 5};

    radio_modem->nvWriteCdmaPrl(serial, std::vector<uint8_t>(prl));
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error, {RadioError::NONE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioModem.nvResetConfig() for the response returned.
 */
TEST_P(RadioModemTest, nvResetConfig) {
    serial = GetRandomSerialNumber();

    radio_modem->nvResetConfig(serial, ResetNvType::FACTORY_RESET);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    // wait until modem reset finishes
    sleep(10);
}

/*
 * Test IRadioModem.getHardwareConfig() for the response returned.
 */
TEST_P(RadioModemTest, getHardwareConfig) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY)) {
            GTEST_SKIP() << "Skipping getHardwareConfig "
                            "due to undefined FEATURE_TELEPHONY";
        }
    }

    serial = GetRandomSerialNumber();

    radio_modem->getHardwareConfig(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error, {RadioError::NONE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * The following test is disabled due to b/64734869
 *
 * Test IRadioModem.requestShutdown() for the response returned.
 */
TEST_P(RadioModemTest, DISABLED_requestShutdown) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping DISABLED_requestShutdown "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_modem->requestShutdown(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error, {RadioError::NONE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioModem.getRadioCapability() for the response returned.
 */
TEST_P(RadioModemTest, getRadioCapability) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getRadioCapability "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_modem->getRadioCapability(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp_modem->rspInfo.error);
    }
}

/*
 * Test IRadioModem.setRadioCapability() for the response returned.
 */
TEST_P(RadioModemTest, setRadioCapability) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setRadioCapability "
                            "due to undefined FEATURE_TELEPHONY";
        }
    }

    serial = GetRandomSerialNumber();
    RadioCapability rc;
    memset(&rc, 0, sizeof(rc));
    rc.logicalModemUuid = std::string();

    radio_modem->setRadioCapability(serial, rc);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioModem.getModemActivityInfo() for the response returned.
 */
TEST_P(RadioModemTest, getModemActivityInfo) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getModemActivityInfo "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_modem->getModemActivityInfo(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadioModem.sendDeviceState() for the response returned.
 */
TEST_P(RadioModemTest, sendDeviceState) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY)) {
            GTEST_SKIP() << "Skipping sendDeviceState "
                            "due to undefined FEATURE_TELEPHONY";
        }
    }

    serial = GetRandomSerialNumber();

    radio_modem->sendDeviceState(serial, DeviceStateType::POWER_SAVE_MODE, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);

    std::cout << static_cast<int>(radioRsp_modem->rspInfo.error) << std::endl;

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_modem->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}
