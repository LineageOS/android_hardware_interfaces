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
 * Test IRadio.setAllowedNetworkTypesBitmap for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, setAllowedNetworkTypesBitmap) {
    serial = GetRandomSerialNumber();
    ::android::hardware::hidl_bitfield<::android::hardware::radio::V1_4::RadioAccessFamily>
            allowedNetworkTypesBitmap{};
    allowedNetworkTypesBitmap |= ::android::hardware::radio::V1_4::RadioAccessFamily::LTE;

    radio_v1_6->setAllowedNetworkTypesBitmap(serial, allowedNetworkTypesBitmap);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);

    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::NONE,
                 ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                 ::android::hardware::radio::V1_6::RadioError::OPERATION_NOT_ALLOWED,
                 ::android::hardware::radio::V1_6::RadioError::MODE_NOT_SUPPORTED,
                 ::android::hardware::radio::V1_6::RadioError::INTERNAL_ERR,
                 ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
                 ::android::hardware::radio::V1_6::RadioError::MODEM_ERR,
                 ::android::hardware::radio::V1_6::RadioError::NO_RESOURCES}));
    }
}

/*
 * Test IRadio.getAllowedNetworkTypesBitmap for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, getAllowedNetworkTypesBitmap) {
    serial = GetRandomSerialNumber();
    ::android::hardware::hidl_bitfield<::android::hardware::radio::V1_4::RadioAccessFamily>
            allowedNetworkTypesBitmap{};
    allowedNetworkTypesBitmap |= ::android::hardware::radio::V1_4::RadioAccessFamily::LTE;

    radio_v1_6->setAllowedNetworkTypesBitmap(serial, allowedNetworkTypesBitmap);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);

    if (radioRsp_v1_6->rspInfo.error == ::android::hardware::radio::V1_6::RadioError::NONE) {
        sleep(3);  // wait for modem
        serial = GetRandomSerialNumber();
        radio_v1_6->getAllowedNetworkTypesBitmap(serial);

        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);

        if (getRadioHalCapabilities()) {
            ASSERT_TRUE(CheckAnyOfErrors(
                    radioRsp_v1_6->rspInfo.error,
                    {::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED}));
        } else {
            ASSERT_TRUE(CheckAnyOfErrors(
                    radioRsp_v1_6->rspInfo.error,
                    {::android::hardware::radio::V1_6::RadioError::NONE,
                     ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                     ::android::hardware::radio::V1_6::RadioError::OPERATION_NOT_ALLOWED,
                     ::android::hardware::radio::V1_6::RadioError::MODE_NOT_SUPPORTED,
                     ::android::hardware::radio::V1_6::RadioError::INTERNAL_ERR,
                     ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
                     ::android::hardware::radio::V1_6::RadioError::MODEM_ERR,
                     ::android::hardware::radio::V1_6::RadioError::NO_RESOURCES}));
        }
    }
}

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

    ::android::hardware::radio::V1_6::OptionalSliceInfo optionalSliceInfo;
    memset(&optionalSliceInfo, 0, sizeof(optionalSliceInfo));

    ::android::hardware::radio::V1_6::OptionalTrafficDescriptor optionalTrafficDescriptor;
    memset(&optionalTrafficDescriptor, 0, sizeof(optionalTrafficDescriptor));

    bool matchAllRuleAllowed = true;

    Return<void> res =
            radio_v1_6->setupDataCall_1_6(serial, accessNetwork, dataProfileInfo, roamingAllowed,
                                          reason, addresses, dnses, -1, optionalSliceInfo,
                                          optionalTrafficDescriptor, matchAllRuleAllowed);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::SIM_ABSENT,
                 ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                 ::android::hardware::radio::V1_6::RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    } else if (cardStatus.base.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::NONE,
                 ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                 ::android::hardware::radio::V1_6::RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    }
}

TEST_P(RadioHidlTest_v1_6, setupDataCall_1_6_osAppId) {
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

    ::android::hardware::radio::V1_6::OptionalSliceInfo optionalSliceInfo;
    memset(&optionalSliceInfo, 0, sizeof(optionalSliceInfo));

    ::android::hardware::radio::V1_6::OptionalTrafficDescriptor optionalTrafficDescriptor;
    memset(&optionalTrafficDescriptor, 0, sizeof(optionalTrafficDescriptor));

    ::android::hardware::radio::V1_6::TrafficDescriptor trafficDescriptor;
    ::android::hardware::radio::V1_6::OsAppId osAppId;
    osAppId.osAppId = 1;
    trafficDescriptor.osAppId.value(osAppId);
    optionalTrafficDescriptor.value(trafficDescriptor);

    bool matchAllRuleAllowed = true;

    Return<void> res =
            radio_v1_6->setupDataCall_1_6(serial, accessNetwork, dataProfileInfo, roamingAllowed,
                                          reason, addresses, dnses, -1, optionalSliceInfo,
                                          optionalTrafficDescriptor, matchAllRuleAllowed);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::SIM_ABSENT,
                 ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                 ::android::hardware::radio::V1_6::RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    } else if (cardStatus.base.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::NONE,
                 ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                 ::android::hardware::radio::V1_6::RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
        if (radioRsp_v1_6->setupDataCallResult.trafficDescriptors.size() <= 0) {
            return;
        }
        EXPECT_EQ(optionalTrafficDescriptor.value().osAppId.value().osAppId,
                radioRsp_v1_6->setupDataCallResult.trafficDescriptors[0].osAppId.value().osAppId);
    }
}

/*
 * Test IRadio.getSlicingConfig() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, getSlicingConfig) {
    serial = GetRandomSerialNumber();
    radio_v1_6->getSlicingConfig(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_v1_6->rspInfo.error,
                                 {::android::hardware::radio::V1_6::RadioError::NONE,
                                  ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                                  ::android::hardware::radio::V1_6::RadioError::INTERNAL_ERR,
                                  ::android::hardware::radio::V1_6::RadioError::MODEM_ERR}));
    }
}

/*
 * Test IRadio_1_6.sendSms() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, sendSms_1_6) {
    LOG(DEBUG) << "sendSms";
    serial = GetRandomSerialNumber();
    GsmSmsMessage msg;
    msg.smscPdu = "";
    msg.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio_v1_6->sendSms_1_6(serial, msg);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);

    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_6->rspInfo.error,
            {::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
             ::android::hardware::radio::V1_6::RadioError::INVALID_STATE,
             ::android::hardware::radio::V1_6::RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
        EXPECT_EQ(0, radioRsp_v1_6->sendSmsResult.errorCode);
    }
    LOG(DEBUG) << "sendSms finished";
}

/*
 * Test IRadio_1_6.sendSmsExpectMore() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, sendSmsExpectMore_1_6) {
    LOG(DEBUG) << "sendSmsExpectMore";
    serial = GetRandomSerialNumber();
    GsmSmsMessage msg;
    msg.smscPdu = "";
    msg.pdu = "01000b916105770203f3000006d4f29c3e9b01";

    radio_v1_6->sendSmsExpectMore_1_6(serial, msg);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);

    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_6->rspInfo.error,
            {::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
             ::android::hardware::radio::V1_6::RadioError::INVALID_STATE,
             ::android::hardware::radio::V1_6::RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "sendSmsExpectMore finished";
}

/*
 * Test IRadio_1_6.sendCdmaSms() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, sendCdmaSms_1_6) {
    LOG(DEBUG) << "sendCdmaSms";
    serial = GetRandomSerialNumber();

    // Create a CdmaSmsAddress
    CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = CdmaSmsDigitMode::FOUR_BIT;
    cdmaSmsAddress.numberMode = CdmaSmsNumberMode::NOT_DATA_NETWORK;
    cdmaSmsAddress.numberType = CdmaSmsNumberType::UNKNOWN;
    cdmaSmsAddress.numberPlan = CdmaSmsNumberPlan::UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>){11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3};

    // Create a CdmaSmsSubAddress
    CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = CdmaSmsSubaddressType::NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>){};

    // Create a CdmaSmsMessage
    android::hardware::radio::V1_0::CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData =
        (std::vector<uint8_t>){15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    radio_v1_6->sendCdmaSms_1_6(serial, cdmaSmsMessage);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);

    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_6->rspInfo.error,
            {::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
             ::android::hardware::radio::V1_6::RadioError::INVALID_STATE,
             ::android::hardware::radio::V1_6::RadioError::SIM_ABSENT},
            CHECK_GENERAL_ERROR));
    }
    LOG(DEBUG) << "sendCdmaSms finished";
}

/*
 * Test IRadio_1_6.sendCdmaSmsExpectMore() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, sendCdmaSmsExpectMore_1_6) {
    serial = GetRandomSerialNumber();

    // Create a CdmaSmsAddress
    CdmaSmsAddress cdmaSmsAddress;
    cdmaSmsAddress.digitMode = CdmaSmsDigitMode::FOUR_BIT;
    cdmaSmsAddress.numberMode = CdmaSmsNumberMode::NOT_DATA_NETWORK;
    cdmaSmsAddress.numberType = CdmaSmsNumberType::UNKNOWN;
    cdmaSmsAddress.numberPlan = CdmaSmsNumberPlan::UNKNOWN;
    cdmaSmsAddress.digits = (std::vector<uint8_t>){11, 1, 6, 5, 10, 7, 7, 2, 10, 3, 10, 3};

    // Create a CdmaSmsSubAddress
    CdmaSmsSubaddress cdmaSmsSubaddress;
    cdmaSmsSubaddress.subaddressType = CdmaSmsSubaddressType::NSAP;
    cdmaSmsSubaddress.odd = false;
    cdmaSmsSubaddress.digits = (std::vector<uint8_t>){};

    // Create a CdmaSmsMessage
    android::hardware::radio::V1_0::CdmaSmsMessage cdmaSmsMessage;
    cdmaSmsMessage.teleserviceId = 4098;
    cdmaSmsMessage.isServicePresent = false;
    cdmaSmsMessage.serviceCategory = 0;
    cdmaSmsMessage.address = cdmaSmsAddress;
    cdmaSmsMessage.subAddress = cdmaSmsSubaddress;
    cdmaSmsMessage.bearerData =
            (std::vector<uint8_t>){15, 0, 3, 32, 3, 16, 1, 8, 16, 53, 76, 68, 6, 51, 106, 0};

    radio_v1_6->sendCdmaSmsExpectMore_1_6(serial, cdmaSmsMessage);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);

    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
                 ::android::hardware::radio::V1_6::RadioError::INVALID_STATE,
                 ::android::hardware::radio::V1_6::RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.setRadioPower_1_6() for the response returned by
 * IRadio.setRadioPowerResponse_1_6().
 */
TEST_P(RadioHidlTest_v1_6, setRadioPower_1_6_emergencyCall_cancelled) {
    // Set radio power to off.
    serial = GetRandomSerialNumber();
    radio_v1_6->setRadioPower_1_6(serial, false, false, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    EXPECT_EQ(::android::hardware::radio::V1_6::RadioError::NONE, radioRsp_v1_6->rspInfo.error);

    // Set radio power to on with forEmergencyCall being true. This should put modem to only scan
    // emergency call bands.
    serial = GetRandomSerialNumber();
    radio_v1_6->setRadioPower_1_6(serial, true, true, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    EXPECT_EQ(::android::hardware::radio::V1_6::RadioError::NONE, radioRsp_v1_6->rspInfo.error);

    // Set radio power to on with forEmergencyCall being false. This should put modem in regular
    // operation modem.
    serial = GetRandomSerialNumber();
    radio_v1_6->setRadioPower_1_6(serial, true, false, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    EXPECT_EQ(::android::hardware::radio::V1_6::RadioError::NONE, radioRsp_v1_6->rspInfo.error);
}

/*
 * Test IRadio.setNrDualConnectivityState() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, setNrDualConnectivityState) {
    serial = GetRandomSerialNumber();

    Return<void> res =
            radio_v1_6->setNrDualConnectivityState(serial, NrDualConnectivityState::DISABLE);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                 ::android::hardware::radio::V1_6::RadioError::INTERNAL_ERR,
                 ::android::hardware::radio::V1_6::RadioError::INVALID_STATE,
                 ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED,
                 ::android::hardware::radio::V1_6::RadioError::NONE}));
    }
}

/*
 * Test IRadio.isNrDualConnectivityEnabled() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, isNrDualConnectivityEnabled) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_6->isNrDualConnectivityEnabled(serial);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_v1_6->rspInfo.error,
                                 {::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                                  ::android::hardware::radio::V1_6::RadioError::INTERNAL_ERR,
                                  ::android::hardware::radio::V1_6::RadioError::NONE}));
    }
}

/*
 * Test IRadio.setDataThrottling() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, setDataThrottling) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_6->setDataThrottling(
            serial, DataThrottlingAction::THROTTLE_SECONDARY_CARRIER, 60000);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                 ::android::hardware::radio::V1_6::RadioError::MODEM_ERR,
                 ::android::hardware::radio::V1_6::RadioError::NONE,
                 ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS}));
    }

    sleep(1);
    serial = GetRandomSerialNumber();

    res = radio_v1_6->setDataThrottling(serial, DataThrottlingAction::THROTTLE_ANCHOR_CARRIER,
                                        60000);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                 ::android::hardware::radio::V1_6::RadioError::MODEM_ERR,
                 ::android::hardware::radio::V1_6::RadioError::NONE,
                 ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS}));
    }

    sleep(1);
    serial = GetRandomSerialNumber();

    res = radio_v1_6->setDataThrottling(serial, DataThrottlingAction::HOLD, 60000);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                 ::android::hardware::radio::V1_6::RadioError::MODEM_ERR,
                 ::android::hardware::radio::V1_6::RadioError::NONE,
                 ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS}));
    }

    sleep(1);
    serial = GetRandomSerialNumber();

    res = radio_v1_6->setDataThrottling(serial, DataThrottlingAction::NO_DATA_THROTTLING, 60000);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
                 ::android::hardware::radio::V1_6::RadioError::MODEM_ERR,
                 ::android::hardware::radio::V1_6::RadioError::NONE,
                 ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS}));
    }

    sleep(1);
}

/*
 * Test IRadio.setSimCardPower_1_6() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, setSimCardPower_1_6) {
    /* Test setSimCardPower power down */
    serial = GetRandomSerialNumber();
    radio_v1_6->setSimCardPower_1_6(serial, CardPowerState::POWER_DOWN);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_6->rspInfo.error,
                             {::android::hardware::radio::V1_6::RadioError::NONE,
                              ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
                              ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE}));

    // setSimCardPower_1_6 does not return  until the request is handled, and should not trigger
    // CardState::ABSENT when turning off power
    if (radioRsp_v1_6->rspInfo.error == ::android::hardware::radio::V1_6::RadioError::NONE) {
        /* Wait some time for setting sim power down and then verify it */
        updateSimCardStatus();
        EXPECT_EQ(CardState::PRESENT, cardStatus.base.base.base.cardState);
        // applications should be an empty vector of AppStatus
        EXPECT_EQ(0, cardStatus.applications.size());
    }

    /* Test setSimCardPower power up */
    serial = GetRandomSerialNumber();
    radio_v1_6->setSimCardPower_1_6(serial, CardPowerState::POWER_UP);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_6->rspInfo.error,
                             {::android::hardware::radio::V1_6::RadioError::NONE,
                              ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
                              ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE}));

    // setSimCardPower_1_6 does not return  until the request is handled. Just verify that we still
    // have CardState::PRESENT after turning the power back on
    if (radioRsp_v1_6->rspInfo.error == ::android::hardware::radio::V1_6::RadioError::NONE) {
        updateSimCardStatus();
        EXPECT_EQ(CardState::PRESENT, cardStatus.base.base.base.cardState);
    }
}

/*
 * Test IRadio.emergencyDial() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, emergencyDial_1_6) {
    if (!deviceSupportsFeature(FEATURE_VOICE_CALL)) {
        ALOGI("Skipping emergencyDial because voice call is not supported in device");
        return;
    } else {
        ALOGI("Running emergencyDial because voice call is supported in device");
    }

    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_0::Dial dialInfo;
    dialInfo.address = hidl_string("911");
    int categories = static_cast<int>(
            ::android::hardware::radio::V1_4::EmergencyServiceCategory::UNSPECIFIED);
    std::vector<hidl_string> urns = {""};
    ::android::hardware::radio::V1_4::EmergencyCallRouting routing =
            ::android::hardware::radio::V1_4::EmergencyCallRouting::UNKNOWN;

    Return<void> res =
            radio_v1_6->emergencyDial_1_6(serial, dialInfo, categories, urns, routing, true, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo_v1_0.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo_v1_0.serial);

    ALOGI("emergencyDial, rspInfo_v1_0.error = %s\n",
          toString(radioRsp_v1_6->rspInfo_v1_0.error).c_str());

    ::android::hardware::radio::V1_0::RadioError rspEmergencyDial =
            radioRsp_v1_6->rspInfo_v1_0.error;
    // In DSDS or TSTS, we only check the result if the current slot is IN_SERVICE
    // or Emergency_Only.
    if (isDsDsEnabled() || isTsTsEnabled()) {
        serial = GetRandomSerialNumber();
        radio_v1_6->getVoiceRegistrationState_1_6(serial);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        if (isVoiceEmergencyOnly(radioRsp_v1_6->voiceRegResp.regState) ||
            isVoiceInService(radioRsp_v1_6->voiceRegResp.regState)) {
            EXPECT_EQ(::android::hardware::radio::V1_0::RadioError::NONE, rspEmergencyDial);
        }
    } else {
        EXPECT_EQ(::android::hardware::radio::V1_0::RadioError::NONE, rspEmergencyDial);
    }

    // Give some time for modem to establish the emergency call channel.
    sleep(MODEM_EMERGENCY_CALL_ESTABLISH_TIME);

    // Disconnect all the potential established calls to prevent them affecting other tests.
    clearPotentialEstablishedCalls();
}

/*
 * Test IRadio.emergencyDial() with specified service and its response returned.
 */
TEST_P(RadioHidlTest_v1_6, emergencyDial_1_6_withServices) {
    if (!deviceSupportsFeature(FEATURE_VOICE_CALL)) {
        ALOGI("Skipping emergencyDial because voice call is not supported in device");
        return;
    } else {
        ALOGI("Running emergencyDial because voice call is supported in device");
    }

    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_0::Dial dialInfo;
    dialInfo.address = hidl_string("911");
    int categories =
            static_cast<int>(::android::hardware::radio::V1_4::EmergencyServiceCategory::AMBULANCE);
    std::vector<hidl_string> urns = {"urn:service:sos.ambulance"};
    ::android::hardware::radio::V1_4::EmergencyCallRouting routing =
            ::android::hardware::radio::V1_4::EmergencyCallRouting::UNKNOWN;

    Return<void> res =
            radio_v1_6->emergencyDial_1_6(serial, dialInfo, categories, urns, routing, true, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo_v1_0.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo_v1_0.serial);

    ALOGI("emergencyDial_withServices, rspInfo_v1_0.error = %s\n",
          toString(radioRsp_v1_6->rspInfo_v1_0.error).c_str());
    ::android::hardware::radio::V1_0::RadioError rspEmergencyDial =
            radioRsp_v1_6->rspInfo_v1_0.error;

    // In DSDS or TSTS, we only check the result if the current slot is IN_SERVICE
    // or Emergency_Only.
    if (isDsDsEnabled() || isTsTsEnabled()) {
        serial = GetRandomSerialNumber();
        radio_v1_6->getVoiceRegistrationState_1_6(serial);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        if (isVoiceEmergencyOnly(radioRsp_v1_6->voiceRegResp.regState) ||
            isVoiceInService(radioRsp_v1_6->voiceRegResp.regState)) {
            EXPECT_EQ(::android::hardware::radio::V1_0::RadioError::NONE, rspEmergencyDial);
        }
    } else {
        EXPECT_EQ(::android::hardware::radio::V1_0::RadioError::NONE, rspEmergencyDial);
    }
    // Give some time for modem to establish the emergency call channel.
    sleep(MODEM_EMERGENCY_CALL_ESTABLISH_TIME);

    // Disconnect all the potential established calls to prevent them affecting other tests.
    clearPotentialEstablishedCalls();
}

/*
 * Test IRadio.emergencyDial() with known emergency call routing and its response returned.
 */
TEST_P(RadioHidlTest_v1_6, emergencyDial_1_6_withEmergencyRouting) {
    if (!deviceSupportsFeature(FEATURE_VOICE_CALL)) {
        ALOGI("Skipping emergencyDial because voice call is not supported in device");
        return;
    } else {
        ALOGI("Running emergencyDial because voice call is supported in device");
    }

    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_0::Dial dialInfo;
    dialInfo.address = hidl_string("911");
    int categories = static_cast<int>(
            ::android::hardware::radio::V1_4::EmergencyServiceCategory::UNSPECIFIED);
    std::vector<hidl_string> urns = {""};
    ::android::hardware::radio::V1_4::EmergencyCallRouting routing =
            ::android::hardware::radio::V1_4::EmergencyCallRouting::EMERGENCY;

    Return<void> res =
            radio_v1_6->emergencyDial_1_6(serial, dialInfo, categories, urns, routing, true, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo_v1_0.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo_v1_0.serial);

    ALOGI("emergencyDial_withEmergencyRouting, rspInfo_v1_0.error = %s\n",
          toString(radioRsp_v1_6->rspInfo_v1_0.error).c_str());
    ::android::hardware::radio::V1_0::RadioError rspEmergencyDial =
            radioRsp_v1_6->rspInfo_v1_0.error;

    // In DSDS or TSTS, we only check the result if the current slot is IN_SERVICE
    // or Emergency_Only.
    if (isDsDsEnabled() || isTsTsEnabled()) {
        serial = GetRandomSerialNumber();
        radio_v1_6->getVoiceRegistrationState_1_6(serial);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        if (isVoiceEmergencyOnly(radioRsp_v1_6->voiceRegResp.regState) ||
            isVoiceInService(radioRsp_v1_6->voiceRegResp.regState)) {
            EXPECT_EQ(::android::hardware::radio::V1_0::RadioError::NONE, rspEmergencyDial);
        }
    } else {
        EXPECT_EQ(::android::hardware::radio::V1_0::RadioError::NONE, rspEmergencyDial);
    }

    // Give some time for modem to establish the emergency call channel.
    sleep(MODEM_EMERGENCY_CALL_ESTABLISH_TIME);

    // Disconnect all the potential established calls to prevent them affecting other tests.
    clearPotentialEstablishedCalls();
}

/*
 * Test IRadio.getCurrentCalls_1_6() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, getCurrentCalls_1_6) {
    serial = GetRandomSerialNumber();
    radio_v1_6->getCurrentCalls_1_6(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    EXPECT_EQ(::android::hardware::radio::V1_6::RadioError::NONE, radioRsp_v1_6->rspInfo.error);
}

/*
 * Test IRadio.setCarrierInfoForImsiEncryption_1_6() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, setCarrierInfoForImsiEncryption_1_6) {
    serial = GetRandomSerialNumber();
    ::android::hardware::radio::V1_6::ImsiEncryptionInfo imsiInfo;
    imsiInfo.base.mcc = "310";
    imsiInfo.base.mnc = "004";
    imsiInfo.base.carrierKey = (std::vector<uint8_t>){1, 2, 3, 4, 5, 6};
    imsiInfo.base.keyIdentifier = "Test";
    imsiInfo.base.expirationTime = 20180101;
    imsiInfo.keyType = PublicKeyType::EPDG;

    radio_v1_6->setCarrierInfoForImsiEncryption_1_6(serial, imsiInfo);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo_v1_0.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo_v1_0.serial);

    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_6->rspInfo.error,
                {::android::hardware::radio::V1_6::RadioError::NONE,
                 ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.getSimPhonebookRecords() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, getSimPhonebookRecords) {
    serial = GetRandomSerialNumber();
    radio_v1_6->getSimPhonebookRecords(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_6->rspInfo.error,
            {::android::hardware::radio::V1_6::RadioError::INVALID_SIM_STATE,
             ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
             ::android::hardware::radio::V1_6::RadioError::MODEM_ERR,
             ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
             ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED},
             CHECK_GENERAL_ERROR));
    } else if (cardStatus.base.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_6->rspInfo.error,
            {::android::hardware::radio::V1_6::RadioError::NONE,
             ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED},
             CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getSimPhonebookCapacity for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, getSimPhonebookCapacity) {
    serial = GetRandomSerialNumber();
    radio_v1_6->getSimPhonebookCapacity(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_6->rspInfo.error,
            {::android::hardware::radio::V1_6::RadioError::INVALID_SIM_STATE,
             ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
             ::android::hardware::radio::V1_6::RadioError::MODEM_ERR,
             ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
             ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED},
             CHECK_GENERAL_ERROR));
    } else if (cardStatus.base.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_6->rspInfo.error,
            {::android::hardware::radio::V1_6::RadioError::NONE,
            ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED},
            CHECK_GENERAL_ERROR));

        ::android::hardware::radio::V1_6::PhonebookCapacity pbCapacity =
             radioRsp_v1_6->capacity;
        if(pbCapacity.maxAdnRecords > 0) {
            EXPECT_TRUE(pbCapacity.maxNameLen > 0 && pbCapacity.maxNumberLen > 0);
            EXPECT_TRUE(pbCapacity.usedAdnRecords <= pbCapacity.maxAdnRecords);
        }

        if(pbCapacity.maxEmailRecords > 0) {
            EXPECT_TRUE(pbCapacity.maxEmailLen > 0);
            EXPECT_TRUE(pbCapacity.usedEmailRecords <= pbCapacity.maxEmailRecords);
        }

        if(pbCapacity.maxAdditionalNumberRecords > 0) {
            EXPECT_TRUE(pbCapacity.maxAdditionalNumberLen > 0);
            EXPECT_TRUE(pbCapacity.usedAdditionalNumberRecords <= pbCapacity.maxAdditionalNumberRecords);
        }
    }
}

/*
 * Test IRadio.updateSimPhonebookRecords() for the response returned.
 */
TEST_P(RadioHidlTest_v1_6, updateSimPhonebookRecords) {
    serial = GetRandomSerialNumber();
    radio_v1_6->getSimPhonebookCapacity(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
    if (cardStatus.base.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_6->rspInfo.error,
            {::android::hardware::radio::V1_6::RadioError::INVALID_SIM_STATE,
             ::android::hardware::radio::V1_6::RadioError::RADIO_NOT_AVAILABLE,
             ::android::hardware::radio::V1_6::RadioError::MODEM_ERR,
             ::android::hardware::radio::V1_6::RadioError::INVALID_ARGUMENTS,
             ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED},
             CHECK_GENERAL_ERROR));
    } else if (cardStatus.base.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_6->rspInfo.error,
            {::android::hardware::radio::V1_6::RadioError::NONE,
             ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED},
             CHECK_GENERAL_ERROR));
        ::android::hardware::radio::V1_6::PhonebookCapacity pbCapacity =
                radioRsp_v1_6->capacity;

        serial = GetRandomSerialNumber();
        radio_v1_6->getSimPhonebookRecords(serial);

        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_6->rspInfo.error,
            {::android::hardware::radio::V1_6::RadioError::NONE,
             ::android::hardware::radio::V1_6::RadioError::REQUEST_NOT_SUPPORTED},
             CHECK_GENERAL_ERROR));

        if(pbCapacity.maxAdnRecords > 0
                && pbCapacity.usedAdnRecords < pbCapacity.maxAdnRecords) {
            // Add a phonebook record
            PhonebookRecordInfo recordInfo;
            recordInfo.recordId = 0;
            recordInfo.name = "ABC";
            recordInfo.number = "1234567890";
            serial = GetRandomSerialNumber();
            radio_v1_6->updateSimPhonebookRecords(serial, recordInfo);

            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
            EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
            EXPECT_EQ(::android::hardware::radio::V1_6::RadioError::NONE, radioRsp_v1_6->rspInfo.error);
            int index = radioRsp_v1_6->updatedRecordIndex;
            EXPECT_TRUE(index > 0);

            // Deleted a phonebook record
            recordInfo.recordId = index;
            recordInfo.name = "";
            recordInfo.number = "";
            serial = GetRandomSerialNumber();
            radio_v1_6->updateSimPhonebookRecords(serial, recordInfo);

            EXPECT_EQ(std::cv_status::no_timeout, wait());
            EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo.type);
            EXPECT_EQ(serial, radioRsp_v1_6->rspInfo.serial);
            EXPECT_EQ(::android::hardware::radio::V1_6::RadioError::NONE, radioRsp_v1_6->rspInfo.error);
        }
    }
}
