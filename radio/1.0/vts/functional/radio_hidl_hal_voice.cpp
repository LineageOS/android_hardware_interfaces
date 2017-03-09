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

#include<radio_hidl_hal_utils.h>

/*
 * Test IRadio.getCurrentCalls() for the response returned.
 */
TEST_F(RadioHidlTest, getCurrentCalls) {
    int serial = 1;

    radio->getCurrentCalls(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::NONE
                || radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS);
    }
}

/*
 * Test IRadio.dial() for the response returned.
 */
TEST_F(RadioHidlTest, dial) {
    int serial = 1;

    Dial dialInfo;
    memset(&dialInfo, 0, sizeof(dialInfo));
    dialInfo.address = hidl_string("123456789");

    radio->dial(serial, dialInfo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.hangup() for the response returned.
 */
TEST_F(RadioHidlTest, hangup) {
    int serial = 1;

    radio->hangup(serial, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.hangupWaitingOrBackground() for the response returned.
 */
TEST_F(RadioHidlTest, hangupWaitingOrBackground) {
    int serial = 1;

    radio->hangupWaitingOrBackground(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.hangupForegroundResumeBackground() for the response returned.
 */
TEST_F(RadioHidlTest, hangupForegroundResumeBackground) {
    int serial = 1;

    radio->hangupForegroundResumeBackground(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.switchWaitingOrHoldingAndActive() for the response returned.
 */
TEST_F(RadioHidlTest, switchWaitingOrHoldingAndActive) {
    int serial = 1;

    radio->switchWaitingOrHoldingAndActive(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.conference() for the response returned.
 */
TEST_F(RadioHidlTest, conference) {
    int serial = 1;

    radio->conference(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.rejectCall() for the response returned.
 */
TEST_F(RadioHidlTest, rejectCall) {
    int serial = 1;

    radio->rejectCall(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.getLastCallFailCause() for the response returned.
 */
TEST_F(RadioHidlTest, getLastCallFailCause) {
    int serial = 1;

    radio->getLastCallFailCause(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::NONE);
    }
}

/*
 * Test IRadio.sendUssd() for the response returned.
 */
TEST_F(RadioHidlTest, sendUssd) {
    int serial = 1;
    radio->sendUssd(serial, hidl_string("test"));
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.cancelPendingUssd() for the response returned.
 */
TEST_F(RadioHidlTest, cancelPendingUssd) {
    int serial = 1;

    radio->cancelPendingUssd(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.getCallForwardStatus() for the response returned.
 */
TEST_F(RadioHidlTest, getCallForwardStatus) {
    int serial = 1;
    CallForwardInfo callInfo;
    memset(&callInfo, 0, sizeof(callInfo));
    callInfo.number = hidl_string();

    radio->getCallForwardStatus(serial, callInfo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.setCallForward() for the response returned.
 */
TEST_F(RadioHidlTest, setCallForward) {
    int serial = 1;
    CallForwardInfo callInfo;
    memset(&callInfo, 0, sizeof(callInfo));
    callInfo.number = hidl_string();

    radio->setCallForward(serial, callInfo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.getCallWaiting() for the response returned.
 */
TEST_F(RadioHidlTest, getCallWaiting) {
    int serial = 1;

    radio->getCallWaiting(serial, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::NONE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.setCallWaiting() for the response returned.
 */
TEST_F(RadioHidlTest, setCallWaiting) {
    int serial = 1;

    radio->setCallWaiting(serial, true, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.acceptCall() for the response returned.
 */
TEST_F(RadioHidlTest, acceptCall) {
    int serial = 1;

    radio->acceptCall(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.separateConnection() for the response returned.
 */
TEST_F(RadioHidlTest, separateConnection) {
    int serial = 1;

    radio->separateConnection(serial, 1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.explicitCallTransfer() for the response returned.
 */
TEST_F(RadioHidlTest, explicitCallTransfer) {
    int serial = 1;

    radio->explicitCallTransfer(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.sendDtmf() for the response returned.
 */
TEST_F(RadioHidlTest, sendDtmf) {
    int serial = 1;

    radio->sendDtmf(serial, "1");
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::NO_RESOURCES
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR);
    }
}

/*
 * Test IRadio.startDtmf() for the response returned.
 */
TEST_F(RadioHidlTest, startDtmf) {
    int serial = 1;

    radio->startDtmf(serial, "1");
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.stopDtmf() for the response returned.
 */
TEST_F(RadioHidlTest, stopDtmf) {
    int serial = 1;

    radio->stopDtmf(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR);
    }
}

/*
 * Test IRadio.setMute() for the response returned.
 */
TEST_F(RadioHidlTest, setMute) {
    int serial = 1;

    radio->setMute(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::NONE);
    }
}

/*
 * Test IRadio.getMute() for the response returned.
 */
TEST_F(RadioHidlTest, getMute) {
    int serial = 1;

    radio->getMute(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::NONE);
    }
}

/*
 * Test IRadio.sendBurstDtmf() for the response returned.
 */
TEST_F(RadioHidlTest, sendBurstDtmf) {
    int serial = 1;

    radio->sendBurstDtmf(serial, "1", 0, 0);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp->rspInfo.error == RadioError::INVALID_ARGUMENTS
                || radioRsp->rspInfo.error == RadioError::SYSTEM_ERR
                || radioRsp->rspInfo.error == RadioError::MODEM_ERR
                || radioRsp->rspInfo.error == RadioError::INTERNAL_ERR
                || radioRsp->rspInfo.error == RadioError::INVALID_STATE);
    }
}