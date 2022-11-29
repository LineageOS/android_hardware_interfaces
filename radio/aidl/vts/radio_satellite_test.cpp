/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "radio_satellite_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(((ret).isOk()))

void RadioSatelliteTest::SetUp() {
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the radio satellite tests due to device configuration.");
        GTEST_SKIP();
    }

    satellite = IRadioSatellite::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, satellite.get());

    rsp_satellite = ndk::SharedRefBase::make<RadioSatelliteResponse>(*this);
    ASSERT_NE(nullptr, rsp_satellite.get());

    count_ = 0;

    ind_satellite = ndk::SharedRefBase::make<RadioSatelliteIndication>(*this);
    ASSERT_NE(nullptr, ind_satellite.get());

    satellite->setResponseFunctions(rsp_satellite, ind_satellite);

    // Assert IRadioConfig exists before testing
    radio_config = config::IRadioConfig::fromBinder(ndk::SpAIBinder(
            AServiceManager_waitForService("android.hardware.radio.config.IRadioConfig/default")));
    ASSERT_NE(nullptr, radio_config.get());
}

/*
 * Test IRadioSatellite.getCapabilities() for the response returned.
 */
TEST_P(RadioSatelliteTest, getCapabilities) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping getCapabilities because satellite is not supported in device");
        return;
    } else {
        ALOGI("Running getCapabilities because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = satellite->getCapabilities(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("getCapabilities, rspInfo.error = %s\n", toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            rsp_satellite->rspInfo.error,
            {RadioError::NONE, RadioError::INTERNAL_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_MODEM_STATE, RadioError::INVALID_SIM_STATE,
             RadioError::INVALID_STATE, RadioError::MODEM_ERR, RadioError::NO_MEMORY,
             RadioError::NO_RESOURCES, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::REQUEST_RATE_LIMITED,
             RadioError::SYSTEM_ERR}));
}

/*
 * Test IRadioSatellite.setPower() for the response returned.
 */
TEST_P(RadioSatelliteTest, setPower) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping setPower because satellite is not supported in device");
        return;
    } else {
        ALOGI("Running setPower because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = satellite->setPower(serial, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("setPower, rspInfo.error = %s\n", toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            rsp_satellite->rspInfo.error,
            {RadioError::NONE, RadioError::INTERNAL_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_MODEM_STATE, RadioError::INVALID_SIM_STATE,
             RadioError::INVALID_STATE, RadioError::MODEM_ERR, RadioError::NO_MEMORY,
             RadioError::NO_RESOURCES, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::REQUEST_RATE_LIMITED,
             RadioError::SYSTEM_ERR}));
}

/*
 * Test IRadioSatellite.getPowerSate() for the response returned.
 */
TEST_P(RadioSatelliteTest, getPowerSate) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping getPowerSate because satellite is not supported in device");
        return;
    } else {
        ALOGI("Running getPowerSate because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = satellite->getPowerState(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("getPowerSate, rspInfo.error = %s\n", toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            rsp_satellite->rspInfo.error,
            {RadioError::NONE, RadioError::INTERNAL_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_MODEM_STATE, RadioError::INVALID_SIM_STATE,
             RadioError::INVALID_STATE, RadioError::MODEM_ERR, RadioError::NO_MEMORY,
             RadioError::NO_RESOURCES, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::REQUEST_RATE_LIMITED,
             RadioError::SYSTEM_ERR}));
}

/*
 * Test IRadioSatellite.provisionService() for the response returned.
 */
TEST_P(RadioSatelliteTest, provisionService) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping provisionService because satellite is not supported in device");
        return;
    } else {
        ALOGI("Running provisionService because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    std::string imei = "imei";
    std::string msisdn = "msisdn";
    std::string imsi = "imsi";
    const std::vector<SatelliteFeature> features{
            SatelliteFeature::SOS_SMS, SatelliteFeature::EMERGENCY_SMS, SatelliteFeature::SMS};
    ndk::ScopedAStatus res = satellite->provisionService(serial, imei, msisdn, imsi, features);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("provisionService, rspInfo.error = %s\n", toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(rsp_satellite->rspInfo.error,
                                 {RadioError::NONE,
                                  RadioError::ABORTED,
                                  RadioError::ACCESS_BARRED,
                                  RadioError::CANCELLED,
                                  RadioError::FEATURE_NOT_SUPPORTED,
                                  RadioError::INTERNAL_ERR,
                                  RadioError::INVALID_ARGUMENTS,
                                  RadioError::INVALID_MODEM_STATE,
                                  RadioError::INVALID_SIM_STATE,
                                  RadioError::INVALID_STATE,
                                  RadioError::MODEM_ERR,
                                  RadioError::MODEM_INCOMPATIBLE,
                                  RadioError::NETWORK_ERR,
                                  RadioError::NETWORK_NOT_READY,
                                  RadioError::NETWORK_REJECT,
                                  RadioError::NETWORK_TIMEOUT,
                                  RadioError::NO_MEMORY,
                                  RadioError::NO_NETWORK_FOUND,
                                  RadioError::NO_RESOURCES,
                                  RadioError::NO_SATELLITE_SIGNAL,
                                  RadioError::NO_SUBSCRIPTION,
                                  RadioError::OPERATION_NOT_ALLOWED,
                                  RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::RADIO_TECHNOLOGY_NOT_SUPPORTED,
                                  RadioError::REQUEST_NOT_SUPPORTED,
                                  RadioError::REQUEST_RATE_LIMITED,
                                  RadioError::SIM_ABSENT,
                                  RadioError::SIM_BUSY,
                                  RadioError::SIM_ERR,
                                  RadioError::SIM_FULL,
                                  RadioError::SUBSCRIBER_NOT_AUTHORIZED,
                                  RadioError::SYSTEM_ERR}));
}

/*
 * Test IRadioSatellite.addAllowedSatelliteContacts() for the response returned.
 */
TEST_P(RadioSatelliteTest, addAllowedSatelliteContacts) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping addAllowedSatelliteContacts because satellite is not supported in device");
        return;
    } else {
        ALOGI("Running addAllowedSatelliteContacts because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    const std::vector<std::string> contacts = {"contact 1", "contact 2"};
    ndk::ScopedAStatus res = satellite->addAllowedSatelliteContacts(serial, contacts);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("addAllowedSatelliteContacts, rspInfo.error = %s\n",
          toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(rsp_satellite->rspInfo.error,
                                 {RadioError::NONE,
                                  RadioError::ABORTED,
                                  RadioError::ACCESS_BARRED,
                                  RadioError::CANCELLED,
                                  RadioError::INTERNAL_ERR,
                                  RadioError::INVALID_ARGUMENTS,
                                  RadioError::INVALID_CONTACT,
                                  RadioError::INVALID_MODEM_STATE,
                                  RadioError::INVALID_SIM_STATE,
                                  RadioError::INVALID_STATE,
                                  RadioError::MODEM_ERR,
                                  RadioError::NETWORK_ERR,
                                  RadioError::NETWORK_NOT_READY,
                                  RadioError::NETWORK_REJECT,
                                  RadioError::NETWORK_TIMEOUT,
                                  RadioError::NO_MEMORY,
                                  RadioError::NO_NETWORK_FOUND,
                                  RadioError::NO_RESOURCES,
                                  RadioError::NO_SATELLITE_SIGNAL,
                                  RadioError::NO_SUBSCRIPTION,
                                  RadioError::NOT_SUFFICIENT_ACCOUNT_BALANCE,
                                  RadioError::OPERATION_NOT_ALLOWED,
                                  RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::REQUEST_NOT_SUPPORTED,
                                  RadioError::REQUEST_RATE_LIMITED,
                                  RadioError::SIM_ABSENT,
                                  RadioError::SIM_BUSY,
                                  RadioError::SIM_ERR,
                                  RadioError::SIM_FULL,
                                  RadioError::SYSTEM_ERR,
                                  RadioError::UNIDENTIFIED_SUBSCRIBER}));
}

/*
 * Test IRadioSatellite.removeAllowedSatelliteContacts() for the response returned.
 */
TEST_P(RadioSatelliteTest, removeAllowedSatelliteContacts) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping removeAllowedSatelliteContacts because satellite is not supported in "
              "device");
        return;
    } else {
        ALOGI("Running removeAllowedSatelliteContacts because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    const std::vector<std::string> contacts = {"contact 1", "contact 2"};
    ndk::ScopedAStatus res = satellite->removeAllowedSatelliteContacts(serial, contacts);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("removeAllowedSatelliteContacts, rspInfo.error = %s\n",
          toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(rsp_satellite->rspInfo.error,
                                 {RadioError::NONE,
                                  RadioError::ABORTED,
                                  RadioError::ACCESS_BARRED,
                                  RadioError::CANCELLED,
                                  RadioError::INTERNAL_ERR,
                                  RadioError::INVALID_ARGUMENTS,
                                  RadioError::INVALID_CONTACT,
                                  RadioError::INVALID_MODEM_STATE,
                                  RadioError::INVALID_SIM_STATE,
                                  RadioError::INVALID_STATE,
                                  RadioError::MODEM_ERR,
                                  RadioError::NETWORK_ERR,
                                  RadioError::NETWORK_NOT_READY,
                                  RadioError::NETWORK_REJECT,
                                  RadioError::NETWORK_TIMEOUT,
                                  RadioError::NO_MEMORY,
                                  RadioError::NO_NETWORK_FOUND,
                                  RadioError::NO_RESOURCES,
                                  RadioError::NO_SATELLITE_SIGNAL,
                                  RadioError::NO_SUBSCRIPTION,
                                  RadioError::NOT_SUFFICIENT_ACCOUNT_BALANCE,
                                  RadioError::OPERATION_NOT_ALLOWED,
                                  RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::REQUEST_NOT_SUPPORTED,
                                  RadioError::REQUEST_RATE_LIMITED,
                                  RadioError::SIM_ABSENT,
                                  RadioError::SIM_BUSY,
                                  RadioError::SIM_ERR,
                                  RadioError::SIM_FULL,
                                  RadioError::SYSTEM_ERR,
                                  RadioError::UNIDENTIFIED_SUBSCRIBER}));
}

/*
 * Test IRadioSatellite.sendMessages() for the response returned.
 */
TEST_P(RadioSatelliteTest, sendMessages) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping sendMessages because satellite is not supported in device");
        return;
    } else {
        ALOGI("Running sendMessages because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    const std::vector<std::string> messages = {"message 1", "message 2"};
    std::string destination = "0123456789";
    ndk::ScopedAStatus res = satellite->sendMessages(serial, messages, destination, 1.0, 2.0);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("sendMessages, rspInfo.error = %s\n", toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(rsp_satellite->rspInfo.error,
                                 {RadioError::NONE,
                                  RadioError::ABORTED,
                                  RadioError::ACCESS_BARRED,
                                  RadioError::BLOCKED_DUE_TO_CALL,
                                  RadioError::CANCELLED,
                                  RadioError::ENCODING_ERR,
                                  RadioError::ENCODING_NOT_SUPPORTED,
                                  RadioError::INTERNAL_ERR,
                                  RadioError::INVALID_ARGUMENTS,
                                  RadioError::INVALID_MODEM_STATE,
                                  RadioError::INVALID_SIM_STATE,
                                  RadioError::INVALID_SMS_FORMAT,
                                  RadioError::INVALID_STATE,
                                  RadioError::MODEM_ERR,
                                  RadioError::NETWORK_ERR,
                                  RadioError::NETWORK_NOT_READY,
                                  RadioError::NETWORK_REJECT,
                                  RadioError::NETWORK_TIMEOUT,
                                  RadioError::NO_MEMORY,
                                  RadioError::NO_NETWORK_FOUND,
                                  RadioError::NO_RESOURCES,
                                  RadioError::NO_SMS_TO_ACK,
                                  RadioError::NO_SATELLITE_SIGNAL,
                                  RadioError::NO_SUBSCRIPTION,
                                  RadioError::NOT_SUFFICIENT_ACCOUNT_BALANCE,
                                  RadioError::OPERATION_NOT_ALLOWED,
                                  RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::REQUEST_NOT_SUPPORTED,
                                  RadioError::REQUEST_RATE_LIMITED,
                                  RadioError::SIM_ABSENT,
                                  RadioError::SIM_BUSY,
                                  RadioError::SIM_ERR,
                                  RadioError::SIM_FULL,
                                  RadioError::SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED,
                                  RadioError::SMS_SEND_FAIL_RETRY,
                                  RadioError::SYSTEM_ERR,
                                  RadioError::SWITCHED_FROM_SATELLITE_TO_TERRESTRIAL,
                                  RadioError::UNIDENTIFIED_SUBSCRIBER}));
}

/*
 * Test IRadioSatellite.getPendingMessages() for the response returned.
 */
TEST_P(RadioSatelliteTest, getPendingMessages) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping getPendingMessages because satellite is not supported in device");
        return;
    } else {
        ALOGI("Running getPendingMessages because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = satellite->getPendingMessages(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("getPendingMessages, rspInfo.error = %s\n",
          toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(rsp_satellite->rspInfo.error,
                                 {RadioError::NONE,
                                  RadioError::ABORTED,
                                  RadioError::ACCESS_BARRED,
                                  RadioError::BLOCKED_DUE_TO_CALL,
                                  RadioError::CANCELLED,
                                  RadioError::ENCODING_ERR,
                                  RadioError::ENCODING_NOT_SUPPORTED,
                                  RadioError::INTERNAL_ERR,
                                  RadioError::INVALID_ARGUMENTS,
                                  RadioError::INVALID_MODEM_STATE,
                                  RadioError::INVALID_SIM_STATE,
                                  RadioError::INVALID_SMS_FORMAT,
                                  RadioError::INVALID_STATE,
                                  RadioError::MODEM_ERR,
                                  RadioError::NETWORK_ERR,
                                  RadioError::NETWORK_NOT_READY,
                                  RadioError::NETWORK_REJECT,
                                  RadioError::NETWORK_TIMEOUT,
                                  RadioError::NO_MEMORY,
                                  RadioError::NO_NETWORK_FOUND,
                                  RadioError::NO_RESOURCES,
                                  RadioError::NO_SMS_TO_ACK,
                                  RadioError::NO_SATELLITE_SIGNAL,
                                  RadioError::NO_SUBSCRIPTION,
                                  RadioError::NOT_SUFFICIENT_ACCOUNT_BALANCE,
                                  RadioError::OPERATION_NOT_ALLOWED,
                                  RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::REQUEST_NOT_SUPPORTED,
                                  RadioError::REQUEST_RATE_LIMITED,
                                  RadioError::SIM_ABSENT,
                                  RadioError::SIM_BUSY,
                                  RadioError::SIM_ERR,
                                  RadioError::SIM_FULL,
                                  RadioError::SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED,
                                  RadioError::SYSTEM_ERR,
                                  RadioError::SWITCHED_FROM_SATELLITE_TO_TERRESTRIAL}));
}

/*
 * Test IRadioSatellite.getSatelliteMode() for the response returned.
 */
TEST_P(RadioSatelliteTest, getSatelliteMode) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping getSatelliteMode because satellite is not supported in device");
        return;
    } else {
        ALOGI("Running getSatelliteMode because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = satellite->getSatelliteMode(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("getSatelliteMode, rspInfo.error = %s\n", toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            rsp_satellite->rspInfo.error,
            {RadioError::NONE, RadioError::INTERNAL_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_MODEM_STATE, RadioError::INVALID_SIM_STATE,
             RadioError::INVALID_STATE, RadioError::MODEM_ERR, RadioError::NO_MEMORY,
             RadioError::NO_RESOURCES, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::REQUEST_RATE_LIMITED,
             RadioError::SYSTEM_ERR}));
}

/*
 * Test IRadioSatellite.setIndicationFilter() for the response returned.
 */
TEST_P(RadioSatelliteTest, setIndicationFilter) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping setIndicationFilter because satellite is not supported in device");
        return;
    } else {
        ALOGI("Running setIndicationFilter because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = satellite->setIndicationFilter(serial, 0);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("setIndicationFilter, rspInfo.error = %s\n",
          toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            rsp_satellite->rspInfo.error,
            {RadioError::NONE, RadioError::INTERNAL_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_MODEM_STATE, RadioError::INVALID_SIM_STATE,
             RadioError::INVALID_STATE, RadioError::MODEM_ERR, RadioError::NO_MEMORY,
             RadioError::NO_RESOURCES, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::REQUEST_RATE_LIMITED,
             RadioError::SYSTEM_ERR}));
}

/*
 * Test IRadioSatellite.startSendingSatellitePointingInfo() for the response returned.
 */
TEST_P(RadioSatelliteTest, startSendingSatellitePointingInfo) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping startSendingSatellitePointingInfo because satellite is not supported in "
              "device");
        return;
    } else {
        ALOGI("Running startSendingSatellitePointingInfo because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = satellite->startSendingSatellitePointingInfo(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("startSendingSatellitePointingInfo, rspInfo.error = %s\n",
          toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            rsp_satellite->rspInfo.error,
            {RadioError::NONE, RadioError::INTERNAL_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_MODEM_STATE, RadioError::INVALID_SIM_STATE,
             RadioError::INVALID_STATE, RadioError::MODEM_ERR, RadioError::NO_MEMORY,
             RadioError::NO_RESOURCES, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::REQUEST_RATE_LIMITED,
             RadioError::SYSTEM_ERR}));
}

/*
 * Test IRadioSatellite.stopSatelliteLocationUpdate() for the response returned.
 */
TEST_P(RadioSatelliteTest, stopSatelliteLocationUpdate) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping stopSendingSatellitePointingInfo because satellite is not supported in "
              "device");
        return;
    } else {
        ALOGI("Running stopSendingSatellitePointingInfo because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = satellite->stopSendingSatellitePointingInfo(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("stopSendingSatellitePointingInfo, rspInfo.error = %s\n",
          toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            rsp_satellite->rspInfo.error,
            {RadioError::NONE, RadioError::INTERNAL_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_MODEM_STATE, RadioError::INVALID_SIM_STATE,
             RadioError::INVALID_STATE, RadioError::MODEM_ERR, RadioError::NO_MEMORY,
             RadioError::NO_RESOURCES, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::REQUEST_RATE_LIMITED,
             RadioError::SYSTEM_ERR}));
}

/*
 * Test IRadioSatellite.getMaxCharactersPerTextMessage() for the response returned.
 */
TEST_P(RadioSatelliteTest, getMaxCharactersPerTextMessage) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping getMaxCharactersPerTextMessage because satellite is not supported in "
              "device");
        return;
    } else {
        ALOGI("Running getMaxCharactersPerTextMessage because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = satellite->getMaxCharactersPerTextMessage(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("getMaxCharactersPerTextMessage, rspInfo.error = %s\n",
          toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            rsp_satellite->rspInfo.error,
            {RadioError::NONE, RadioError::INTERNAL_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_MODEM_STATE, RadioError::INVALID_SIM_STATE,
             RadioError::INVALID_STATE, RadioError::MODEM_ERR, RadioError::NO_MEMORY,
             RadioError::NO_RESOURCES, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::REQUEST_RATE_LIMITED,
             RadioError::SYSTEM_ERR}));
}

/*
 * Test IRadioSatellite.getTimeForNextSatelliteVisibility() for the response returned.
 */
TEST_P(RadioSatelliteTest, getTimeForNextSatelliteVisibility) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_SATELLITE)) {
        ALOGI("Skipping getTimeForNextSatelliteVisibility because satellite is not supported in "
              "device");
        return;
    } else {
        ALOGI("Running getTimeForNextSatelliteVisibility because satellite is supported in device");
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = satellite->getTimeForNextSatelliteVisibility(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, rsp_satellite->rspInfo.type);
    EXPECT_EQ(serial, rsp_satellite->rspInfo.serial);

    ALOGI("getTimeForNextSatelliteVisibility, rspInfo.error = %s\n",
          toString(rsp_satellite->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
            rsp_satellite->rspInfo.error,
            {RadioError::NONE, RadioError::INTERNAL_ERR, RadioError::INVALID_ARGUMENTS,
             RadioError::INVALID_MODEM_STATE, RadioError::INVALID_SIM_STATE,
             RadioError::INVALID_STATE, RadioError::MODEM_ERR, RadioError::NO_MEMORY,
             RadioError::NO_RESOURCES, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::REQUEST_RATE_LIMITED,
             RadioError::SYSTEM_ERR}));
}