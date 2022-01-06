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
#include <android-base/logging.h>
#include <android/binder_manager.h>

#include "radio_network_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioNetworkTest::SetUp() {
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_network = IRadioNetwork::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_network.get());

    radioRsp_network = ndk::SharedRefBase::make<RadioNetworkResponse>(*this);
    ASSERT_NE(nullptr, radioRsp_network.get());

    count_ = 0;

    radioInd_network = ndk::SharedRefBase::make<RadioNetworkIndication>(*this);
    ASSERT_NE(nullptr, radioInd_network.get());

    radio_network->setResponseFunctions(radioRsp_network, radioInd_network);

    // Assert IRadioConfig exists before testing
    std::shared_ptr<aidl::android::hardware::radio::config::IRadioConfig> radioConfig =
            aidl::android::hardware::radio::config::IRadioConfig::fromBinder(
                    ndk::SpAIBinder(AServiceManager_waitForService(
                            "android.hardware.radio.config.IRadioConfig/default")));
    ASSERT_NE(nullptr, radioConfig.get());
}

/*
 * Test IRadioNetwork.setAllowedNetworkTypesBitmap for the response returned.
 */
TEST_P(RadioNetworkTest, setAllowedNetworkTypesBitmap) {
    serial = GetRandomSerialNumber();
    int32_t allowedNetworkTypesBitmap = static_cast<int32_t>(RadioAccessFamily::LTE);

    radio_network->setAllowedNetworkTypesBitmap(serial, allowedNetworkTypesBitmap);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::OPERATION_NOT_ALLOWED,
             RadioError::MODE_NOT_SUPPORTED, RadioError::INTERNAL_ERR, RadioError::MODEM_ERR,
             RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED,
             RadioError::NO_RESOURCES}));
}

/*
 * Test IRadioNetwork.getAllowedNetworkTypesBitmap for the response returned.
 */
TEST_P(RadioNetworkTest, getAllowedNetworkTypesBitmap) {
    serial = GetRandomSerialNumber();
    int32_t allowedNetworkTypesBitmap = static_cast<int32_t>(RadioAccessFamily::LTE);

    radio_network->setAllowedNetworkTypesBitmap(serial, allowedNetworkTypesBitmap);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (radioRsp_network->rspInfo.error == RadioError::NONE) {
        sleep(3);  // wait for modem
        serial = GetRandomSerialNumber();
        radio_network->getAllowedNetworkTypesBitmap(serial);

        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR,
                 RadioError::OPERATION_NOT_ALLOWED, RadioError::MODE_NOT_SUPPORTED,
                 RadioError::INVALID_ARGUMENTS, RadioError::MODEM_ERR,
                 RadioError::REQUEST_NOT_SUPPORTED, RadioError::NO_RESOURCES}));
    }
}

/*
 * Test IRadioNetwork.setNrDualConnectivityState() for the response returned.
 */
TEST_P(RadioNetworkTest, setNrDualConnectivityState) {
    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res =
            radio_network->setNrDualConnectivityState(serial, NrDualConnectivityState::DISABLE);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR,
                 RadioError::INVALID_STATE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::NONE}));
    }
}

/*
 * Test IRadioNetwork.isNrDualConnectivityEnabled() for the response returned.
 */
TEST_P(RadioNetworkTest, isNrDualConnectivityEnabled) {
    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->isNrDualConnectivityEnabled(serial);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    if (getRadioHalCapabilities()) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::REQUEST_NOT_SUPPORTED}));
    } else {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR, RadioError::NONE}));
    }
}