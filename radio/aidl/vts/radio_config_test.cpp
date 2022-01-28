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

#include <android-base/logging.h>
#include <android/binder_manager.h>

#include "radio_config_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioConfigTest::SetUp() {
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_config = IRadioConfig::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_config.get());

    radioRsp_config = ndk::SharedRefBase::make<RadioConfigResponse>(*this);
    ASSERT_NE(nullptr, radioRsp_config.get());

    count_ = 0;

    radioInd_config = ndk::SharedRefBase::make<RadioConfigIndication>(*this);
    ASSERT_NE(nullptr, radioInd_config.get());

    radio_config->setResponseFunctions(radioRsp_config, radioInd_config);
}

/*
 * Test IRadioConfig.getHalDeviceCapabilities() for the response returned.
 */
TEST_P(RadioConfigTest, getHalDeviceCapabilities) {
    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = radio_config->getHalDeviceCapabilities(serial);
    ASSERT_OK(res);
    ALOGI("getHalDeviceCapabilities, rspInfo.error = %s\n",
          toString(radioRsp_config->rspInfo.error).c_str());
}

/*
 * Test IRadioConfig.getSimSlotsStatus() for the response returned.
 */
TEST_P(RadioConfigTest, getSimSlotsStatus) {
    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = radio_config->getSimSlotsStatus(serial);
    ASSERT_OK(res);
    ALOGI("getSimSlotsStatus, rspInfo.error = %s\n",
          toString(radioRsp_config->rspInfo.error).c_str());
}

/*
 * Test IRadioConfig.getPhoneCapability() for the response returned.
 */
TEST_P(RadioConfigTest, getPhoneCapability) {
    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = radio_config->getPhoneCapability(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_config->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_config->rspInfo.serial);
    ALOGI("getPhoneCapability, rspInfo.error = %s\n",
          toString(radioRsp_config->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_config->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));

    if (radioRsp_config->rspInfo.error == RadioError ::NONE) {
        // maxActiveData should be greater than or equal to maxActiveInternetData.
        EXPECT_GE(radioRsp_config->phoneCap.maxActiveData,
                  radioRsp_config->phoneCap.maxActiveInternetData);
        // maxActiveData and maxActiveInternetData should be 0 or positive numbers.
        EXPECT_GE(radioRsp_config->phoneCap.maxActiveInternetData, 0);
    }
}

/*
 * Test IRadioConfig.setPreferredDataModem() for the response returned.
 */
TEST_P(RadioConfigTest, setPreferredDataModem) {
    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = radio_config->getPhoneCapability(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_config->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_config->rspInfo.serial);
    ALOGI("getPhoneCapability, rspInfo.error = %s\n",
          toString(radioRsp_config->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_config->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));

    if (radioRsp_config->rspInfo.error != RadioError ::NONE) {
        return;
    }

    if (radioRsp_config->phoneCap.logicalModemIds.size() == 0) {
        return;
    }

    // We get phoneCapability. Send setPreferredDataModem command
    serial = GetRandomSerialNumber();
    uint8_t modemId = radioRsp_config->phoneCap.logicalModemIds[0];
    res = radio_config->setPreferredDataModem(serial, modemId);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_config->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_config->rspInfo.serial);
    ALOGI("setPreferredDataModem, rspInfo.error = %s\n",
          toString(radioRsp_config->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_config->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));
}

/*
 * Test IRadioConfig.setPreferredDataModem() with invalid arguments.
 */
TEST_P(RadioConfigTest, setPreferredDataModem_invalidArgument) {
    serial = GetRandomSerialNumber();
    uint8_t modemId = -1;
    ndk::ScopedAStatus res = radio_config->setPreferredDataModem(serial, modemId);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_config->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_config->rspInfo.serial);
    ALOGI("setPreferredDataModem, rspInfo.error = %s\n",
          toString(radioRsp_config->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_config->rspInfo.error,
                                 {RadioError::INVALID_ARGUMENTS, RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::INTERNAL_ERR}));
}
