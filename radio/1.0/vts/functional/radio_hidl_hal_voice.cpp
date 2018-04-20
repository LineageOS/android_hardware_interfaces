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

/*
 * Test IRadio.getCurrentCalls() for the response returned.
 */
TEST_F(RadioHidlTest, getCurrentCalls) {
    serial = GetRandomSerialNumber();

    radio->getCurrentCalls(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.dial() for the response returned.
 */
TEST_F(RadioHidlTest, dial) {
    serial = GetRandomSerialNumber();

    Dial dialInfo;
    memset(&dialInfo, 0, sizeof(dialInfo));
    dialInfo.address = hidl_string("123456789");

    radio->dial(serial, dialInfo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::CANCELLED, RadioError::DEVICE_IN_USE, RadioError::FDN_CHECK_FAILURE,
             RadioError::INVALID_ARGUMENTS, RadioError::INVALID_CALL_ID,
             RadioError::INVALID_MODEM_STATE, RadioError::INVALID_STATE, RadioError::MODEM_ERR,
             RadioError::NO_NETWORK_FOUND, RadioError::NO_SUBSCRIPTION,
             RadioError::OPERATION_NOT_ALLOWED},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.hangup() for the response returned.
 */
TEST_F(RadioHidlTest, hangup) {
    serial = GetRandomSerialNumber();

    radio->hangup(serial, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::MODEM_ERR},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.hangupWaitingOrBackground() for the response returned.
 */
TEST_F(RadioHidlTest, hangupWaitingOrBackground) {
    serial = GetRandomSerialNumber();

    radio->hangupWaitingOrBackground(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_STATE, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.hangupForegroundResumeBackground() for the response returned.
 */
TEST_F(RadioHidlTest, hangupForegroundResumeBackground) {
    serial = GetRandomSerialNumber();

    radio->hangupForegroundResumeBackground(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_STATE, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.switchWaitingOrHoldingAndActive() for the response returned.
 */
TEST_F(RadioHidlTest, switchWaitingOrHoldingAndActive) {
    serial = GetRandomSerialNumber();

    radio->switchWaitingOrHoldingAndActive(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_STATE, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.conference() for the response returned.
 */
TEST_F(RadioHidlTest, conference) {
    serial = GetRandomSerialNumber();

    radio->conference(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_STATE, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.rejectCall() for the response returned.
 */
TEST_F(RadioHidlTest, rejectCall) {
    serial = GetRandomSerialNumber();

    radio->rejectCall(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_STATE, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getLastCallFailCause() for the response returned.
 */
TEST_F(RadioHidlTest, getLastCallFailCause) {
    serial = GetRandomSerialNumber();

    radio->getLastCallFailCause(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.sendUssd() for the response returned.
 */
TEST_F(RadioHidlTest, sendUssd) {
    serial = GetRandomSerialNumber();
    radio->sendUssd(serial, hidl_string("test"));
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::MODEM_ERR},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.cancelPendingUssd() for the response returned.
 */
TEST_F(RadioHidlTest, cancelPendingUssd) {
    serial = GetRandomSerialNumber();

    radio->cancelPendingUssd(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error,
                             {RadioError::NONE, RadioError::INVALID_STATE, RadioError::MODEM_ERR},
                             CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getCallForwardStatus() for the response returned.
 */
TEST_F(RadioHidlTest, getCallForwardStatus) {
    serial = GetRandomSerialNumber();
    CallForwardInfo callInfo;
    memset(&callInfo, 0, sizeof(callInfo));
    callInfo.number = hidl_string();

    radio->getCallForwardStatus(serial, callInfo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::MODEM_ERR},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setCallForward() for the response returned.
 */
TEST_F(RadioHidlTest, setCallForward) {
    serial = GetRandomSerialNumber();
    CallForwardInfo callInfo;
    memset(&callInfo, 0, sizeof(callInfo));
    callInfo.number = hidl_string();

    radio->setCallForward(serial, callInfo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::MODEM_ERR},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getCallWaiting() for the response returned.
 */
TEST_F(RadioHidlTest, getCallWaiting) {
    serial = GetRandomSerialNumber();

    radio->getCallWaiting(serial, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::MODEM_ERR},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setCallWaiting() for the response returned.
 */
TEST_F(RadioHidlTest, setCallWaiting) {
    serial = GetRandomSerialNumber();

    radio->setCallWaiting(serial, true, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::MODEM_ERR},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.acceptCall() for the response returned.
 */
TEST_F(RadioHidlTest, acceptCall) {
    serial = GetRandomSerialNumber();

    radio->acceptCall(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_STATE, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.separateConnection() for the response returned.
 */
TEST_F(RadioHidlTest, separateConnection) {
    serial = GetRandomSerialNumber();

    radio->separateConnection(serial, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::MODEM_ERR},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.explicitCallTransfer() for the response returned.
 */
TEST_F(RadioHidlTest, explicitCallTransfer) {
    serial = GetRandomSerialNumber();

    radio->explicitCallTransfer(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_STATE, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.sendCDMAFeatureCode() for the response returned.
 */
TEST_F(RadioHidlTest, sendCDMAFeatureCode) {
    serial = GetRandomSerialNumber();

    radio->sendCDMAFeatureCode(serial, hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                      RadioError::INVALID_CALL_ID, RadioError::INVALID_MODEM_STATE,
                                      RadioError::MODEM_ERR, RadioError::OPERATION_NOT_ALLOWED},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.sendDtmf() for the response returned.
 */
TEST_F(RadioHidlTest, sendDtmf) {
    serial = GetRandomSerialNumber();

    radio->sendDtmf(serial, "1");
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::INVALID_CALL_ID,
             RadioError::INVALID_MODEM_STATE, RadioError::MODEM_ERR},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.startDtmf() for the response returned.
 */
TEST_F(RadioHidlTest, startDtmf) {
    serial = GetRandomSerialNumber();

    radio->startDtmf(serial, "1");
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::INVALID_CALL_ID,
             RadioError::INVALID_MODEM_STATE, RadioError::MODEM_ERR},
            CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.stopDtmf() for the response returned.
 */
TEST_F(RadioHidlTest, stopDtmf) {
    serial = GetRandomSerialNumber();

    radio->stopDtmf(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_CALL_ID,
                                      RadioError::INVALID_MODEM_STATE, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setMute() for the response returned.
 */
TEST_F(RadioHidlTest, setMute) {
    serial = GetRandomSerialNumber();

    radio->setMute(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getMute() for the response returned.
 */
TEST_F(RadioHidlTest, getMute) {
    serial = GetRandomSerialNumber();

    radio->getMute(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
}

/*
 * Test IRadio.sendBurstDtmf() for the response returned.
 */
TEST_F(RadioHidlTest, sendBurstDtmf) {
    serial = GetRandomSerialNumber();

    radio->sendBurstDtmf(serial, "1", 0, 0);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE,
                                      RadioError::MODEM_ERR, RadioError::OPERATION_NOT_ALLOWED},
                                     CHECK_GENERAL_ERROR));
    }
}
