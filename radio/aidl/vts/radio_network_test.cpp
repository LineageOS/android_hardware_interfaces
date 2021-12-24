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
    RadioAccessFamily allowedNetworkTypesBitmap = RadioAccessFamily::LTE;

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
    RadioAccessFamily allowedNetworkTypesBitmap = RadioAccessFamily::LTE;

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

void RadioNetworkTest::invokeAndExpectResponse(
        std::function<ndk::ScopedAStatus(int32_t serial)> request,
        std::vector<RadioError> errors_to_check) {
    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = request(serial);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, errors_to_check));
}

/*
 * Test IRadioNetwork.getUsageSetting()
 *
 * Verify that the usage setting can be retrieved.
 */
TEST_P(RadioNetworkTest, getUsageSetting) {
    invokeAndExpectResponse([&](int serial) { return radio_network->getUsageSetting(serial); },
                            {RadioError::RADIO_NOT_AVAILABLE, RadioError::INVALID_STATE,
                             RadioError::SIM_ABSENT, RadioError::INTERNAL_ERR, RadioError::NONE});

    ASSERT_TRUE(radioRsp_network->usageSetting == UsageSetting::VOICE_CENTRIC ||
                radioRsp_network->usageSetting == UsageSetting::DATA_CENTRIC);
}

void RadioNetworkTest::testSetUsageSetting_InvalidValues(std::vector<RadioError> errors) {
    invokeAndExpectResponse(
            [&](int serial) {
                return radio_network->setUsageSetting(serial,
                                                      UsageSetting(0) /*below valid range*/);
            },
            errors);
    invokeAndExpectResponse(
            [&](int serial) {
                return radio_network->setUsageSetting(serial, UsageSetting(-1) /*negative*/);
            },
            errors);
    invokeAndExpectResponse(
            [&](int serial) {
                return radio_network->setUsageSetting(serial,
                                                      UsageSetting(3) /*above valid range*/);
            },
            errors);
}

/*
 * Test IRadioNetwork.setUsageSetting() and IRadioNetwork.getUsageSetting()
 *
 * Verify the following:
 * -That the usage setting can be retrieved.
 * -That the usage setting can be successfully set to allowed values.
 * -That the usage setting cannot be set to invalid values.
 */
TEST_P(RadioNetworkTest, setUsageSetting) {
    invokeAndExpectResponse([&](int serial) { return radio_network->getUsageSetting(serial); },
                            {RadioError::RADIO_NOT_AVAILABLE, RadioError::INVALID_STATE,
                             RadioError::SIM_ABSENT, RadioError::INTERNAL_ERR, RadioError::NONE});

    if (radioRsp_network->rspInfo.error != RadioError::NONE) {
        // Test only for invalid values, with the only allowable response being the same error
        // that was previously provided, or an error indicating invalid arguments.
        testSetUsageSetting_InvalidValues(
                {radioRsp_network->rspInfo.error, RadioError::INVALID_ARGUMENTS});
        // It is unsafe to proceed with setting valid values without knowing the starting value, but
        // we expect errors anyway, so not necessary.
        return;
    } else {
        // Because querying succeeded, the device is in a valid state to test for invalid values
        // and the only thing that can change is that we expect to have an EINVAL return each time.
        testSetUsageSetting_InvalidValues({RadioError::INVALID_ARGUMENTS});
    }

    // Store the original setting value to reset later.
    const UsageSetting originalSetting = radioRsp_network->usageSetting;

    // Choose the "other" value that is not the current value for test.
    const UsageSetting testSetting = radioRsp_network->usageSetting == UsageSetting::VOICE_CENTRIC
                                             ? UsageSetting::DATA_CENTRIC
                                             : UsageSetting::VOICE_CENTRIC;

    // Set an alternative setting; it may either succeed or be disallowed as out of range for
    // the current device (if the device only supports its current mode).
    invokeAndExpectResponse(
            [&](int serial) { return radio_network->setUsageSetting(serial, testSetting); },
            {RadioError::INVALID_ARGUMENTS, RadioError::NONE});

    // If there was no error, then we expect the test setting to be set, or if there is an error
    // we expect the original setting to be maintained.
    const UsageSetting expectedSetting =
            radioRsp_network->rspInfo.error == RadioError::NONE ? testSetting : originalSetting;
    invokeAndExpectResponse([&](int serial) { return radio_network->getUsageSetting(serial); },
                            {RadioError::NONE});

    const UsageSetting updatedSetting = radioRsp_network->usageSetting;

    // Re-set the original setting, which must always succeed.
    invokeAndExpectResponse(
            [&](int serial) { return radio_network->setUsageSetting(serial, originalSetting); },
            {RadioError::NONE});

    // Check that indeed the updated setting was set. We do this after resetting to original
    // conditions to avoid early-exiting the test and leaving the device in a modified state.
    ASSERT_TRUE(expectedSetting == updatedSetting);
    // Check that indeed the original setting was reset.
    ASSERT_TRUE(originalSetting == radioRsp_network->usageSetting);
}
