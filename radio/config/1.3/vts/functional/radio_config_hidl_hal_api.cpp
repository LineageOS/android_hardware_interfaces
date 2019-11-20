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

#include <radio_config_hidl_hal_utils.h>

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

/*
 * Test IRadioConfig.getPhoneCapability_1_3()
 */
TEST_P(RadioConfigHidlTest, getPhoneCapability_1_3) {
    serial = GetRandomSerialNumber();
    Return<void> res = radioConfig->getPhoneCapability_1_3(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);
    ALOGI("getPhoneCapability_1_3, rspInfo.error = %s\n",
          toString(radioConfigRsp->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            radioConfigRsp->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));

    if (radioConfigRsp->rspInfo.error == RadioError ::NONE) {
        int numModems = radioConfigRsp->phoneCap_1_3.logicalModemUuids.size();
        EXPECT_GE(numModems, 0);
        // length of simSlotCapabilities should be equal to length of logicalModemUuids.
        EXPECT_EQ(numModems, radioConfigRsp->phoneCap_1_3.simSlotCapabilities.size());
        // length of modemFeatures in each ConcurrentModemFeatures should be
        // equal to length of logicalModemUuids.
        for (V1_3::ConcurrentModemFeatures cmf :
             radioConfigRsp->phoneCap_1_3.concurrentFeatureSupport) {
            EXPECT_EQ(numModems, cmf.modemFeatures.size());
        }
    }
}
