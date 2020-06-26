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

/*
 * Test IRadio.getCurrentCalls() for the response returned.
 */
TEST_P(RadioHidlTest, getCurrentCalls) {
    LOG(DEBUG) << "getCurrentCalls";
    serial = GetRandomSerialNumber();

    radio->getCurrentCalls(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getCurrentCalls finished";
}

/*
 * Test IRadio.dial() for the response returned.
 */
TEST_P(RadioHidlTest, dial) {
    LOG(DEBUG) << "dial";
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
    LOG(DEBUG) << "dial finished";
}

/*
 * Test IRadio.hangup() for the response returned.
 */
TEST_P(RadioHidlTest, hangup) {
    LOG(DEBUG) << "hangup";
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
    LOG(DEBUG) << "hangup finished";
}

/*
 * Test IRadio.hangupWaitingOrBackground() for the response returned.
 */
TEST_P(RadioHidlTest, hangupWaitingOrBackground) {
    LOG(DEBUG) << "hangupWaitingOrBackground";
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
    LOG(DEBUG) << "hangupWaitingOrBackground finished";
}

/*
 * Test IRadio.hangupForegroundResumeBackground() for the response returned.
 */
TEST_P(RadioHidlTest, hangupForegroundResumeBackground) {
    LOG(DEBUG) << "hangupForegroundResumeBackground";
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
    LOG(DEBUG) << "hangupForegroundResumeBackground finished";
}

/*
 * Test IRadio.switchWaitingOrHoldingAndActive() for the response returned.
 */
TEST_P(RadioHidlTest, switchWaitingOrHoldingAndActive) {
    LOG(DEBUG) << "switchWaitingOrHoldingAndActive";
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
    LOG(DEBUG) << "switchWaitingOrHoldingAndActive finished";
}

/*
 * Test IRadio.conference() for the response returned.
 */
TEST_P(RadioHidlTest, conference) {
    LOG(DEBUG) << "conference";
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
    LOG(DEBUG) << "conference finished";
}

/*
 * Test IRadio.rejectCall() for the response returned.
 */
TEST_P(RadioHidlTest, rejectCall) {
    LOG(DEBUG) << "rejectCall";
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
    LOG(DEBUG) << "rejectCall finished";
}

/*
 * Test IRadio.getLastCallFailCause() for the response returned.
 */
TEST_P(RadioHidlTest, getLastCallFailCause) {
    LOG(DEBUG) << "getLastCallFailCause";
    serial = GetRandomSerialNumber();

    radio->getLastCallFailCause(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp->rspInfo.error, {RadioError::NONE}, CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "getLastCallFailCause finished";
}

/*
 * Test IRadio.sendUssd() for the response returned.
 */
TEST_P(RadioHidlTest, sendUssd) {
    LOG(DEBUG) << "sendUssd";
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
    LOG(DEBUG) << "sendUssd finished";
}

/*
 * Test IRadio.cancelPendingUssd() for the response returned.
 */
TEST_P(RadioHidlTest, cancelPendingUssd) {
    LOG(DEBUG) << "cancelPendingUssd";
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
    LOG(DEBUG) << "cancelPendingUssd finished";
}

/*
 * Test IRadio.getCallForwardStatus() for the response returned.
 */
TEST_P(RadioHidlTest, getCallForwardStatus) {
    LOG(DEBUG) << "getCallForwardStatus";
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
    LOG(DEBUG) << "getCallForwardStatus finished";
}

/*
 * Test IRadio.setCallForward() for the response returned.
 */
TEST_P(RadioHidlTest, setCallForward) {
    LOG(DEBUG) << "setCallForward";
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
    LOG(DEBUG) << "setCallForward finished";
}

/*
 * Test IRadio.getCallWaiting() for the response returned.
 */
TEST_P(RadioHidlTest, getCallWaiting) {
    LOG(DEBUG) << "getCallWaiting";
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
    LOG(DEBUG) << "getCallWaiting finished";
}

/*
 * Test IRadio.setCallWaiting() for the response returned.
 */
TEST_P(RadioHidlTest, setCallWaiting) {
    LOG(DEBUG) << "setCallWaiting";
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
    LOG(DEBUG) << "setCallWaiting finished";
}

/*
 * Test IRadio.acceptCall() for the response returned.
 */
TEST_P(RadioHidlTest, acceptCall) {
    LOG(DEBUG) << "acceptCall";
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
    LOG(DEBUG) << "acceptCall finished";
}

/*
 * Test IRadio.separateConnection() for the response returned.
 */
TEST_P(RadioHidlTest, separateConnection) {
    LOG(DEBUG) << "separateConnection";
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
    LOG(DEBUG) << "separateConnection finished";
}

/*
 * Test IRadio.explicitCallTransfer() for the response returned.
 */
TEST_P(RadioHidlTest, explicitCallTransfer) {
    LOG(DEBUG) << "explicitCallTransfer";
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
    LOG(DEBUG) << "explicitCallTransfer finished";
}

/*
 * Test IRadio.sendCDMAFeatureCode() for the response returned.
 */
TEST_P(RadioHidlTest, sendCDMAFeatureCode) {
    LOG(DEBUG) << "sendCDMAFeatureCode";
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
    LOG(DEBUG) << "sendCDMAFeatureCode finished";
}

/*
 * Test IRadio.sendDtmf() for the response returned.
 */
TEST_P(RadioHidlTest, sendDtmf) {
    LOG(DEBUG) << "sendDtmf";
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
    LOG(DEBUG) << "sendDtmf finished";
}

/*
 * Test IRadio.startDtmf() for the response returned.
 */
TEST_P(RadioHidlTest, startDtmf) {
    LOG(DEBUG) << "startDtmf";
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
    LOG(DEBUG) << "startDtmf finished";
}

/*
 * Test IRadio.stopDtmf() for the response returned.
 */
TEST_P(RadioHidlTest, stopDtmf) {
    LOG(DEBUG) << "stopDtmf";
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
    LOG(DEBUG) << "stopDtmf finished";
}

/*
 * Test IRadio.setMute() for the response returned.
 */
TEST_P(RadioHidlTest, setMute) {
    LOG(DEBUG) << "setMute";
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
    LOG(DEBUG) << "setMute finished";
}

/*
 * Test IRadio.getMute() for the response returned.
 */
TEST_P(RadioHidlTest, getMute) {
    LOG(DEBUG) << "getMute";
    serial = GetRandomSerialNumber();

    radio->getMute(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "getMute finished";
}

/*
 * Test IRadio.sendBurstDtmf() for the response returned.
 */
TEST_P(RadioHidlTest, sendBurstDtmf) {
    LOG(DEBUG) << "sendBurstDtmf";
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
    LOG(DEBUG) << "sendBurstDtmf finished";
}
