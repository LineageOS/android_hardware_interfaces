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
 * Test IRadio.sendEnvelope() for the response returned.
 */
TEST_P(RadioHidlTest, sendEnvelope) {
    LOG(DEBUG) << "sendEnvelope";
    serial = GetRandomSerialNumber();

    // Test with sending empty string
    std::string content = "";

    radio->sendEnvelope(serial, content);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                      RadioError::MODEM_ERR, RadioError::SIM_ABSENT},
                                     CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "sendEnvelope finished";
}

/*
 * Test IRadio.sendTerminalResponseToSim() for the response returned.
 */
TEST_P(RadioHidlTest, sendTerminalResponseToSim) {
    LOG(DEBUG) << "sendTerminalResponseToSim";
    serial = GetRandomSerialNumber();

    // Test with sending empty string
    std::string commandResponse = "";

    radio->sendTerminalResponseToSim(serial, commandResponse);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "sendTerminalResponseToSim finished";
}

/*
 * Test IRadio.handleStkCallSetupRequestFromSim() for the response returned.
 */
TEST_P(RadioHidlTest, handleStkCallSetupRequestFromSim) {
    LOG(DEBUG) << "handleStkCallSetupRequestFromSim";
    serial = GetRandomSerialNumber();
    bool accept = false;

    radio->handleStkCallSetupRequestFromSim(serial, accept);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                      RadioError::MODEM_ERR, RadioError::SIM_ABSENT},
                                     CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "handleStkCallSetupRequestFromSim finished";
}

/*
 * Test IRadio.reportStkServiceIsRunning() for the response returned.
 */
TEST_P(RadioHidlTest, reportStkServiceIsRunning) {
    LOG(DEBUG) << "reportStkServiceIsRunning";
    serial = GetRandomSerialNumber();

    radio->reportStkServiceIsRunning(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "reportStkServiceIsRunning finished";
}

/*
 * Test IRadio.sendEnvelopeWithStatus() for the response returned with empty
 * string.
 */
TEST_P(RadioHidlTest, sendEnvelopeWithStatus) {
    LOG(DEBUG) << "sendEnvelopeWithStatus";
    serial = GetRandomSerialNumber();

    // Test with sending empty string
    std::string contents = "";

    radio->sendEnvelopeWithStatus(serial, contents);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::MODEM_ERR, RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "sendEnvelopeWithStatus finished";
}
