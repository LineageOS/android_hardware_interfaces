/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android/hardware/radio/1.2/IRadio.h>
#include <radio_hidl_hal_utils_v1_0.h>

/*
 * Test IRadio.getSignalStrength() for the response returned.
 */
TEST_P(RadioHidlTest, getSignalStrength) {
    LOG(DEBUG) << "getSignalStrength";
    serial = GetRandomSerialNumber();

    radio->getSignalStrength(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getSignalStrength finished";
}

/*
 * Test IRadio.getVoiceRegistrationState() for the response returned.
 */
TEST_P(RadioHidlTest, getVoiceRegistrationState) {
    LOG(DEBUG) << "getVoiceRegistrationState";
    serial = GetRandomSerialNumber();

    radio->getVoiceRegistrationState(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getVoiceRegistrationState finished";
}

/*
 * Test IRadio.getOperator() for the response returned.
 */
TEST_P(RadioHidlTest, getOperator) {
    LOG(DEBUG) << "getOperator";
    serial = GetRandomSerialNumber();

    radio->getOperator(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getOperator finished";
}

/*
 * Test IRadio.setRadioPower() for the response returned.
 */
TEST_P(RadioHidlTest, setRadioPower) {
    LOG(DEBUG) << "setRadioPower";
    serial = GetRandomSerialNumber();

    radio->setRadioPower(serial, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "setRadioPower finished";
}

/*
 * Test IRadio.getNetworkSelectionMode() for the response returned.
 */
TEST_P(RadioHidlTest, getNetworkSelectionMode) {
    LOG(DEBUG) << "getNetworkSelectionMode";
    serial = GetRandomSerialNumber();

    radio->getNetworkSelectionMode(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getNetworkSelectionMode finished";
}

/*
 * Test IRadio.setNetworkSelectionModeAutomatic() for the response returned.
 */
TEST_P(RadioHidlTest, setNetworkSelectionModeAutomatic) {
    LOG(DEBUG) << "setNetworkSelectionModeAutomatic";
    serial = GetRandomSerialNumber();

    radio->setNetworkSelectionModeAutomatic(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::ILLEGAL_SIM_OR_ME, RadioError::OPERATION_NOT_ALLOWED},
            CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setNetworkSelectionModeAutomatic finished";
}

/*
 * Test IRadio.setNetworkSelectionModeManual() for the response returned.
 */
TEST_P(RadioHidlTest, setNetworkSelectionModeManual) {
    LOG(DEBUG) << "setNetworkSelectionModeManual";
    serial = GetRandomSerialNumber();

    radio->setNetworkSelectionModeManual(serial, "123456");
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::ILLEGAL_SIM_OR_ME,
                                      RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE},
                                     CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setNetworkSelectionModeManual finished";
}

/*
 * Test IRadio.getAvailableNetworks() for the response returned.
 */
TEST_P(RadioHidlTest, getAvailableNetworks) {
    LOG(DEBUG) << "getAvailableNetworks";
    serial = GetRandomSerialNumber();

    radio->getAvailableNetworks(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait(300));
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    ASSERT_TRUE(radioRsp->rspInfo.type == RadioResponseType::SOLICITED ||
                radioRsp->rspInfo.type == RadioResponseType::SOLICITED_ACK_EXP);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error,
                             {RadioError::NONE, RadioError::CANCELLED, RadioError::DEVICE_IN_USE,
                              RadioError::MODEM_ERR, RadioError::OPERATION_NOT_ALLOWED},
                             CHECK_GENERAL_ERROR));
  }
  LOG(DEBUG) << "getAvailableNetworks finished";
}

/*
 * Test IRadio.getBasebandVersion() for the response returned.
 */
TEST_P(RadioHidlTest, getBasebandVersion) {
    LOG(DEBUG) << "getBasebandVersion";
    serial = GetRandomSerialNumber();

    radio->getBasebandVersion(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getBasebandVersion finished";
}

/*
 * Test IRadio.setBandMode() for the response returned.
 */
TEST_P(RadioHidlTest, setBandMode) {
    LOG(DEBUG) << "setBandMode";
    serial = GetRandomSerialNumber();

    radio->setBandMode(serial, RadioBandMode::BAND_MODE_USA);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setBandMode finished";
}

/*
 * Test IRadio.getAvailableBandModes() for the response returned.
 */
TEST_P(RadioHidlTest, getAvailableBandModes) {
    LOG(DEBUG) << "getAvailableBandModes";
    serial = GetRandomSerialNumber();

    radio->getAvailableBandModes(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getAvailableBandModes finished";
}

/*
 * Test IRadio.setPreferredNetworkType() for the response returned.
 */
TEST_P(RadioHidlTest, setPreferredNetworkType) {
    LOG(DEBUG) << "setPreferredNetworkType";
    serial = GetRandomSerialNumber();

    radio->setPreferredNetworkType(serial, PreferredNetworkType::GSM_ONLY);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setPreferredNetworkType finished";
}

/*
 * Test IRadio.getPreferredNetworkType() for the response returned.
 */
TEST_P(RadioHidlTest, getPreferredNetworkType) {
    LOG(DEBUG) << "getPreferredNetworkType";
    serial = GetRandomSerialNumber();

    radio->getPreferredNetworkType(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getPreferredNetworkType finished";
}

/*
 * Test IRadio.getNeighboringCids() for the response returned.
 */
TEST_P(RadioHidlTest, getNeighboringCids) {
    LOG(DEBUG) << "getNeighboringCids";
    serial = GetRandomSerialNumber();

    radio->getNeighboringCids(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT},
                                     CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "getNeighboringCids finished";
}

/*
 * Test IRadio.setLocationUpdates() for the response returned.
 */
TEST_P(RadioHidlTest, setLocationUpdates) {
    LOG(DEBUG) << "setLocationUpdates";
    serial = GetRandomSerialNumber();

    radio->setLocationUpdates(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE, RadioError::SIM_ABSENT}));
    }
    LOG(DEBUG) << "setLocationUpdates finished";
}

/*
 * Test IRadio.setCdmaRoamingPreference() for the response returned.
 */
TEST_P(RadioHidlTest, setCdmaRoamingPreference) {
    LOG(DEBUG) << "setCdmaRoamingPreference";
    serial = GetRandomSerialNumber();

    radio->setCdmaRoamingPreference(serial, CdmaRoamingType::HOME_NETWORK);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::SIM_ABSENT, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "setCdmaRoamingPreference finished";
}

/*
 * Test IRadio.getCdmaRoamingPreference() for the response returned.
 */
TEST_P(RadioHidlTest, getCdmaRoamingPreference) {
    LOG(DEBUG) << "getCdmaRoamingPreference";
    serial = GetRandomSerialNumber();

    radio->getCdmaRoamingPreference(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error,
                             {RadioError::NONE, RadioError::SIM_ABSENT, RadioError::MODEM_ERR},
                             CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "getCdmaRoamingPreference finished";
}

/*
 * Test IRadio.getTTYMode() for the response returned.
 */
TEST_P(RadioHidlTest, getTTYMode) {
    LOG(DEBUG) << "getTTYMode";
    serial = GetRandomSerialNumber();

    radio->getTTYMode(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getTTYMode finished";
}

/*
 * Test IRadio.setTTYMode() for the response returned.
 */
TEST_P(RadioHidlTest, setTTYMode) {
    LOG(DEBUG) << "setTTYMode";
    serial = GetRandomSerialNumber();

    radio->setTTYMode(serial, TtyMode::OFF);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "setTTYMode finished";
}

/*
 * Test IRadio.setPreferredVoicePrivacy() for the response returned.
 */
TEST_P(RadioHidlTest, setPreferredVoicePrivacy) {
    LOG(DEBUG) << "setPreferredVoicePrivacy";
    serial = GetRandomSerialNumber();

    radio->setPreferredVoicePrivacy(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "setPreferredVoicePrivacy finished";
}

/*
 * Test IRadio.getPreferredVoicePrivacy() for the response returned.
 */
TEST_P(RadioHidlTest, getPreferredVoicePrivacy) {
    LOG(DEBUG) << "getPreferredVoicePrivacy";
    serial = GetRandomSerialNumber();

    radio->getPreferredVoicePrivacy(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "getPreferredVoicePrivacy finished";
}

/*
 * Test IRadio.getCDMASubscription() for the response returned.
 */
TEST_P(RadioHidlTest, getCDMASubscription) {
    LOG(DEBUG) << "getCDMASubscription";
    serial = GetRandomSerialNumber();

    radio->getCDMASubscription(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_ABSENT}));
    }
    LOG(DEBUG) << "getCDMASubscription finished";
}

/*
 * Test IRadio.getDeviceIdentity() for the response returned.
 */
TEST_P(RadioHidlTest, getDeviceIdentity) {
    LOG(DEBUG) << "getDeviceIdentity";
    serial = GetRandomSerialNumber();

    radio->getDeviceIdentity(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::EMPTY_RECORD}));
    }
    LOG(DEBUG) << "getDeviceIdentity finished";
}

/*
 * Test IRadio.exitEmergencyCallbackMode() for the response returned.
 */
TEST_P(RadioHidlTest, exitEmergencyCallbackMode) {
    LOG(DEBUG) << "exitEmergencyCallbackMode";
    serial = GetRandomSerialNumber();

    radio->exitEmergencyCallbackMode(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_ABSENT}));
    }
    LOG(DEBUG) << "exitEmergencyCallbackMode finished";
}

/*
 * Test IRadio.getCdmaSubscriptionSource() for the response returned.
 */
TEST_P(RadioHidlTest, getCdmaSubscriptionSource) {
    LOG(DEBUG) << "getCdmaSubscriptionSource";
    serial = GetRandomSerialNumber();

    radio->getCdmaSubscriptionSource(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_ABSENT}));
    }
    LOG(DEBUG) << "getCdmaSubscriptionSource finished";
}

/*
 * Test IRadio.setCdmaSubscriptionSource() for the response returned.
 */
TEST_P(RadioHidlTest, setCdmaSubscriptionSource) {
    LOG(DEBUG) << "setCdmaSubscriptionSource";
    serial = GetRandomSerialNumber();

    radio->setCdmaSubscriptionSource(serial, CdmaSubscriptionSource::RUIM_SIM);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::SIM_ABSENT, RadioError::SUBSCRIPTION_NOT_AVAILABLE},
            CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setCdmaSubscriptionSource finished";
}

/*
 * Test IRadio.getVoiceRadioTechnology() for the response returned.
 */
TEST_P(RadioHidlTest, getVoiceRadioTechnology) {
    LOG(DEBUG) << "getVoiceRadioTechnology";
    serial = GetRandomSerialNumber();

    radio->getVoiceRadioTechnology(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getVoiceRadioTechnology finished";
}

/*
 * Test IRadio.getCellInfoList() for the response returned.
 */
TEST_P(RadioHidlTest, getCellInfoList) {
    LOG(DEBUG) << "getCellInfoList";
    serial = GetRandomSerialNumber();

    radio->getCellInfoList(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::NO_NETWORK_FOUND},
                                     CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "getCellInfoList finished";
}

/*
 * Test IRadio.setCellInfoListRate() for the response returned.
 */
TEST_P(RadioHidlTest, setCellInfoListRate) {
    LOG(DEBUG) << "setCellInfoListRate";
    serial = GetRandomSerialNumber();

    // TODO(sanketpadawe): RIL crashes with value of rate = 10
    radio->setCellInfoListRate(serial, 10);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "setCellInfoListRate finished";
}

/*
 * Test IRadio.nvReadItem() for the response returned.
 */
TEST_P(RadioHidlTest, nvReadItem) {
    LOG(DEBUG) << "nvReadItem";
    serial = GetRandomSerialNumber();

    radio->nvReadItem(serial, NvItem::LTE_BAND_ENABLE_25);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "nvReadItem finished";
}

/*
 * Test IRadio.nvWriteItem() for the response returned.
 */
TEST_P(RadioHidlTest, nvWriteItem) {
    LOG(DEBUG) << "nvWriteItem";
    serial = GetRandomSerialNumber();
    NvWriteItem item;
    memset(&item, 0, sizeof(item));
    item.value = hidl_string();

    radio->nvWriteItem(serial, item);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "nvWriteItem finished";
}

/*
 * Test IRadio.nvWriteCdmaPrl() for the response returned.
 */
TEST_P(RadioHidlTest, nvWriteCdmaPrl) {
    LOG(DEBUG) << "nvWriteCdmaPrl";
    serial = GetRandomSerialNumber();
    std::vector<uint8_t> prl = {1, 2, 3, 4, 5};

    radio->nvWriteCdmaPrl(serial, hidl_vec<uint8_t>(prl));
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "nvWriteCdmaPrl finished";
}

/*
 * Test IRadio.nvResetConfig() for the response returned.
 */
TEST_P(RadioHidlTest, nvResetConfig) {
    LOG(DEBUG) << "nvResetConfig";
    serial = GetRandomSerialNumber();

    radio->nvResetConfig(serial, ResetNvType::FACTORY_RESET);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "nvResetConfig finished";
}

/*
 * Test IRadio.setUiccSubscription() for the response returned.
 */
TEST_P(RadioHidlTest, setUiccSubscription) {
    LOG(DEBUG) << "setUiccSubscription";
    serial = GetRandomSerialNumber();
    SelectUiccSub item;
    memset(&item, 0, sizeof(item));

    radio->setUiccSubscription(serial, item);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error,
                             {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                              RadioError::MODEM_ERR, RadioError::SUBSCRIPTION_NOT_SUPPORTED},
                             CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setUiccSubscription finished";
}

/*
 * Test IRadio.getHardwareConfig() for the response returned.
 */
TEST_P(RadioHidlTest, getHardwareConfig) {
    LOG(DEBUG) << "getHardwareConfig";
    serial = GetRandomSerialNumber();

    radio->getHardwareConfig(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "getHardwareConfig finished";
}

/*
 * The following test is disabled due to b/64734869
 *
 * Test IRadio.requestShutdown() for the response returned.
 */
TEST_P(RadioHidlTest, DISABLED_requestShutdown) {
    serial = GetRandomSerialNumber();

    radio->requestShutdown(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getRadioCapability() for the response returned.
 */
TEST_P(RadioHidlTest, getRadioCapability) {
    LOG(DEBUG) << "getRadioCapability";
    serial = GetRandomSerialNumber();

    radio->getRadioCapability(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getRadioCapability finished";
}

/*
 * Test IRadio.setRadioCapability() for the response returned.
 */
TEST_P(RadioHidlTest, setRadioCapability) {
    LOG(DEBUG) << "setRadioCapability";
    serial = GetRandomSerialNumber();
    RadioCapability rc;
    memset(&rc, 0, sizeof(rc));
    rc.logicalModemUuid = hidl_string();

    radio->setRadioCapability(serial, rc);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE},
                                     CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setRadioCapability finished";
}

/*
 * Test IRadio.startLceService() for the response returned.
 */
TEST_P(RadioHidlTest, startLceService) {
    LOG(DEBUG) << "startLceService";
    serial = GetRandomSerialNumber();

    radio->startLceService(serial, 5, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    // HAL 1.2 and later use the always-on LCE that relies on indications.
    SKIP_TEST_IF_REQUEST_NOT_SUPPORTED_WITH_HAL_VERSION_AT_LEAST(1_2);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INTERNAL_ERR, RadioError::LCE_NOT_SUPPORTED,
             RadioError::RADIO_NOT_AVAILABLE, RadioError::SIM_ABSENT, RadioError::NONE}));
    }
    LOG(DEBUG) << "startLceService finished";
}

/*
 * Test IRadio.stopLceService() for the response returned.
 */
TEST_P(RadioHidlTest, stopLceService) {
    LOG(DEBUG) << "stopLceService";
    serial = GetRandomSerialNumber();

    radio->stopLceService(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    // HAL 1.2 and later use the always-on LCE that relies on indications.
    SKIP_TEST_IF_REQUEST_NOT_SUPPORTED_WITH_HAL_VERSION_AT_LEAST(1_2);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::LCE_NOT_SUPPORTED,
                                      RadioError::REQUEST_NOT_SUPPORTED, RadioError::SIM_ABSENT}));
    }
    LOG(DEBUG) << "stopLceService finished";
}

/*
 * Test IRadio.pullLceData() for the response returned.
 */
TEST_P(RadioHidlTest, pullLceData) {
    LOG(DEBUG) << "pullLceData";
    serial = GetRandomSerialNumber();

    radio->pullLceData(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    // HAL 1.2 and later use the always-on LCE that relies on indications.
    SKIP_TEST_IF_REQUEST_NOT_SUPPORTED_WITH_HAL_VERSION_AT_LEAST(1_2);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::INTERNAL_ERR,
                                      RadioError::RADIO_NOT_AVAILABLE, RadioError::SIM_ABSENT},
                                     CHECK_OEM_ERROR));
    }
    LOG(DEBUG) << "pullLceData finished";
}

/*
 * Test IRadio.getModemActivityInfo() for the response returned.
 */
TEST_P(RadioHidlTest, getModemActivityInfo) {
    LOG(DEBUG) << "getModemActivityInfo";
    serial = GetRandomSerialNumber();

    radio->getModemActivityInfo(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "getModemActivityInfo finished";
}

/*
 * The following test is disabled due to b/79930549
 *
 * Test IRadio.setAllowedCarriers() for the response returned.
 */
TEST_P(RadioHidlTest, DISABLED_setAllowedCarriers) {
    serial = GetRandomSerialNumber();
    CarrierRestrictions carriers;
    memset(&carriers, 0, sizeof(carriers));
    carriers.allowedCarriers.resize(1);
    carriers.excludedCarriers.resize(0);
    carriers.allowedCarriers[0].mcc = hidl_string();
    carriers.allowedCarriers[0].mnc = hidl_string();
    carriers.allowedCarriers[0].matchType = CarrierMatchType::ALL;
    carriers.allowedCarriers[0].matchData = hidl_string();

    radio->setAllowedCarriers(serial, false, carriers);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }

    if (radioRsp->rspInfo.error == RadioError::NONE) {
        /* Setting to carrier restriction needs some time */
        updateSimCardStatus();
        auto startTime = std::chrono::system_clock::now();
        while (cardStatus.cardState != CardState::RESTRICTED &&
               std::chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now() -
                                                           startTime)
                       .count() < 10) {
            /* Set 2 seconds as interval to check card status */
            sleep(2);
            updateSimCardStatus();
        }
        EXPECT_EQ(CardState::RESTRICTED, cardStatus.cardState);
    }
    sleep(10);

    /** 
     * Another test case of the API to cover to allow carrier.
     * If the API is supported, this is also used to reset to no carrier restriction
     * status for cardStatus. 
     */
    memset(&carriers, 0, sizeof(carriers));
    carriers.allowedCarriers.resize(0);
    carriers.excludedCarriers.resize(0);

    serial = GetRandomSerialNumber();
    radio->setAllowedCarriers(serial, true, carriers);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }

    if (radioRsp->rspInfo.error == RadioError::NONE) {
        /* Resetting back to no carrier restriction needs some time */
        updateSimCardStatus();
        auto startTime = std::chrono::system_clock::now();
        while (cardStatus.cardState == CardState::RESTRICTED &&
               std::chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now() -
                                                           startTime)
                       .count() < 10) {
            /* Set 2 seconds as interval to check card status */
            sleep(2);
            updateSimCardStatus();
        }
        EXPECT_NE(CardState::RESTRICTED, cardStatus.cardState);
        sleep(10);
    }
}

/*
 * Test IRadio.getAllowedCarriers() for the response returned.
 */
TEST_P(RadioHidlTest, getAllowedCarriers) {
    LOG(DEBUG) << "getAllowedCarriers";
    serial = GetRandomSerialNumber();

    radio->getAllowedCarriers(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "getAllowedCarriers finished";
}

/*
 * Test IRadio.sendDeviceState() for the response returned.
 */
TEST_P(RadioHidlTest, sendDeviceState) {
    LOG(DEBUG) << "sendDeviceState";
    serial = GetRandomSerialNumber();

    radio->sendDeviceState(serial, DeviceStateType::POWER_SAVE_MODE, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    std::cout << static_cast<int>(radioRsp->rspInfo.error) << std::endl;

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "sendDeviceState finished";
}

/*
 * Test IRadio.setIndicationFilter() for the response returned.
 */
TEST_P(RadioHidlTest, setIndicationFilter) {
    LOG(DEBUG) << "setIndicationFilter";
    serial = GetRandomSerialNumber();

    radio->setIndicationFilter(serial, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    // setIndicationFilter is deprecated on radio::V1_2 with setIndicationFilter_1_2
    SKIP_TEST_IF_REQUEST_NOT_SUPPORTED_WITH_HAL_VERSION_AT_LEAST(1_2);

    std::cout << static_cast<int>(radioRsp->rspInfo.error) << std::endl;

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "setIndicationFilter finished";
}

/*
 * Test IRadio.setSimCardPower() for the response returned.
 */
TEST_P(RadioHidlTest, setSimCardPower) {
    LOG(DEBUG) << "setSimCardPower";
    serial = GetRandomSerialNumber();

    radio->setSimCardPower(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    // setSimCardPower is deprecated on radio::V1_1 with setSimCardPower_1_1
    SKIP_TEST_IF_REQUEST_NOT_SUPPORTED_WITH_HAL_VERSION_AT_LEAST(1_1);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "setSimCardPower finished";
}
