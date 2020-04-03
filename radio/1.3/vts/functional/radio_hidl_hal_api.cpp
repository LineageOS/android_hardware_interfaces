/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_3.h>
#include <vector>
#include "VtsCoreUtil.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

/*
 * Test IRadio.enableMddem() for the response returned.
 */
TEST_P(RadioHidlTest_v1_3, enableModem) {
    serial = GetRandomSerialNumber();

    bool isMultiSimEnabled =
            testing::checkSubstringInCommandOutput("getprop persist.radio.multisim.config",
                                                   "dsds") ||
            testing::checkSubstringInCommandOutput("getprop persist.radio.multisim.config", "tsts");
    if (!isMultiSimEnabled) {
        ALOGI("enableModem, no need to test in single SIM mode");
        return;
    }

    bool responseToggle = radioRsp_v1_3->enableModemResponseToggle;
    Return<void> res = radio_v1_3->enableModem(serial, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_3->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_3->rspInfo.serial);
    ALOGI("getModemStackStatus, rspInfo.error = %s\n",
          toString(radioRsp_v1_3->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_3->rspInfo.error,
                                 {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::MODEM_ERR, RadioError::INVALID_STATE}));

    // checking if getModemStackStatus returns true, as modem was enabled above
    if (RadioError::NONE == radioRsp_v1_3->rspInfo.error) {
        // wait until modem enabling is finished
        while (responseToggle == radioRsp_v1_3->enableModemResponseToggle) {
            sleep(1);
        }
        Return<void> resEnabled = radio_v1_3->getModemStackStatus(serial);
        ASSERT_OK(resEnabled);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_3->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_v1_3->rspInfo.serial);
        ALOGI("getModemStackStatus, rspInfo.error = %s\n",
              toString(radioRsp_v1_3->rspInfo.error).c_str());
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_3->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::MODEM_ERR, RadioError::INVALID_STATE}));
        // verify that enableModem did set isEnabled correctly
        EXPECT_EQ(true, radioRsp_v1_3->isModemEnabled);
    }
}

/*
 * Test IRadio.getModemStackStatus() for the response returned.
 */
TEST_P(RadioHidlTest_v1_3, getModemStackStatus) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_3->getModemStackStatus(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_3->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_3->rspInfo.serial);
    ALOGI("getModemStackStatus, rspInfo.error = %s\n",
          toString(radioRsp_v1_3->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_3->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));
}
