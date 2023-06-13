/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <aidl/android/hardware/radio/RadioAccessFamily.h>
#include <aidl/android/hardware/radio/config/IRadioConfig.h>
#include <aidl/android/hardware/radio/data/ApnTypes.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>

#include "radio_data_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioDataTest::SetUp() {
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_data = IRadioData::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_data.get());

    radioRsp_data = ndk::SharedRefBase::make<RadioDataResponse>(*this);
    ASSERT_NE(nullptr, radioRsp_data.get());

    count_ = 0;

    radioInd_data = ndk::SharedRefBase::make<RadioDataIndication>(*this);
    ASSERT_NE(nullptr, radioInd_data.get());

    radio_data->setResponseFunctions(radioRsp_data, radioInd_data);

    // Assert IRadioSim exists and SIM is present before testing
    radio_sim = sim::IRadioSim::fromBinder(ndk::SpAIBinder(
            AServiceManager_waitForService("android.hardware.radio.sim.IRadioSim/slot1")));
    ASSERT_NE(nullptr, radio_sim.get());
    updateSimCardStatus();
    EXPECT_EQ(CardStatus::STATE_PRESENT, cardStatus.cardState);

    // Assert IRadioConfig exists before testing
    radio_config = config::IRadioConfig::fromBinder(ndk::SpAIBinder(
            AServiceManager_waitForService("android.hardware.radio.config.IRadioConfig/default")));
    ASSERT_NE(nullptr, radio_config.get());
}

ndk::ScopedAStatus RadioDataTest::getDataCallList() {
    serial = GetRandomSerialNumber();
    radio_data->getDataCallList(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    return ndk::ScopedAStatus::ok();
}

/*
 * Test IRadioData.setupDataCall() for the response returned.
 */
TEST_P(RadioDataTest, setupDataCall) {
    serial = GetRandomSerialNumber();

    AccessNetwork accessNetwork = AccessNetwork::EUTRAN;

    DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileInfo::ID_DEFAULT;
    dataProfileInfo.apn = std::string("internet");
    dataProfileInfo.protocol = PdpProtocolType::IP;
    dataProfileInfo.roamingProtocol = PdpProtocolType::IP;
    dataProfileInfo.authType = ApnAuthType::NO_PAP_NO_CHAP;
    dataProfileInfo.user = std::string("username");
    dataProfileInfo.password = std::string("password");
    dataProfileInfo.type = DataProfileInfo::TYPE_3GPP;
    dataProfileInfo.maxConnsTime = 300;
    dataProfileInfo.maxConns = 20;
    dataProfileInfo.waitTime = 0;
    dataProfileInfo.enabled = true;
    dataProfileInfo.supportedApnTypesBitmap =
            static_cast<int32_t>(ApnTypes::IMS) | static_cast<int32_t>(ApnTypes::IA);
    dataProfileInfo.bearerBitmap = static_cast<int32_t>(RadioAccessFamily::GPRS) |
                                   static_cast<int32_t>(RadioAccessFamily::EDGE) |
                                   static_cast<int32_t>(RadioAccessFamily::UMTS) |
                                   static_cast<int32_t>(RadioAccessFamily::HSDPA) |
                                   static_cast<int32_t>(RadioAccessFamily::HSUPA) |
                                   static_cast<int32_t>(RadioAccessFamily::HSPA) |
                                   static_cast<int32_t>(RadioAccessFamily::EHRPD) |
                                   static_cast<int32_t>(RadioAccessFamily::LTE) |
                                   static_cast<int32_t>(RadioAccessFamily::HSPAP) |
                                   static_cast<int32_t>(RadioAccessFamily::IWLAN);
    dataProfileInfo.mtuV4 = 0;
    dataProfileInfo.mtuV6 = 0;
    dataProfileInfo.preferred = true;
    dataProfileInfo.persistent = false;

    bool roamingAllowed = false;

    std::vector<LinkAddress> addresses = {};
    std::vector<std::string> dnses = {};

    DataRequestReason reason = DataRequestReason::NORMAL;
    SliceInfo sliceInfo;
    bool matchAllRuleAllowed = true;

    ndk::ScopedAStatus res =
            radio_data->setupDataCall(serial, accessNetwork, dataProfileInfo, roamingAllowed,
                                      reason, addresses, dnses, -1, sliceInfo, matchAllRuleAllowed);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    }
}

/*
 * Test IRadioData.setupDataCall() with osAppId for the response returned.
 */
TEST_P(RadioDataTest, setupDataCall_osAppId) {
    serial = GetRandomSerialNumber();

    AccessNetwork accessNetwork = AccessNetwork::EUTRAN;

    TrafficDescriptor trafficDescriptor;
    OsAppId osAppId;
    // hardcode osAppId for ENTERPRISE
    osAppId.osAppId = {static_cast<unsigned char>(-105), static_cast<unsigned char>(-92),
                       static_cast<unsigned char>(-104), static_cast<unsigned char>(-29),
                       static_cast<unsigned char>(-4),   static_cast<unsigned char>(-110),
                       static_cast<unsigned char>(92),   static_cast<unsigned char>(-108),
                       static_cast<unsigned char>(-119), static_cast<unsigned char>(-122),
                       static_cast<unsigned char>(3),    static_cast<unsigned char>(51),
                       static_cast<unsigned char>(-48),  static_cast<unsigned char>(110),
                       static_cast<unsigned char>(78),   static_cast<unsigned char>(71),
                       static_cast<unsigned char>(10),   static_cast<unsigned char>(69),
                       static_cast<unsigned char>(78),   static_cast<unsigned char>(84),
                       static_cast<unsigned char>(69),   static_cast<unsigned char>(82),
                       static_cast<unsigned char>(80),   static_cast<unsigned char>(82),
                       static_cast<unsigned char>(73),   static_cast<unsigned char>(83),
                       static_cast<unsigned char>(69)};
    trafficDescriptor.osAppId = osAppId;

    DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileInfo::ID_DEFAULT;
    dataProfileInfo.apn = std::string("internet");
    dataProfileInfo.protocol = PdpProtocolType::IP;
    dataProfileInfo.roamingProtocol = PdpProtocolType::IP;
    dataProfileInfo.authType = ApnAuthType::NO_PAP_NO_CHAP;
    dataProfileInfo.user = std::string("username");
    dataProfileInfo.password = std::string("password");
    dataProfileInfo.type = DataProfileInfo::TYPE_3GPP;
    dataProfileInfo.maxConnsTime = 300;
    dataProfileInfo.maxConns = 20;
    dataProfileInfo.waitTime = 0;
    dataProfileInfo.enabled = true;
    dataProfileInfo.supportedApnTypesBitmap =
            static_cast<int32_t>(ApnTypes::IMS) | static_cast<int32_t>(ApnTypes::IA);
    dataProfileInfo.bearerBitmap = static_cast<int32_t>(RadioAccessFamily::GPRS) |
                                   static_cast<int32_t>(RadioAccessFamily::EDGE) |
                                   static_cast<int32_t>(RadioAccessFamily::UMTS) |
                                   static_cast<int32_t>(RadioAccessFamily::HSDPA) |
                                   static_cast<int32_t>(RadioAccessFamily::HSUPA) |
                                   static_cast<int32_t>(RadioAccessFamily::HSPA) |
                                   static_cast<int32_t>(RadioAccessFamily::EHRPD) |
                                   static_cast<int32_t>(RadioAccessFamily::LTE) |
                                   static_cast<int32_t>(RadioAccessFamily::HSPAP) |
                                   static_cast<int32_t>(RadioAccessFamily::IWLAN);
    dataProfileInfo.mtuV4 = 0;
    dataProfileInfo.mtuV6 = 0;
    dataProfileInfo.preferred = true;
    dataProfileInfo.persistent = false;
    dataProfileInfo.trafficDescriptor = trafficDescriptor;

    bool roamingAllowed = false;

    std::vector<LinkAddress> addresses = {};
    std::vector<std::string> dnses = {};

    DataRequestReason reason = DataRequestReason::NORMAL;
    SliceInfo sliceInfo;
    bool matchAllRuleAllowed = true;

    ndk::ScopedAStatus res =
            radio_data->setupDataCall(serial, accessNetwork, dataProfileInfo, roamingAllowed,
                                      reason, addresses, dnses, -1, sliceInfo, matchAllRuleAllowed);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
        if (radioRsp_data->setupDataCallResult.trafficDescriptors.size() <= 0) {
            return;
        }
        EXPECT_EQ(trafficDescriptor.osAppId.value().osAppId,
                  radioRsp_data->setupDataCallResult.trafficDescriptors[0].osAppId.value().osAppId);
    }
}

/*
 * Test IRadioData.getSlicingConfig() for the response returned.
 */
TEST_P(RadioDataTest, getSlicingConfig) {
    serial = GetRandomSerialNumber();
    radio_data->getSlicingConfig(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::INTERNAL_ERR, RadioError::MODEM_ERR}));
    }
}

/*
 * Test IRadioData.setDataThrottling() for the response returned.
 */
TEST_P(RadioDataTest, setDataThrottling) {
    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_data->setDataThrottling(
            serial, DataThrottlingAction::THROTTLE_SECONDARY_CARRIER, 60000);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::REQUEST_NOT_SUPPORTED, RadioError::NONE}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR,
                                      RadioError::NONE, RadioError::INVALID_ARGUMENTS}));
    }

    sleep(1);
    serial = GetRandomSerialNumber();

    res = radio_data->setDataThrottling(serial, DataThrottlingAction::THROTTLE_ANCHOR_CARRIER,
                                        60000);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::REQUEST_NOT_SUPPORTED, RadioError::NONE}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR,
                                      RadioError::NONE, RadioError::INVALID_ARGUMENTS}));
    }

    sleep(1);
    serial = GetRandomSerialNumber();

    res = radio_data->setDataThrottling(serial, DataThrottlingAction::HOLD, 60000);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::REQUEST_NOT_SUPPORTED, RadioError::NONE}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR,
                                      RadioError::NONE, RadioError::INVALID_ARGUMENTS}));
    }

    sleep(1);
    serial = GetRandomSerialNumber();

    res = radio_data->setDataThrottling(serial, DataThrottlingAction::NO_DATA_THROTTLING, 60000);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::REQUEST_NOT_SUPPORTED, RadioError::NONE}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR,
                                      RadioError::NONE, RadioError::INVALID_ARGUMENTS}));
    }

    sleep(1);
}

/*
 * Test IRadioData.setInitialAttachApn() for the response returned.
 */
TEST_P(RadioDataTest, setInitialAttachApn) {
    serial = GetRandomSerialNumber();

    // Create a dataProfileInfo
    DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileInfo::ID_DEFAULT;
    dataProfileInfo.apn = std::string("internet");
    dataProfileInfo.protocol = PdpProtocolType::IPV4V6;
    dataProfileInfo.roamingProtocol = PdpProtocolType::IPV4V6;
    dataProfileInfo.authType = ApnAuthType::NO_PAP_NO_CHAP;
    dataProfileInfo.user = std::string("username");
    dataProfileInfo.password = std::string("password");
    dataProfileInfo.type = DataProfileInfo::TYPE_3GPP;
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

    radio_data->setInitialAttachApn(serial, dataProfileInfo);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE}));
    }
}

/*
 * Test IRadioData.setDataProfile() for the response returned.
 */
TEST_P(RadioDataTest, setDataProfile) {
    serial = GetRandomSerialNumber();

    // Create a dataProfileInfo
    DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileInfo::ID_DEFAULT;
    dataProfileInfo.apn = std::string("internet");
    dataProfileInfo.protocol = PdpProtocolType::IPV4V6;
    dataProfileInfo.roamingProtocol = PdpProtocolType::IPV4V6;
    dataProfileInfo.authType = ApnAuthType::NO_PAP_NO_CHAP;
    dataProfileInfo.user = std::string("username");
    dataProfileInfo.password = std::string("password");
    dataProfileInfo.type = DataProfileInfo::TYPE_3GPP;
    dataProfileInfo.maxConnsTime = 300;
    dataProfileInfo.maxConns = 20;
    dataProfileInfo.waitTime = 0;
    dataProfileInfo.enabled = true;
    dataProfileInfo.supportedApnTypesBitmap = 320;
    dataProfileInfo.bearerBitmap = 161543;
    dataProfileInfo.mtuV4 = 0;
    dataProfileInfo.mtuV6 = 0;
    dataProfileInfo.preferred = true;
    dataProfileInfo.persistent = true;

    // Create a dataProfileInfoList
    std::vector<DataProfileInfo> dataProfileInfoList = {dataProfileInfo};

    radio_data->setDataProfile(serial, dataProfileInfoList);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE}));
    }
}

/*
 * Test IRadioData.deactivateDataCall() for the response returned.
 */
TEST_P(RadioDataTest, deactivateDataCall) {
    serial = GetRandomSerialNumber();
    int cid = 1;
    DataRequestReason reason = DataRequestReason::NORMAL;

    ndk::ScopedAStatus res = radio_data->deactivateDataCall(serial, cid, reason);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                                 {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::INVALID_CALL_ID, RadioError::INVALID_STATE,
                                  RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED,
                                  RadioError::CANCELLED, RadioError::SIM_ABSENT}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_data->rspInfo.error,
                {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INVALID_CALL_ID,
                 RadioError::INVALID_STATE, RadioError::INVALID_ARGUMENTS,
                 RadioError::REQUEST_NOT_SUPPORTED, RadioError::CANCELLED}));
    }
}

/*
 * Test IRadioData.startKeepalive() for the response returned.
 */
TEST_P(RadioDataTest, startKeepalive) {
    std::vector<KeepaliveRequest> requests = {
            {
                    // Invalid IPv4 source address
                    KeepaliveRequest::TYPE_NATT_IPV4,
                    {192, 168, 0 /*, 100*/},
                    1234,
                    {8, 8, 4, 4},
                    4500,
                    20000,
                    0xBAD,
            },
            {
                    // Invalid IPv4 destination address
                    KeepaliveRequest::TYPE_NATT_IPV4,
                    {192, 168, 0, 100},
                    1234,
                    {8, 8, 4, 4, 1, 2, 3, 4},
                    4500,
                    20000,
                    0xBAD,
            },
            {
                    // Invalid Keepalive Type
                    -1,
                    {192, 168, 0, 100},
                    1234,
                    {8, 8, 4, 4},
                    4500,
                    20000,
                    0xBAD,
            },
            {
                    // Invalid IPv6 source address
                    KeepaliveRequest::TYPE_NATT_IPV6,
                    {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE,
                     0xED, 0xBE, 0xEF, 0xBD},
                    1234,
                    {0x20, 0x01, 0x48, 0x60, 0x48, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x88, 0x44},
                    4500,
                    20000,
                    0xBAD,
            },
            {
                    // Invalid IPv6 destination address
                    KeepaliveRequest::TYPE_NATT_IPV6,
                    {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE,
                     0xED, 0xBE, 0xEF},
                    1234,
                    {0x20, 0x01, 0x48, 0x60, 0x48, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x88,
                     /*0x44*/},
                    4500,
                    20000,
                    0xBAD,
            },
            {
                    // Invalid Context ID (cid), this should survive the initial
                    // range checking and fail in the modem data layer
                    KeepaliveRequest::TYPE_NATT_IPV4,
                    {192, 168, 0, 100},
                    1234,
                    {8, 8, 4, 4},
                    4500,
                    20000,
                    0xBAD,
            },
            {
                    // Invalid Context ID (cid), this should survive the initial
                    // range checking and fail in the modem data layer
                    KeepaliveRequest::TYPE_NATT_IPV6,
                    {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE,
                     0xED, 0xBE, 0xEF},
                    1234,
                    {0x20, 0x01, 0x48, 0x60, 0x48, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                     0x00, 0x88, 0x44},
                    4500,
                    20000,
                    0xBAD,
            }};

    for (auto req = requests.begin(); req != requests.end(); req++) {
        serial = GetRandomSerialNumber();
        radio_data->startKeepalive(serial, *req);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);

        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_data->rspInfo.error,
                {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INVALID_ARGUMENTS,
                 RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadioData.stopKeepalive() for the response returned.
 */
TEST_P(RadioDataTest, stopKeepalive) {
    serial = GetRandomSerialNumber();

    radio_data->stopKeepalive(serial, 0xBAD);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);

    ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_data->rspInfo.error,
                             {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                              RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioData.getDataCallList() for the response returned.
 */
TEST_P(RadioDataTest, getDataCallList) {
    LOG(DEBUG) << "getDataCallList";
    serial = GetRandomSerialNumber();

    radio_data->getDataCallList(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_data->rspInfo.error,
                {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::SIM_ABSENT}));
    }
    LOG(DEBUG) << "getDataCallList finished";
}

/*
 * Test IRadioData.setDataAllowed() for the response returned.
 */
TEST_P(RadioDataTest, setDataAllowed) {
    LOG(DEBUG) << "setDataAllowed";
    serial = GetRandomSerialNumber();
    bool allow = true;

    radio_data->setDataAllowed(serial, allow);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_data->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_data->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp_data->rspInfo.error);
    }
    LOG(DEBUG) << "setDataAllowed finished";
}
