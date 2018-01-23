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
 * Test IRadioConfig.getSimSlotsStatus()
 */
TEST_F(RadioConfigHidlTest, getSimSlotsStatus) {
    const int serial = GetRandomSerialNumber();
    Return<void> res = radioConfig->getSimSlotsStatus(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("getIccSlotsStatus, rspInfo.error = %s\n",
          toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(radioConfigRsp->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioConfig.setSimSlotsMapping()
 */
TEST_F(RadioConfigHidlTest, setSimSlotsMapping) {
    const int serial = GetRandomSerialNumber();
    android::hardware::hidl_vec<uint32_t> mapping = {0};
    Return<void> res = radioConfig->setSimSlotsMapping(serial, mapping);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("setSimSlotsMapping, rspInfo.error = %s\n",
          toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(radioConfigRsp->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}
