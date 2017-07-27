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

#include <radio_hidl_hal_utils_v1_1.h>

/*
 * Test IRadio.setSimCardPower() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, setSimCardPower_1_1) {
    int serial = GetRandomSerialNumber();

    radio_v1_1->setSimCardPower_1_1(serial, CardPowerState::POWER_DOWN);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_1->rspInfo.error == RadioError::NONE ||
                    radioRsp_v1_1->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED ||
                    radioRsp_v1_1->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_1->rspInfo.error == RadioError::RADIO_NOT_AVAILABLE);
    }
}

/*
 * Test IRadio.startNetworkScan() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, startNetworkScan) {
    int serial = GetRandomSerialNumber();

    NetworkScanRequest request;
    request.type = ScanType::ONE_SHOT;
    request.interval = 60;
    RadioAccessSpecifier specifier;
    specifier.radioAccessNetwork = RadioAccessNetworks::GERAN;
    specifier.geranBands.resize(2);
    specifier.geranBands[0] = GeranBands::BAND_450;
    specifier.geranBands[1] = GeranBands::BAND_480;
    specifier.channels.resize(2);
    specifier.channels[0] = 1;
    specifier.channels[1] = 2;
    request.specifiers.resize(1);
    request.specifiers[0] = specifier;

    radio_v1_1->startNetworkScan(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_1->rspInfo.error == RadioError::NONE ||
                    radioRsp_v1_1->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_1->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_1->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}

/*
 * Test IRadio.startNetworkScan() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, startNetworkScan_InvalidArgument) {
    int serial = GetRandomSerialNumber();

    NetworkScanRequest request;
    request.type = ScanType::ONE_SHOT;
    request.interval = 60;

    radio_v1_1->startNetworkScan(serial, request);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_1->rspInfo.error == RadioError::INVALID_ARGUMENTS ||
                    radioRsp_v1_1->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_1->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}

/*
 * Test IRadio.stopNetworkScan() for the response returned.
 */
TEST_F(RadioHidlTest_v1_1, stopNetworkScan) {
    int serial = GetRandomSerialNumber();

    radio_v1_1->stopNetworkScan(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(radioRsp_v1_1->rspInfo.error == RadioError::NONE ||
                    radioRsp_v1_1->rspInfo.error == RadioError::SIM_ABSENT ||
                    radioRsp_v1_1->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED);
    }
}