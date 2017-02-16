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
 * Test IRadio.getIccCardStatus() for the response returned.
 */
TEST_F(RadioHidlTest, getIccCardStatus) {
    radio->getIccCardStatus(1);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(1, radioRsp->rspInfo.serial);
    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::NONE);

    EXPECT_LE(radioRsp->cardStatus.applications.size(), (unsigned int) RadioConst::CARD_MAX_APPS);
    EXPECT_LT(radioRsp->cardStatus.gsmUmtsSubscriptionAppIndex, (int) RadioConst::CARD_MAX_APPS);
    EXPECT_LT(radioRsp->cardStatus.cdmaSubscriptionAppIndex, (int) RadioConst::CARD_MAX_APPS);
    EXPECT_LT(radioRsp->cardStatus.imsSubscriptionAppIndex, (int) RadioConst::CARD_MAX_APPS);
}

/*
 * Test IRadio.supplyIccPinForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPinForApp) {
    radio->supplyIccPinForApp(2, hidl_string("test1"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(2, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.supplyIccPukForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPukForApp) {
    radio->supplyIccPukForApp(3, hidl_string("test1"), hidl_string("test2"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(3, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.supplyIccPin2ForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPin2ForApp) {
    radio->supplyIccPin2ForApp(4, hidl_string("test1"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(4, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.supplyIccPuk2ForApp() for the response returned.
 */
TEST_F(RadioHidlTest, supplyIccPuk2ForApp) {
    radio->supplyIccPuk2ForApp(5, hidl_string("test1"), hidl_string("test2"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(5, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.changeIccPinForApp() for the response returned.
 */
TEST_F(RadioHidlTest, changeIccPinForApp) {
    radio->changeIccPinForApp(6, hidl_string("test1"), hidl_string("test2"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(6, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}

/*
 * Test IRadio.changeIccPin2ForApp() for the response returned.
 */
TEST_F(RadioHidlTest, changeIccPin2ForApp) {
    radio->changeIccPin2ForApp(7, hidl_string("test1"), hidl_string("test2"), hidl_string());
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(7, radioRsp->rspInfo.serial);

    EXPECT_EQ(radioRsp->rspInfo.error, RadioError::PASSWORD_INCORRECT);
}
