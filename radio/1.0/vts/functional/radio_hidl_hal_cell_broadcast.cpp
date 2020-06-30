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
#include <radio_hidl_hal_utils_v1_0.h>

using namespace ::android::hardware::radio::V1_0;

/*
 * Test IRadio.setGsmBroadcastConfig() for the response returned.
 */
TEST_P(RadioHidlTest, setGsmBroadcastConfig) {
    LOG(DEBUG) << "setGsmBroadcastConfig";
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

    android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo> gsmBroadcastSmsConfigsInfoList = {
        gbSmsConfig1, gbSmsConfig2, gbSmsConfig3, gbSmsConfig4, gbSmsConfig5};

    radio->setGsmBroadcastConfig(serial, gsmBroadcastSmsConfigsInfoList);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                      RadioError::INVALID_MODEM_STATE, RadioError::INVALID_STATE},
                                     CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setGsmBroadcastConfig finished";
}

/*
 * Test IRadio.getGsmBroadcastConfig() for the response returned.
 */
TEST_P(RadioHidlTest, getGsmBroadcastConfig) {
    LOG(DEBUG) << "getGsmBroadcastConfig";
    serial = GetRandomSerialNumber();

    radio->getGsmBroadcastConfig(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::INVALID_MODEM_STATE, RadioError::INVALID_STATE},
            CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "getGsmBroadcastConfig finished";
}

/*
 * Test IRadio.setCdmaBroadcastConfig() for the response returned.
 */
TEST_P(RadioHidlTest, setCdmaBroadcastConfig) {
    LOG(DEBUG) << "setCdmaBroadcastConfig";
    serial = GetRandomSerialNumber();

    CdmaBroadcastSmsConfigInfo cbSmsConfig;
    cbSmsConfig.serviceCategory = 4096;
    cbSmsConfig.language = 1;
    cbSmsConfig.selected = true;

    android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo> cdmaBroadcastSmsConfigInfoList = {
        cbSmsConfig};

    radio->setCdmaBroadcastConfig(serial, cdmaBroadcastSmsConfigInfoList);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_MODEM_STATE},
                                     CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setCdmaBroadcastConfig finished";
}

/*
 * Test IRadio.getCdmaBroadcastConfig() for the response returned.
 */
TEST_P(RadioHidlTest, getCdmaBroadcastConfig) {
    LOG(DEBUG) << "getCdmaBroadcastConfig";
    serial = GetRandomSerialNumber();

    radio->getCdmaBroadcastConfig(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "getCdmaBroadcastConfig finished";
}

/*
 * Test IRadio.setCdmaBroadcastActivation() for the response returned.
 */
TEST_P(RadioHidlTest, setCdmaBroadcastActivation) {
    LOG(DEBUG) << "setCdmaBroadcastActivation";
    serial = GetRandomSerialNumber();
    bool activate = false;

    radio->setCdmaBroadcastActivation(serial, activate);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS},
                                     CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setCdmaBroadcastActivation finished";
}

/*
 * Test IRadio.setGsmBroadcastActivation() for the response returned.
 */
TEST_P(RadioHidlTest, setGsmBroadcastActivation) {
    LOG(DEBUG) << "setGsmBroadcastActivation";
    serial = GetRandomSerialNumber();
    bool activate = false;

    radio->setGsmBroadcastActivation(serial, activate);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::INVALID_MODEM_STATE,
             RadioError::INVALID_STATE, RadioError::OPERATION_NOT_ALLOWED},
            CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "setGsmBroadcastActivation finished";
}
