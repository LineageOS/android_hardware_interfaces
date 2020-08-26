/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_6.h>

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

/*
 * Test IRadio.setupDataCall_1_6() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, setupDataCall_1_6) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::AccessNetwork accessNetwork =
            ::android::hardware::radio::V1_5::AccessNetwork::EUTRAN;

    android::hardware::radio::V1_5::DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileId::DEFAULT;
    dataProfileInfo.apn = hidl_string("internet");
    dataProfileInfo.protocol = PdpProtocolType::IP;
    dataProfileInfo.roamingProtocol = PdpProtocolType::IP;
    dataProfileInfo.authType = ApnAuthType::NO_PAP_NO_CHAP;
    dataProfileInfo.user = hidl_string("username");
    dataProfileInfo.password = hidl_string("password");
    dataProfileInfo.type = DataProfileInfoType::THREE_GPP;
    dataProfileInfo.maxConnsTime = 300;
    dataProfileInfo.maxConns = 20;
    dataProfileInfo.waitTime = 0;
    dataProfileInfo.enabled = true;
    dataProfileInfo.supportedApnTypesBitmap = 320;
    dataProfileInfo.bearerBitmap = 161543;
    dataProfileInfo.mtuV4 = 0;
    dataProfileInfo.mtuV6 = 0;
    dataProfileInfo.preferred = true;
    dataProfileInfo.persistent = false;

    bool roamingAllowed = false;

    std::vector<::android::hardware::radio::V1_5::LinkAddress> addresses = {};
    std::vector<hidl_string> dnses = {};

    ::android::hardware::radio::V1_2::DataRequestReason reason =
            ::android::hardware::radio::V1_2::DataRequestReason::NORMAL;

    Return<void> res = radio_v1_6->setupDataCall_1_6(serial, accessNetwork, dataProfileInfo,
                                                     roamingAllowed, reason, addresses, dnses);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);

    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_6->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    } else if (cardStatus.base.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_6->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    }
}
