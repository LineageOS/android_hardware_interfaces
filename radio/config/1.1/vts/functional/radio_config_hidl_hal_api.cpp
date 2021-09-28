/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <radio_config_hidl_hal_utils.h>

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

/*
 * Test IRadioConfig.getModemsConfig()
 */
TEST_P(RadioConfigHidlTest, getModemsConfig) {
    serial = GetRandomSerialNumber();
    Return<void> res = radioConfig->getModemsConfig(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("getModemsConfig, rspInfo.error = %s\n", toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(radioConfigRsp->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioConfig.setModemsConfig()
 */
TEST_P(RadioConfigHidlTest, setModemsConfig_invalidArgument) {
    serial = GetRandomSerialNumber();
    ModemsConfig* mConfig = new ModemsConfig();
    Return<void> res = radioConfig->setModemsConfig(serial, *mConfig);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("setModemsConfig, rspInfo.error = %s\n", toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(
        CheckAnyOfErrors(radioConfigRsp->rspInfo.error,
                         {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioConfig.setModemsConfig()
 */
TEST_P(RadioConfigHidlTest, setModemsConfig_goodRequest) {
    serial = GetRandomSerialNumber();
    ModemsConfig* mConfig = new ModemsConfig();
    if (isSsSsEnabled()) {
        mConfig->numOfLiveModems = 1;
    } else if (isDsDsEnabled()) {
        mConfig->numOfLiveModems = 2;
    } else if (isTsTsEnabled()) {
        mConfig->numOfLiveModems = 3;
    } else {
        ALOGI("Skipping setModemsConfig_goodRequest, unsupported multisim number");
        return;
    }
    Return<void> res = radioConfig->setModemsConfig(serial, *mConfig);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("setModemsConfig, rspInfo.error = %s\n", toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(radioConfigRsp->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioConfig.getPhoneCapability()
 */
TEST_P(RadioConfigHidlTest, getPhoneCapability) {
    serial = GetRandomSerialNumber();
    Return<void> res = radioConfig->getPhoneCapability(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("getPhoneCapability, rspInfo.error = %s\n",
          toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            radioConfigRsp->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));

    if (radioConfigRsp->rspInfo.error == RadioError ::NONE) {
        // maxActiveData should be greater than or equal to maxActiveInternetData.
        EXPECT_GE(radioConfigRsp->phoneCap.maxActiveData,
                  radioConfigRsp->phoneCap.maxActiveInternetData);
        // maxActiveData and maxActiveInternetData should be 0 or positive numbers.
        EXPECT_GE(radioConfigRsp->phoneCap.maxActiveInternetData, 0);
    }
}

/*
 * Test IRadioConfig.getPhoneCapability()
 */
TEST_P(RadioConfigHidlTest, setPreferredDataModem) {
    serial = GetRandomSerialNumber();
    Return<void> res = radioConfig->getPhoneCapability(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("getPhoneCapability, rspInfo.error = %s\n",
          toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            radioConfigRsp->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));

    if (radioConfigRsp->rspInfo.error != RadioError ::NONE) {
        return;
    }

    if (radioConfigRsp->phoneCap.logicalModemList.size() == 0) {
        return;
    }

    // We get phoneCapability. send setPreferredDataModem command
    serial = GetRandomSerialNumber();
    uint8_t modemId = radioConfigRsp->phoneCap.logicalModemList[0].modemId;
    res = radioConfig->setPreferredDataModem(serial, modemId);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("setPreferredDataModem, rspInfo.error = %s\n",
          toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            radioConfigRsp->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));
}

/*
 * Test IRadioConfig.getPhoneCapability()
 */
TEST_P(RadioConfigHidlTest, setPreferredDataModem_invalidArgument) {
    serial = GetRandomSerialNumber();
    uint8_t modemId = -1;
    Return<void> res = radioConfig->setPreferredDataModem(serial, modemId);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("setPreferredDataModem, rspInfo.error = %s\n",
          toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(radioConfigRsp->rspInfo.error,
                                 {RadioError::INVALID_ARGUMENTS, RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::INTERNAL_ERR}));
}
