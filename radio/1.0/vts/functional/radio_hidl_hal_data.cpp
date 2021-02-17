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
#include <android/hardware/radio/1.2/IRadio.h>
#include <radio_hidl_hal_utils_v1_0.h>

using namespace ::android::hardware::radio::V1_0;

/*
 * Test IRadio.getDataRegistrationState() for the response returned.
 */
TEST_P(RadioHidlTest, getDataRegistrationState) {
    LOG(DEBUG) << "getDataRegistrationState";
    serial = GetRandomSerialNumber();

    radio->getDataRegistrationState(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    } else if (cardStatus.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::NOT_PROVISIONED, RadioError::CANCELLED}));

        // Check the mcc [0, 999] and mnc [0, 999].
        string hidl_mcc;
        string hidl_mnc;
        bool checkMccMnc = true;
        int totalIdentitySizeExpected = 1;
        CellIdentity cellIdentities = radioRsp->dataRegResp.cellIdentity;
        CellInfoType cellInfoType = cellIdentities.cellInfoType;

        if (cellInfoType == CellInfoType::NONE) {
            // All the fields are 0
            totalIdentitySizeExpected = 0;
            checkMccMnc = false;
        } else if (cellInfoType == CellInfoType::GSM) {
            EXPECT_EQ(1, cellIdentities.cellIdentityGsm.size());
            CellIdentityGsm cig = cellIdentities.cellIdentityGsm[0];
            hidl_mcc = cig.mcc;
            hidl_mnc = cig.mnc;
        } else if (cellInfoType == CellInfoType::LTE) {
            EXPECT_EQ(1, cellIdentities.cellIdentityLte.size());
            CellIdentityLte cil = cellIdentities.cellIdentityLte[0];
            hidl_mcc = cil.mcc;
            hidl_mnc = cil.mnc;
        } else if (cellInfoType == CellInfoType::WCDMA) {
            EXPECT_EQ(1, cellIdentities.cellIdentityWcdma.size());
            CellIdentityWcdma ciw = cellIdentities.cellIdentityWcdma[0];
            hidl_mcc = ciw.mcc;
            hidl_mnc = ciw.mnc;
        } else if (cellInfoType == CellInfoType::TD_SCDMA) {
            EXPECT_EQ(1, cellIdentities.cellIdentityTdscdma.size());
            CellIdentityTdscdma cit = cellIdentities.cellIdentityTdscdma[0];
            hidl_mcc = cit.mcc;
            hidl_mnc = cit.mnc;
        } else {
            // CellIndentityCdma has no mcc and mnc.
            EXPECT_EQ(CellInfoType::CDMA, cellInfoType);
            EXPECT_EQ(1, cellIdentities.cellIdentityCdma.size());
            checkMccMnc = false;
        }

        // Check only one CellIdentity is size 1, and others must be 0.
        EXPECT_EQ(totalIdentitySizeExpected, cellIdentities.cellIdentityGsm.size() +
                                                 cellIdentities.cellIdentityCdma.size() +
                                                 cellIdentities.cellIdentityLte.size() +
                                                 cellIdentities.cellIdentityWcdma.size() +
                                                 cellIdentities.cellIdentityTdscdma.size());

        if (checkMccMnc) {
            // 32 bit system gets result: "\xff\xff\xff..." from RIL, which is not testable. Only
            // test for 64 bit here. TODO: remove this limit after b/113181277 being fixed.
            if (hidl_mcc.size() < 4 && hidl_mnc.size() < 4) {
                int mcc = stoi(hidl_mcc);
                int mnc = stoi(hidl_mnc);
                EXPECT_TRUE(mcc >= 0 && mcc <= 999);
                EXPECT_TRUE(mnc >= 0 && mnc <= 999);
            }
        }
    }
    LOG(DEBUG) << "getDataRegistrationState finished";
}

/*
 * Test IRadio.setupDataCall() for the response returned.
 */
TEST_P(RadioHidlTest, setupDataCall) {
    LOG(DEBUG) << "setupDataCall";
    serial = GetRandomSerialNumber();

    RadioTechnology radioTechnology = RadioTechnology::LTE;

    DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileId::IMS;
    dataProfileInfo.apn = hidl_string("VZWIMS");
    dataProfileInfo.protocol = hidl_string("IPV4V6");
    dataProfileInfo.roamingProtocol = hidl_string("IPV6");
    dataProfileInfo.authType = ApnAuthType::NO_PAP_NO_CHAP;
    dataProfileInfo.user = "";
    dataProfileInfo.password = "";
    dataProfileInfo.type = DataProfileInfoType::THREE_GPP2;
    dataProfileInfo.maxConnsTime = 300;
    dataProfileInfo.maxConns = 20;
    dataProfileInfo.waitTime = 0;
    dataProfileInfo.enabled = true;
    dataProfileInfo.supportedApnTypesBitmap = 320;
    dataProfileInfo.bearerBitmap = 161543;
    dataProfileInfo.mtu = 0;
    dataProfileInfo.mvnoType = MvnoType::NONE;
    dataProfileInfo.mvnoMatchData = hidl_string();

    bool modemCognitive = false;
    bool roamingAllowed = false;
    bool isRoaming = false;

    radio->setupDataCall(serial, radioTechnology, dataProfileInfo, modemCognitive, roamingAllowed,
                         isRoaming);

    EXPECT_EQ(std::cv_status::no_timeout, wait(300));
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    // setupDataCall is deprecated on radio::V1_2 with setupDataCall_1_2
    SKIP_TEST_IF_REQUEST_NOT_SUPPORTED_WITH_HAL_VERSION_AT_LEAST(1_2);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW,
                                      RadioError::OP_NOT_ALLOWED_DURING_VOICE_CALL,
                                      RadioError::RADIO_NOT_AVAILABLE, RadioError::SIM_ABSENT},
                                     CHECK_OEM_ERROR));
    }
    LOG(DEBUG) << "setupDataCall finished";
}

/*
 * Test IRadio.deactivateDataCall() for the response returned.
 */
TEST_P(RadioHidlTest, deactivateDataCall) {
    LOG(DEBUG) << "deactivateDataCall";
    serial = GetRandomSerialNumber();
    int cid = 1;
    bool reasonRadioShutDown = false;

    radio->deactivateDataCall(serial, cid, reasonRadioShutDown);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    // deactivateDataCall is deprecated on radio::V1_2 with deactiveDataCall_1_2
    SKIP_TEST_IF_REQUEST_NOT_SUPPORTED_WITH_HAL_VERSION_AT_LEAST(1_2);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::SIM_ABSENT, RadioError::INVALID_CALL_ID},
                                     CHECK_OEM_ERROR));
    }
    LOG(DEBUG) << "deactivateDataCall finished";
}

/*
 * Test IRadio.getDataCallList() for the response returned.
 */
TEST_P(RadioHidlTest, getDataCallList) {
    LOG(DEBUG) << "getDataCallList";
    serial = GetRandomSerialNumber();

    radio->getDataCallList(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::SIM_ABSENT}));
    }
    LOG(DEBUG) << "getDataCallList finished";
}

/*
 * Test IRadio.setInitialAttachApn() for the response returned.
 */
TEST_P(RadioHidlTest, setInitialAttachApn) {
    LOG(DEBUG) << "setInitialAttachApn";
    serial = GetRandomSerialNumber();

    DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileId::IMS;
    dataProfileInfo.apn = hidl_string("VZWIMS");
    dataProfileInfo.protocol = hidl_string("IPV4V6");
    dataProfileInfo.roamingProtocol = hidl_string("IPV6");
    dataProfileInfo.authType = ApnAuthType::NO_PAP_NO_CHAP;
    dataProfileInfo.user = "";
    dataProfileInfo.password = "";
    dataProfileInfo.type = DataProfileInfoType::THREE_GPP2;
    dataProfileInfo.maxConnsTime = 300;
    dataProfileInfo.maxConns = 20;
    dataProfileInfo.waitTime = 0;
    dataProfileInfo.enabled = true;
    dataProfileInfo.supportedApnTypesBitmap = 320;
    dataProfileInfo.bearerBitmap = 161543;
    dataProfileInfo.mtu = 0;
    dataProfileInfo.mvnoType = MvnoType::NONE;
    dataProfileInfo.mvnoMatchData = hidl_string();

    bool modemCognitive = true;
    bool isRoaming = false;

    radio->setInitialAttachApn(serial, dataProfileInfo, modemCognitive, isRoaming);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::SUBSCRIPTION_NOT_AVAILABLE},
                                     CHECK_OEM_ERROR));
    }
    LOG(DEBUG) << "setInitialAttachApn finished";
}

/*
 * Test IRadio.setDataAllowed() for the response returned.
 */
TEST_P(RadioHidlTest, setDataAllowed) {
    LOG(DEBUG) << "setDataAllowed";
    serial = GetRandomSerialNumber();
    bool allow = true;

    radio->setDataAllowed(serial, allow);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);
    }
    LOG(DEBUG) << "setDataAllowed finished";
}

/*
 * Test IRadio.setDataProfile() for the response returned.
 */
TEST_P(RadioHidlTest, setDataProfile) {
    LOG(DEBUG) << "setDataProfile";
    serial = GetRandomSerialNumber();

    // Create a dataProfileInfo
    DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileId::IMS;
    dataProfileInfo.apn = hidl_string("VZWIMS");
    dataProfileInfo.protocol = hidl_string("IPV4V6");
    dataProfileInfo.roamingProtocol = hidl_string("IPV6");
    dataProfileInfo.authType = ApnAuthType::NO_PAP_NO_CHAP;
    dataProfileInfo.user = "";
    dataProfileInfo.password = "";
    dataProfileInfo.type = DataProfileInfoType::THREE_GPP2;
    dataProfileInfo.maxConnsTime = 300;
    dataProfileInfo.maxConns = 20;
    dataProfileInfo.waitTime = 0;
    dataProfileInfo.enabled = true;
    dataProfileInfo.supportedApnTypesBitmap = 320;
    dataProfileInfo.bearerBitmap = 161543;
    dataProfileInfo.mtu = 0;
    dataProfileInfo.mvnoType = MvnoType::NONE;
    dataProfileInfo.mvnoMatchData = hidl_string();

    // Create a dataProfileInfoList
    android::hardware::hidl_vec<DataProfileInfo> dataProfileInfoList = {dataProfileInfo};

    bool isRoadming = false;

    radio->setDataProfile(serial, dataProfileInfoList, isRoadming);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);

    if (cardStatus.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::SIM_ABSENT, RadioError::REQUEST_NOT_SUPPORTED}));
    }
    LOG(DEBUG) << "setDataProfile finished";
}
