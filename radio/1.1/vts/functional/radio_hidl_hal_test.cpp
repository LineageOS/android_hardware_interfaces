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

void RadioHidlTest_v1_1::SetUp() {
    radio_v1_1 =
        ::testing::VtsHalHidlTargetTestBase::getService<::android::hardware::radio::V1_1::IRadio>(
            hidl_string(RADIO_SERVICE_NAME));
    ASSERT_NE(radio_v1_1, nullptr);

    radioRsp_v1_1 = new RadioResponse_v1_1(*this);
    ASSERT_NE(radioRsp_v1_1, nullptr);

    count = 0;

    radioInd_v1_1 = NULL;
    radio_v1_1->setResponseFunctions(radioRsp_v1_1, radioInd_v1_1);

    int serial = GetRandomSerialNumber();
    radio_v1_1->getIccCardStatus(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_1->rspInfo.error);
}