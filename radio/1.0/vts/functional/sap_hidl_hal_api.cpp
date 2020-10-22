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
#include <sap_hidl_hal_utils.h>

/*
 * Test ISap.connectReq() for the response returned.
 */
TEST_P(SapHidlTest, connectReq) {
    LOG(DEBUG) << "connectReq";
    token = GetRandomSerialNumber();
    int32_t maxMsgSize = 100;

    sap->connectReq(token, maxMsgSize);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    // Modem side need time for connect to finish. Adding a waiting time to prevent
    // disconnect being requested right after connect request.
    sleep(1);
    LOG(DEBUG) << "connectReq finished";
}

/*
 * Test IRadio.disconnectReq() for the response returned
 */
TEST_P(SapHidlTest, disconnectReq) {
    LOG(DEBUG) << "disconnectReq";
    token = GetRandomSerialNumber();

    sap->disconnectReq(token);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);
    LOG(DEBUG) << "disconnectReq finished";
}

/*
 * Test IRadio.apduReq() for the response returned.
 */
TEST_P(SapHidlTest, apduReq) {
    LOG(DEBUG) << "apduReq";
    token = GetRandomSerialNumber();
    SapApduType sapApduType = SapApduType::APDU;
    android::hardware::hidl_vec<uint8_t> command = {};

    sap->apduReq(token, sapApduType, command);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(CheckAnyOfErrors(
            sapCb->sapResultCode,
            {SapResultCode::GENERIC_FAILURE, SapResultCode::CARD_ALREADY_POWERED_OFF,
             SapResultCode::CARD_NOT_ACCESSSIBLE, SapResultCode::CARD_REMOVED,
             SapResultCode::SUCCESS}));
    LOG(DEBUG) << "apduReq finished";
}

/*
 * Test IRadio.transferAtrReq() for the response returned.
 */
TEST_P(SapHidlTest, transferAtrReq) {
    LOG(DEBUG) << "transferAtrReq";
    token = GetRandomSerialNumber();

    sap->transferAtrReq(token);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(CheckAnyOfErrors(sapCb->sapResultCode,
                                 {SapResultCode::GENERIC_FAILURE, SapResultCode::DATA_NOT_AVAILABLE,
                                  SapResultCode::CARD_ALREADY_POWERED_OFF,
                                  SapResultCode::CARD_REMOVED, SapResultCode::SUCCESS}));
    LOG(DEBUG) << "transferAtrReq finished";
}

/*
 * Test IRadio.powerReq() for the response returned.
 */
TEST_P(SapHidlTest, powerReq) {
    LOG(DEBUG) << "powerReq";
    token = GetRandomSerialNumber();
    bool state = true;

    sap->powerReq(token, state);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(
            CheckAnyOfErrors(sapCb->sapResultCode,
                             {SapResultCode::GENERIC_FAILURE, SapResultCode::CARD_NOT_ACCESSSIBLE,
                              SapResultCode::CARD_ALREADY_POWERED_OFF, SapResultCode::CARD_REMOVED,
                              SapResultCode::CARD_ALREADY_POWERED_ON, SapResultCode::SUCCESS}));
    LOG(DEBUG) << "powerReq finished";
}

/*
 * Test IRadio.resetSimReq() for the response returned.
 */
TEST_P(SapHidlTest, resetSimReq) {
    LOG(DEBUG) << "resetSimReq";
    token = GetRandomSerialNumber();

    sap->resetSimReq(token);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(
            CheckAnyOfErrors(sapCb->sapResultCode,
                             {SapResultCode::GENERIC_FAILURE, SapResultCode::CARD_NOT_ACCESSSIBLE,
                              SapResultCode::CARD_ALREADY_POWERED_OFF, SapResultCode::CARD_REMOVED,
                              SapResultCode::SUCCESS}));
    LOG(DEBUG) << "resetSimReq finished";
}

/*
 * Test IRadio.transferCardReaderStatusReq() for the response returned.
 */
TEST_P(SapHidlTest, transferCardReaderStatusReq) {
    LOG(DEBUG) << "transferCardReaderStatusReq";
    token = GetRandomSerialNumber();

    sap->transferCardReaderStatusReq(token);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(CheckAnyOfErrors(sapCb->sapResultCode,
                                 {SapResultCode::GENERIC_FAILURE, SapResultCode::DATA_NOT_AVAILABLE,
                                  SapResultCode::SUCCESS}));
    LOG(DEBUG) << "transferCardReaderStatusReq finished";
}

/*
 * Test IRadio.setTransferProtocolReq() for the response returned.
 */
TEST_P(SapHidlTest, setTransferProtocolReq) {
    LOG(DEBUG) << "setTransferProtocolReq";
    token = GetRandomSerialNumber();
    SapTransferProtocol sapTransferProtocol = SapTransferProtocol::T0;

    sap->setTransferProtocolReq(token, sapTransferProtocol);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseToken, token);

    ASSERT_TRUE(CheckAnyOfErrors(sapCb->sapResultCode,
                                 {SapResultCode::NOT_SUPPORTED, SapResultCode::SUCCESS}));
    LOG(DEBUG) << "setTransferProtocolReq finished";
}
