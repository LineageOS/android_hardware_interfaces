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

#include <radio_hidl_hal_utils_v1_0.h>

using namespace ::android::hardware::radio::V1_0;

/*
 * Test IRadio.getClir() for the response returned.
 */
TEST_P(RadioHidlTest, getClir) {
    serial = GetRandomSerialNumber();

    radio->getClir(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setClir() for the response returned.
 */
TEST_P(RadioHidlTest, setClir) {
    serial = GetRandomSerialNumber();
    int32_t status = 1;

    radio->setClir(serial, status);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.getFacilityLockForApp() for the response returned.
 */
TEST_P(RadioHidlTest, getFacilityLockForApp) {
    serial = GetRandomSerialNumber();
    std::string facility = "";
    std::string password = "";
    int32_t serviceClass = 1;
    std::string appId = "";

    radio->getFacilityLockForApp(serial, facility, password, serviceClass, appId);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setFacilityLockForApp() for the response returned.
 */
TEST_P(RadioHidlTest, setFacilityLockForApp) {
    serial = GetRandomSerialNumber();
    std::string facility = "";
    bool lockState = false;
    std::string password = "";
    int32_t serviceClass = 1;
    std::string appId = "";

    radio->setFacilityLockForApp(serial, facility, lockState, password, serviceClass, appId);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setBarringPassword() for the response returned.
 */
TEST_P(RadioHidlTest, setBarringPassword) {
    serial = GetRandomSerialNumber();
    std::string facility = "";
    std::string oldPassword = "";
    std::string newPassword = "";

    radio->setBarringPassword(serial, facility, oldPassword, newPassword);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::FDN_CHECK_FAILURE,
                                      RadioError::INVALID_ARGUMENTS, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getClip() for the response returned.
 */
TEST_P(RadioHidlTest, getClip) {
    serial = GetRandomSerialNumber();

    radio->getClip(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setSuppServiceNotifications() for the response returned.
 */
TEST_P(RadioHidlTest, setSuppServiceNotifications) {
    serial = GetRandomSerialNumber();
    bool enable = false;

    radio->setSuppServiceNotifications(serial, enable);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadio.requestIsimAuthentication() for the response returned.
 */
TEST_P(RadioHidlTest, requestIsimAuthentication) {
    serial = GetRandomSerialNumber();
    std::string challenge = "";

    radio->requestIsimAuthentication(serial, challenge);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::NO_MEMORY, RadioError::SYSTEM_ERR, RadioError::REQUEST_NOT_SUPPORTED,
             RadioError::CANCELLED}));
    }
}

/*
 * Test IRadio.getImsRegistrationState() for the response returned.
 */
TEST_P(RadioHidlTest, getImsRegistrationState) {
    serial = GetRandomSerialNumber();

    radio->getImsRegistrationState(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::MODEM_ERR, RadioError::INVALID_MODEM_STATE},
            CHECK_GENERAL_ERROR));
    }
}
