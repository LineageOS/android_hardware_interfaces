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
TEST_F(RadioConfigHidlTest, getModemsConfig) {
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
TEST_F(RadioConfigHidlTest, setModemsConfig_invalidArgument) {
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
TEST_F(RadioConfigHidlTest, setModemsConfig_goodRequest) {
    serial = GetRandomSerialNumber();
    ModemsConfig* mConfig = new ModemsConfig();
    mConfig->numOfLiveModems = 1;
    Return<void> res = radioConfig->setModemsConfig(serial, *mConfig);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("setModemsConfig, rspInfo.error = %s\n", toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(radioConfigRsp->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}
