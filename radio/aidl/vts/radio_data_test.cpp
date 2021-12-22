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

#include <aidl/android/hardware/radio/config/IRadioConfig.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <algorithm>

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

    // Assert IRadioConfig exists before testing
    std::shared_ptr<aidl::android::hardware::radio::config::IRadioConfig> radioConfig =
            aidl::android::hardware::radio::config::IRadioConfig::fromBinder(
                    ndk::SpAIBinder(AServiceManager_waitForService(
                            "android.hardware.radio.config.IRadioConfig/default")));
    ASSERT_NE(nullptr, radioConfig.get());
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
    dataProfileInfo.type = DataProfileInfo::TYPE_THREE_GPP;
    dataProfileInfo.maxConnsTime = 300;
    dataProfileInfo.maxConns = 20;
    dataProfileInfo.waitTime = 0;
    dataProfileInfo.enabled = true;
    // TODO(b/210712359): 320 was the previous value; need to support bitmaps
    dataProfileInfo.supportedApnTypesBitmap = ApnTypes::DEFAULT;
    // TODO(b/210712359): 161543 was the previous value; need to support bitmaps
    dataProfileInfo.bearerBitmap = RadioAccessFamily::LTE;
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
    std::string osAppIdString("osAppId");
    // TODO(b/210712359): there should be a cleaner way to convert this
    std::vector<unsigned char> output(osAppIdString.length());
    std::transform(osAppIdString.begin(), osAppIdString.end(), output.begin(),
                   [](char c) { return static_cast<unsigned char>(c); });
    osAppId.osAppId = output;
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
    dataProfileInfo.type = DataProfileInfo::TYPE_THREE_GPP;
    dataProfileInfo.maxConnsTime = 300;
    dataProfileInfo.maxConns = 20;
    dataProfileInfo.waitTime = 0;
    dataProfileInfo.enabled = true;
    // TODO(b/210712359): 320 was the previous value; need to support bitmaps
    dataProfileInfo.supportedApnTypesBitmap = ApnTypes::DEFAULT;
    // TODO(b/210712359): 161543 was the previous value; need to support bitmaps
    dataProfileInfo.bearerBitmap = RadioAccessFamily::LTE;
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
