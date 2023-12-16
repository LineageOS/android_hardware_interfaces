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
#include <aidl/android/hardware/radio/network/IndicationFilter.h>
#include <android/binder_manager.h>

#include "radio_network_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

namespace {
const RadioAccessSpecifierBands EUTRAN_BAND_17 =
        RadioAccessSpecifierBands::make<RadioAccessSpecifierBands::eutranBands>(
                {EutranBands::BAND_17});
const RadioAccessSpecifierBands EUTRAN_BAND_20 =
        RadioAccessSpecifierBands::make<RadioAccessSpecifierBands::eutranBands>(
                {EutranBands::BAND_20});
const RadioAccessSpecifier EUTRAN_SPECIFIER_17 = {
        .accessNetwork = AccessNetwork::EUTRAN, .bands = EUTRAN_BAND_17, .channels = {1, 2}};
const RadioAccessSpecifier EUTRAN_SPECIFIER_20 = {
        .accessNetwork = AccessNetwork::EUTRAN, .bands = EUTRAN_BAND_20, .channels = {128, 129}};
}  // namespace

void RadioNetworkTest::SetUp() {
    RadioServiceTest::SetUp();
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

    radioInd_network = ndk::SharedRefBase::make<RadioNetworkIndication>(*this);
    ASSERT_NE(nullptr, radioInd_network.get());

    radio_network->setResponseFunctions(radioRsp_network, radioInd_network);

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

void RadioNetworkTest::stopNetworkScan() {
    serial = GetRandomSerialNumber();
    radio_network->stopNetworkScan(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}

/*
 * Test IRadioNetwork.setAllowedNetworkTypesBitmap and IRadioNetwork.getAllowedNetworkTypesBitmap
 * for the response returned.
 */
TEST_P(RadioNetworkTest, setGetAllowedNetworkTypesBitmap) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setGetAllowedNetworkTypesBitmap "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    // get aidl version
    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);

    // save current value
    radio_network->getAllowedNetworkTypesBitmap(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    int32_t currentAllowedNetworkTypesBitmap = radioRsp_network->networkTypeBitmapResponse;

    // set new value
    int32_t allowedNetworkTypesBitmap = static_cast<int32_t>(RadioAccessFamily::LTE);
    serial = GetRandomSerialNumber();
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
        if (radioRsp_network->rspInfo.error == RadioError::NONE) {
            if (aidl_version < 2) {
                radioRsp_network->networkTypeBitmapResponse
                    &= ~static_cast<int32_t>(RadioAccessFamily::LTE_CA);
            }

            // verify we get the value we set
            EXPECT_EQ(radioRsp_network->networkTypeBitmapResponse, allowedNetworkTypesBitmap);
        }
    }

    // reset value to previous
    serial = GetRandomSerialNumber();
    radio_network->setAllowedNetworkTypesBitmap(serial, currentAllowedNetworkTypesBitmap);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}

/*
 * Test IRadioNetwork.setNrDualConnectivityState() for the response returned.
 */
TEST_P(RadioNetworkTest, setNrDualConnectivityState) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setNrDualConnectivityState "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

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
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping isNrDualConnectivityEnabled "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

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
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY)) {
            GTEST_SKIP() << "Skipping getUsageSetting "
                            "due to undefined FEATURE_TELEPHONY";
        }
    }

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
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY)) {
            GTEST_SKIP() << "Skipping setUsageSetting "
                            "due to undefined FEATURE_TELEPHONY";
        }
    }

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

    // After resetting the value to its original value, update the local cache, which must
    // always succeed.
    invokeAndExpectResponse([&](int serial) { return radio_network->getUsageSetting(serial); },
                            {RadioError::NONE});

    // Check that indeed the updated setting was set. We do this after resetting to original
    // conditions to avoid early-exiting the test and leaving the device in a modified state.
    EXPECT_EQ(expectedSetting, updatedSetting);
    // Check that indeed the original setting was reset.
    EXPECT_EQ(originalSetting, radioRsp_network->usageSetting);
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() with invalid hysteresisDb
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_invalidHysteresisDb) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_invalidHysteresisDb "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSSI;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 10;  // hysteresisDb too large given threshold list deltas
    signalThresholdInfo.thresholds = {-109, -103, -97, -89};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::EUTRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_invalidHysteresisDb, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() with empty thresholds
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_EmptyThresholds) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_EmptyThresholds "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSSI;
    signalThresholdInfo.hysteresisMs = 0;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::EUTRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_EmptyParams, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for GERAN
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_Geran) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_Geran "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSSI;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-109, -103, -97, -89};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::GERAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_Geran, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for UTRAN
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_Utran_Rscp) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_Utran_Rscp "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSCP;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-110, -97, -73, -49, -25};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::UTRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_Utran, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for UTRAN
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_Utran_Ecno) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_Utran_Ecno "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_ECNO;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-22, -18, 0};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::UTRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_Utran, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for EUTRAN
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_Eutran_RSRP) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_Eutran_RSRP "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSRP;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-128, -108, -88, -68};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::EUTRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_Eutran, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for EUTRAN
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_Eutran_RSRQ) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_Eutran_RSRQ "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSRQ;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-27, -20, -13, -6};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::EUTRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_Eutran, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for EUTRAN
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_Eutran_RSSNR) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_Eutran_RSSNR "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSSNR;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-10, 0, 10, 20};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::EUTRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for CDMA2000
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_Cdma2000) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_Cdma2000 "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSSI;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-105, -90, -75, -65};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::CDMA2000;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_Cdma2000, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for NGRAN_SSRSRP
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_NGRAN_SSRSRP) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_NGRAN_SSRSRP "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_SSRSRP;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.thresholds = {-105, -90, -75, -65};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::NGRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_NGRAN_SSRSRP, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());

    // Allow REQUEST_NOT_SUPPORTED because some non-5G device may not support NGRAN for
    // setSignalStrengthReportingCriteria()
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for NGRAN_SSRSRQ
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_NGRAN_SSRSRQ) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_NGRAN_SSRSRQ "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_SSRSRQ;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.thresholds = {-43, -20, 0, 20};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::NGRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_NGRAN_SSRSRQ, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());

    // Allow REQUEST_NOT_SUPPORTED because some non-5G device may not support NGRAN for
    // setSignalStrengthReportingCriteria()
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for EUTRAN
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_Disable_RSSNR) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_Disable_RSSNR "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSSNR;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-10, 0, 10, 20};
    signalThresholdInfo.isEnabled = false;
    signalThresholdInfo.ran = AccessNetwork::EUTRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for NGRAN_SSSINR
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_NGRAN_SSSINR) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_NGRAN_SSSINR "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_SSSINR;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.thresholds = {-10, 3, 16, 18};
    signalThresholdInfo.isEnabled = true;
    signalThresholdInfo.ran = AccessNetwork::NGRAN;

    ndk::ScopedAStatus res =
            radio_network->setSignalStrengthReportingCriteria(serial, {signalThresholdInfo});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_NGRAN_SSSINR, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());

    // Allow REQUEST_NOT_SUPPORTED because some non-5G device may not support NGRAN for
    // setSignalStrengthReportingCriteria()
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioNetwork.setSignalStrengthReportingCriteria() for multi-RANs per request
 */
TEST_P(RadioNetworkTest, setSignalStrengthReportingCriteria_multiRansPerRequest) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSignalStrengthReportingCriteria_multiRansPerRequest "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    SignalThresholdInfo signalThresholdInfoGeran;
    signalThresholdInfoGeran.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSSI;
    signalThresholdInfoGeran.hysteresisMs = 5000;
    signalThresholdInfoGeran.hysteresisDb = 2;
    signalThresholdInfoGeran.thresholds = {-109, -103, -97, -89};
    signalThresholdInfoGeran.isEnabled = true;
    signalThresholdInfoGeran.ran = AccessNetwork::GERAN;

    SignalThresholdInfo signalThresholdInfoUtran;
    signalThresholdInfoUtran.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSCP;
    signalThresholdInfoUtran.hysteresisMs = 5000;
    signalThresholdInfoUtran.hysteresisDb = 2;
    signalThresholdInfoUtran.thresholds = {-110, -97, -73, -49, -25};
    signalThresholdInfoUtran.isEnabled = true;
    signalThresholdInfoUtran.ran = AccessNetwork::UTRAN;

    SignalThresholdInfo signalThresholdInfoEutran;
    signalThresholdInfoEutran.signalMeasurement = SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSRP;
    signalThresholdInfoEutran.hysteresisMs = 5000;
    signalThresholdInfoEutran.hysteresisDb = 2;
    signalThresholdInfoEutran.thresholds = {-128, -108, -88, -68};
    signalThresholdInfoEutran.isEnabled = true;
    signalThresholdInfoEutran.ran = AccessNetwork::EUTRAN;

    SignalThresholdInfo signalThresholdInfoCdma2000;
    signalThresholdInfoCdma2000.signalMeasurement =
            SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_RSSI;
    signalThresholdInfoCdma2000.hysteresisMs = 5000;
    signalThresholdInfoCdma2000.hysteresisDb = 2;
    signalThresholdInfoCdma2000.thresholds = {-105, -90, -75, -65};
    signalThresholdInfoCdma2000.isEnabled = true;
    signalThresholdInfoCdma2000.ran = AccessNetwork::CDMA2000;

    SignalThresholdInfo signalThresholdInfoNgran;
    signalThresholdInfoNgran.signalMeasurement =
            SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_SSRSRP;
    signalThresholdInfoNgran.hysteresisMs = 5000;
    signalThresholdInfoNgran.hysteresisDb = 0;
    signalThresholdInfoNgran.thresholds = {-105, -90, -75, -65};
    signalThresholdInfoNgran.isEnabled = true;
    signalThresholdInfoNgran.ran = AccessNetwork::NGRAN;

    const static std::vector<SignalThresholdInfo> candidateSignalThresholdInfos = {
            signalThresholdInfoGeran, signalThresholdInfoUtran, signalThresholdInfoEutran,
            signalThresholdInfoCdma2000, signalThresholdInfoNgran};

    std::vector<SignalThresholdInfo> supportedSignalThresholdInfos;
    for (size_t i = 0; i < candidateSignalThresholdInfos.size(); i++) {
        serial = GetRandomSerialNumber();
        ndk::ScopedAStatus res = radio_network->setSignalStrengthReportingCriteria(
                serial, {candidateSignalThresholdInfos[i]});
        ASSERT_OK(res);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        if (radioRsp_network->rspInfo.error == RadioError::NONE) {
            supportedSignalThresholdInfos.push_back(signalThresholdInfoEutran);
        } else {
            // Refer to IRadioNetworkResponse#setSignalStrengthReportingCriteriaResponse
            ASSERT_TRUE(CheckAnyOfErrors(
                    radioRsp_network->rspInfo.error,
                    {RadioError::INVALID_ARGUMENTS, RadioError::RADIO_NOT_AVAILABLE}));
        }
    }

    ASSERT_FALSE(supportedSignalThresholdInfos.empty());

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = radio_network->setSignalStrengthReportingCriteria(
            serial, supportedSignalThresholdInfos);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_multiRansPerRequest, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadioNetwork.setLinkCapacityReportingCriteria() invalid hysteresisDlKbps
 */
TEST_P(RadioNetworkTest, setLinkCapacityReportingCriteria_invalidHysteresisDlKbps) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setLinkCapacityReportingCriteria_invalidHysteresisDlKbps "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->setLinkCapacityReportingCriteria(
            serial, 5000,
            5000,  // hysteresisDlKbps too big for thresholds delta
            100, {1000, 5000, 10000, 20000}, {500, 1000, 5000, 10000}, AccessNetwork::EUTRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_invalidHysteresisDlKbps, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
}

/*
 * Test IRadioNetwork.setLinkCapacityReportingCriteria() invalid hysteresisUlKbps
 */
TEST_P(RadioNetworkTest, setLinkCapacityReportingCriteria_invalidHysteresisUlKbps) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setLinkCapacityReportingCriteria_invalidHysteresisUlKbps "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->setLinkCapacityReportingCriteria(
            serial, 5000, 500, 1000,  // hysteresisUlKbps too big for thresholds delta
            {1000, 5000, 10000, 20000}, {500, 1000, 5000, 10000}, AccessNetwork::EUTRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_invalidHysteresisUlKbps, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
}

/*
 * Test IRadioNetwork.setLinkCapacityReportingCriteria() empty params
 */
TEST_P(RadioNetworkTest, setLinkCapacityReportingCriteria_emptyParams) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setLinkCapacityReportingCriteria_emptyParams "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->setLinkCapacityReportingCriteria(
            serial, 0, 0, 0, {}, {}, AccessNetwork::EUTRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_emptyParams, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadioNetwork.setLinkCapacityReportingCriteria() for GERAN
 */
TEST_P(RadioNetworkTest, setLinkCapacityReportingCriteria_Geran) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setLinkCapacityReportingCriteria_Geran "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->setLinkCapacityReportingCriteria(
            serial, 5000, 500, 100, {1000, 5000, 10000, 20000}, {500, 1000, 5000, 10000},
            AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_Geran, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    // Allow REQUEST_NOT_SUPPORTED as setLinkCapacityReportingCriteria() may not be supported
    // for GERAN
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadioNetwork.setSystemSelectionChannels() for the response returned.
 */
TEST_P(RadioNetworkTest, setSystemSelectionChannels) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setSystemSelectionChannels "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = radio_network->getSystemSelectionChannels(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    if (radioRsp_network->specifiers.size() == 0) {
        // TODO (b/189255895): Throw an error once getSystemSelectionChannels is functional.
        ALOGI("Skipped the test due to empty system selection channels.");
        GTEST_SKIP();
    }
    std::vector<RadioAccessSpecifier> originalSpecifiers = radioRsp_network->specifiers;

    serial = GetRandomSerialNumber();
    res = radio_network->setSystemSelectionChannels(serial, true,
                                                    {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("setSystemSelectionChannels, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));

    if (radioRsp_network->rspInfo.error == RadioError::NONE) {
        serial = GetRandomSerialNumber();
        res = radio_network->setSystemSelectionChannels(
                serial, false, {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20});
        ASSERT_OK(res);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
        ALOGI("setSystemSelectionChannels, rspInfo.error = %s\n",
              toString(radioRsp_network->rspInfo.error).c_str());
        EXPECT_EQ(RadioError::NONE, radioRsp_network->rspInfo.error);
    }

    serial = GetRandomSerialNumber();
    res = radio_network->setSystemSelectionChannels(serial, true, originalSpecifiers);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
}

/*
 * Test IRadioNetwork.startNetworkScan() for the response returned.
 */
TEST_P(RadioNetworkTest, startNetworkScan) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping startNetworkScan "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    NetworkScanRequest request = {.type = NetworkScanRequest::SCAN_TYPE_ONE_SHOT,
                                  .interval = 60,
                                  .specifiers = {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20},
                                  .maxSearchTime = 60,
                                  .incrementalResults = false,
                                  .incrementalResultsPeriodicity = 1};

    ndk::ScopedAStatus res = radio_network->startNetworkScan(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("startNetworkScan, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::SIM_ABSENT}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        if (deviceSupportsFeature(FEATURE_TELEPHONY_GSM) && isLteConnected()) {
            // Modems support 3GPP RAT family need to
            // support scanning requests combined with some parameters.
            ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                         {RadioError::NONE, RadioError::OPERATION_NOT_ALLOWED}));
        } else {
            ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                         {RadioError::NONE, RadioError::OPERATION_NOT_ALLOWED,
                                          RadioError::INVALID_ARGUMENTS}));
        }
    }

    if (radioRsp_network->rspInfo.error == RadioError::NONE) {
        ALOGI("Stop Network Scan");
        stopNetworkScan();
    }
}

/*
 * Test IRadioNetwork.startNetworkScan() with invalid specifier.
 */
TEST_P(RadioNetworkTest, startNetworkScan_InvalidArgument) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping startNetworkScan_InvalidArgument "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    NetworkScanRequest request = {.type = NetworkScanRequest::SCAN_TYPE_ONE_SHOT, .interval = 60};

    ndk::ScopedAStatus res = radio_network->startNetworkScan(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadioNetwork.startNetworkScan() with invalid interval (lower boundary).
 */
TEST_P(RadioNetworkTest, startNetworkScan_InvalidInterval1) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping startNetworkScan_InvalidInterval1 "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    NetworkScanRequest request = {.type = NetworkScanRequest::SCAN_TYPE_PERIODIC,
                                  .interval = 4,
                                  .specifiers = {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20},
                                  .maxSearchTime = 60,
                                  .incrementalResults = false,
                                  .incrementalResultsPeriodicity = 1};

    ndk::ScopedAStatus res = radio_network->startNetworkScan(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidInterval1, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadioNetwork.startNetworkScan() with invalid interval (upper boundary).
 */
TEST_P(RadioNetworkTest, startNetworkScan_InvalidInterval2) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping startNetworkScan_InvalidInterval2 "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    NetworkScanRequest request = {.type = NetworkScanRequest::SCAN_TYPE_PERIODIC,
                                  .interval = 301,
                                  .specifiers = {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20},
                                  .maxSearchTime = 60,
                                  .incrementalResults = false,
                                  .incrementalResultsPeriodicity = 1};

    ndk::ScopedAStatus res = radio_network->startNetworkScan(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidInterval2, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadioNetwork.startNetworkScan() with invalid max search time (lower boundary).
 */
TEST_P(RadioNetworkTest, startNetworkScan_InvalidMaxSearchTime1) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping startNetworkScan_InvalidMaxSearchTime1 "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    NetworkScanRequest request = {.type = NetworkScanRequest::SCAN_TYPE_ONE_SHOT,
                                  .interval = 60,
                                  .specifiers = {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20},
                                  .maxSearchTime = 59,
                                  .incrementalResults = false,
                                  .incrementalResultsPeriodicity = 1};

    ndk::ScopedAStatus res = radio_network->startNetworkScan(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidMaxSearchTime1, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadioNetwork.startNetworkScan() with invalid max search time (upper boundary).
 */
TEST_P(RadioNetworkTest, startNetworkScan_InvalidMaxSearchTime2) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping startNetworkScan_InvalidMaxSearchTime2 "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    NetworkScanRequest request = {.type = NetworkScanRequest::SCAN_TYPE_ONE_SHOT,
                                  .interval = 60,
                                  .specifiers = {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20},
                                  .maxSearchTime = 3601,
                                  .incrementalResults = false,
                                  .incrementalResultsPeriodicity = 1};

    ndk::ScopedAStatus res = radio_network->startNetworkScan(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidMaxSearchTime2, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadioNetwork.startNetworkScan() with invalid periodicity (lower boundary).
 */
TEST_P(RadioNetworkTest, startNetworkScan_InvalidPeriodicity1) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping startNetworkScan_InvalidPeriodicity1 "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    NetworkScanRequest request = {.type = NetworkScanRequest::SCAN_TYPE_ONE_SHOT,
                                  .interval = 60,
                                  .specifiers = {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20},
                                  .maxSearchTime = 600,
                                  .incrementalResults = true,
                                  .incrementalResultsPeriodicity = 0};

    ndk::ScopedAStatus res = radio_network->startNetworkScan(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidPeriodicity1, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadioNetwork.startNetworkScan() with invalid periodicity (upper boundary).
 */
TEST_P(RadioNetworkTest, startNetworkScan_InvalidPeriodicity2) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping startNetworkScan_InvalidPeriodicity2 "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    NetworkScanRequest request = {.type = NetworkScanRequest::SCAN_TYPE_ONE_SHOT,
                                  .interval = 60,
                                  .specifiers = {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20},
                                  .maxSearchTime = 600,
                                  .incrementalResults = true,
                                  .incrementalResultsPeriodicity = 11};

    ndk::ScopedAStatus res = radio_network->startNetworkScan(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidPeriodicity2, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
    }
}

/*
 * Test IRadioNetwork.startNetworkScan() with valid periodicity
 */
TEST_P(RadioNetworkTest, startNetworkScan_GoodRequest1) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping startNetworkScan_GoodRequest1 "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    NetworkScanRequest request = {.type = NetworkScanRequest::SCAN_TYPE_ONE_SHOT,
                                  .interval = 60,
                                  .specifiers = {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20},
                                  .maxSearchTime = 360,
                                  .incrementalResults = false,
                                  .incrementalResultsPeriodicity = 10};

    ndk::ScopedAStatus res = radio_network->startNetworkScan(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("startNetworkScan_GoodRequest1, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS}));
    }

    if (radioRsp_network->rspInfo.error == RadioError::NONE) {
        ALOGI("Stop Network Scan");
        stopNetworkScan();
    }
}

/*
 * Test IRadioNetwork.startNetworkScan() with valid periodicity and plmns
 */
TEST_P(RadioNetworkTest, startNetworkScan_GoodRequest2) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping startNetworkScan_GoodRequest2 "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    NetworkScanRequest request = {.type = NetworkScanRequest::SCAN_TYPE_ONE_SHOT,
                                  .interval = 60,
                                  .specifiers = {::EUTRAN_SPECIFIER_17, ::EUTRAN_SPECIFIER_20},
                                  .maxSearchTime = 360,
                                  .incrementalResults = false,
                                  .incrementalResultsPeriodicity = 10,
                                  .mccMncs = {"310410"}};

    ndk::ScopedAStatus res = radio_network->startNetworkScan(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("startNetworkScan_GoodRequest2, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS}));
    }

    if (radioRsp_network->rspInfo.error == RadioError::NONE) {
        ALOGI("Stop Network Scan");
        stopNetworkScan();
    }
}

/*
 * Test IRadioNetwork.setNetworkSelectionModeManual() for the response returned.
 */
TEST_P(RadioNetworkTest, setNetworkSelectionModeManual) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setNetworkSelectionModeManual "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    // can't camp on nonexistent MCCMNC, so we expect this to fail.
    ndk::ScopedAStatus res =
            radio_network->setNetworkSelectionModeManual(serial, "123456", AccessNetwork::EUTRAN);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::NONE, RadioError::ILLEGAL_SIM_OR_ME, RadioError::INVALID_ARGUMENTS,
                 RadioError::INVALID_STATE, RadioError::RADIO_NOT_AVAILABLE, RadioError::NO_MEMORY,
                 RadioError::INTERNAL_ERR, RadioError::SYSTEM_ERR, RadioError::CANCELLED}));
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INVALID_ARGUMENTS,
                 RadioError::INVALID_STATE, RadioError::NO_MEMORY, RadioError::INTERNAL_ERR,
                 RadioError::SYSTEM_ERR, RadioError::CANCELLED}));
    }
}

/*
 * Test IRadioNetwork.getBarringInfo() for the response returned.
 */
TEST_P(RadioNetworkTest, getBarringInfo) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getBarringInfo "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = radio_network->getBarringInfo(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ASSERT_TRUE(radioRsp_network->barringInfoList.size() > 0);

    std::set<int> reportedServices;

    // validate that the service types are in range
    for (const auto& info : radioRsp_network->barringInfoList) {
        ASSERT_TRUE((info.serviceType >= BarringInfo::SERVICE_TYPE_CS_SERVICE &&
                     info.serviceType <= BarringInfo::SERVICE_TYPE_SMS) ||
                    (info.serviceType >= BarringInfo::SERVICE_TYPE_OPERATOR_1 &&
                     info.serviceType <= BarringInfo::SERVICE_TYPE_OPERATOR_32));
        reportedServices.insert(info.serviceType);

        // Any type that is "conditional" must have valid values for conditional barring
        // factor and time.
        switch (info.barringType) {
            case BarringInfo::BARRING_TYPE_NONE:  // fall through
            case BarringInfo::BARRING_TYPE_UNCONDITIONAL:
                break;
            case BarringInfo::BARRING_TYPE_CONDITIONAL: {
                const int32_t barringFactor = info.barringTypeSpecificInfo->factor;
                ASSERT_TRUE(barringFactor >= 0 && barringFactor <= 100);
                ASSERT_TRUE(info.barringTypeSpecificInfo->timeSeconds > 0);
                break;
            }
            default:
                FAIL();
        }
    }

    // Certain types of barring are relevant for certain RANs. Ensure that only the right
    // types are reported. Note that no types are required, simply that for a given technology
    // only certain types are valid. This is one way to check that implementations are
    // not providing information that they don't have.
    static const std::set<int> UTRA_SERVICES{
            BarringInfo::SERVICE_TYPE_CS_SERVICE, BarringInfo::SERVICE_TYPE_PS_SERVICE,
            BarringInfo::SERVICE_TYPE_CS_VOICE,   BarringInfo::SERVICE_TYPE_EMERGENCY,
            BarringInfo::SERVICE_TYPE_SMS,
    };

    static const std::set<int> EUTRA_SERVICES{
            BarringInfo::SERVICE_TYPE_MO_SIGNALLING, BarringInfo::SERVICE_TYPE_MO_DATA,
            BarringInfo::SERVICE_TYPE_CS_FALLBACK,   BarringInfo::SERVICE_TYPE_MMTEL_VOICE,
            BarringInfo::SERVICE_TYPE_MMTEL_VIDEO,   BarringInfo::SERVICE_TYPE_EMERGENCY,
            BarringInfo::SERVICE_TYPE_SMS,
    };

    static const std::set<int> NGRA_SERVICES = {
            BarringInfo::SERVICE_TYPE_MO_SIGNALLING, BarringInfo::SERVICE_TYPE_MO_DATA,
            BarringInfo::SERVICE_TYPE_CS_FALLBACK,   BarringInfo::SERVICE_TYPE_MMTEL_VOICE,
            BarringInfo::SERVICE_TYPE_MMTEL_VIDEO,   BarringInfo::SERVICE_TYPE_EMERGENCY,
            BarringInfo::SERVICE_TYPE_SMS,           BarringInfo::SERVICE_TYPE_OPERATOR_1,
            BarringInfo::SERVICE_TYPE_OPERATOR_2,    BarringInfo::SERVICE_TYPE_OPERATOR_3,
            BarringInfo::SERVICE_TYPE_OPERATOR_4,    BarringInfo::SERVICE_TYPE_OPERATOR_5,
            BarringInfo::SERVICE_TYPE_OPERATOR_6,    BarringInfo::SERVICE_TYPE_OPERATOR_7,
            BarringInfo::SERVICE_TYPE_OPERATOR_8,    BarringInfo::SERVICE_TYPE_OPERATOR_9,
            BarringInfo::SERVICE_TYPE_OPERATOR_10,   BarringInfo::SERVICE_TYPE_OPERATOR_11,
            BarringInfo::SERVICE_TYPE_OPERATOR_12,   BarringInfo::SERVICE_TYPE_OPERATOR_13,
            BarringInfo::SERVICE_TYPE_OPERATOR_14,   BarringInfo::SERVICE_TYPE_OPERATOR_15,
            BarringInfo::SERVICE_TYPE_OPERATOR_16,   BarringInfo::SERVICE_TYPE_OPERATOR_17,
            BarringInfo::SERVICE_TYPE_OPERATOR_18,   BarringInfo::SERVICE_TYPE_OPERATOR_19,
            BarringInfo::SERVICE_TYPE_OPERATOR_20,   BarringInfo::SERVICE_TYPE_OPERATOR_21,
            BarringInfo::SERVICE_TYPE_OPERATOR_22,   BarringInfo::SERVICE_TYPE_OPERATOR_23,
            BarringInfo::SERVICE_TYPE_OPERATOR_24,   BarringInfo::SERVICE_TYPE_OPERATOR_25,
            BarringInfo::SERVICE_TYPE_OPERATOR_26,   BarringInfo::SERVICE_TYPE_OPERATOR_27,
            BarringInfo::SERVICE_TYPE_OPERATOR_28,   BarringInfo::SERVICE_TYPE_OPERATOR_29,
            BarringInfo::SERVICE_TYPE_OPERATOR_30,   BarringInfo::SERVICE_TYPE_OPERATOR_31,
    };

    const std::set<int>* compareTo = nullptr;

    switch (radioRsp_network->barringCellIdentity.getTag()) {
        case CellIdentity::Tag::wcdma:
            // fall through
        case CellIdentity::Tag::tdscdma:
            compareTo = &UTRA_SERVICES;
            break;
        case CellIdentity::Tag::lte:
            compareTo = &EUTRA_SERVICES;
            break;
        case CellIdentity::Tag::nr:
            compareTo = &NGRA_SERVICES;
            break;
        case CellIdentity::Tag::cdma:
            // fall through
        default:
            FAIL();
            break;
    }

    std::set<int> diff;

    std::set_difference(reportedServices.begin(), reportedServices.end(), compareTo->begin(),
                        compareTo->end(), std::inserter(diff, diff.begin()));
}

/*
 * Test IRadioNetwork.getSignalStrength() for the response returned.
 */
TEST_P(RadioNetworkTest, getSignalStrength) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getSignalStrength "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->getSignalStrength(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp_network->rspInfo.error);
    } else if (cardStatus.cardState == CardStatus::STATE_PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE}));
    }
}

/*
 * Test IRadioNetwork.getCellInfoList() for the response returned.
 */
TEST_P(RadioNetworkTest, getCellInfoList) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getCellInfoList "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->getCellInfoList(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("getCellInfoList, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::NO_NETWORK_FOUND}));
}

/*
 * Test IRadioNetwork.getVoiceRegistrationState() for the response returned.
 */
TEST_P(RadioNetworkTest, getVoiceRegistrationState) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getVoiceRegistrationState "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->getVoiceRegistrationState(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("getVoiceRegistrationStateResponse, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE}));
}

/*
 * Test IRadioNetwork.getDataRegistrationState() for the response returned.
 */
TEST_P(RadioNetworkTest, getDataRegistrationState) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getDataRegistrationState "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->getDataRegistrationState(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("getDataRegistrationStateResponse, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::NOT_PROVISIONED}));

    // Check the mcc [0, 999] and mnc [0, 999].
    std::string mcc;
    std::string mnc;
    bool checkMccMnc = true;
    CellIdentity cellIdentity = radioRsp_network->dataRegResp.cellIdentity;
    switch (cellIdentity.getTag()) {
        case CellIdentity::noinit: {
            checkMccMnc = false;
            break;
        }
        case CellIdentity::gsm: {
            CellIdentityGsm cig = cellIdentity.get<CellIdentity::gsm>();
            mcc = cig.mcc;
            mnc = cig.mnc;
            break;
        }
        case CellIdentity::wcdma: {
            CellIdentityWcdma ciw = cellIdentity.get<CellIdentity::wcdma>();
            mcc = ciw.mcc;
            mnc = ciw.mnc;
            break;
        }
        case CellIdentity::tdscdma: {
            CellIdentityTdscdma cit = cellIdentity.get<CellIdentity::tdscdma>();
            mcc = cit.mcc;
            mnc = cit.mnc;
            break;
        }
        case CellIdentity::cdma: {
            // CellIdentityCdma has no mcc/mnc
            CellIdentityCdma cic = cellIdentity.get<CellIdentity::cdma>();
            checkMccMnc = false;
            break;
        }
        case CellIdentity::lte: {
            CellIdentityLte cil = cellIdentity.get<CellIdentity::lte>();
            mcc = cil.mcc;
            mnc = cil.mnc;
            break;
        }
        case CellIdentity::nr: {
            CellIdentityNr cin = cellIdentity.get<CellIdentity::nr>();
            mcc = cin.mcc;
            mnc = cin.mnc;
            break;
        }
    }

    // 32 bit system might return invalid mcc and mnc string "\xff\xff..."
    if (checkMccMnc) {
        int mccSize = mcc.size();
        EXPECT_TRUE(mccSize == 0 || mccSize == 3);
        if (mccSize > 0) {
            int mcc_int = stoi(mcc);
            EXPECT_TRUE(mcc_int >= 0 && mcc_int <= 999);
        }

        int mncSize = mnc.size();
        EXPECT_TRUE(mncSize == 0 || mncSize == 2 || mncSize == 3);
        if (mncSize > 0) {
            int mnc_int = stoi(mnc);
            EXPECT_TRUE(mnc_int >= 0 && mnc_int <= 999);
        }
    }

    // Check for access technology specific info
    AccessTechnologySpecificInfo info = radioRsp_network->dataRegResp.accessTechnologySpecificInfo;
    RadioTechnology rat = radioRsp_network->dataRegResp.rat;

    // TODO: add logic for cdmaInfo
    if (rat == RadioTechnology::LTE) {
        ASSERT_EQ(info.getTag(), AccessTechnologySpecificInfo::eutranInfo);
    } else if (rat == RadioTechnology::NR) {
        ASSERT_TRUE(info.getTag() == AccessTechnologySpecificInfo::ngranNrVopsInfo);
    }
}

/*
 * Test IRadioNetwork.getAvailableBandModes() for the response returned.
 */
TEST_P(RadioNetworkTest, getAvailableBandModes) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getAvailableBandModes "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->getAvailableBandModes(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ALOGI("getAvailableBandModes, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                  RadioError::MODEM_ERR, RadioError::INTERNAL_ERR,
                                  // If REQUEST_NOT_SUPPORTED is returned, then it should also be
                                  // returned for setBandMode().
                                  RadioError::REQUEST_NOT_SUPPORTED}));
    bool hasUnspecifiedBandMode = false;
    if (radioRsp_network->rspInfo.error == RadioError::NONE) {
        for (const RadioBandMode& mode : radioRsp_network->radioBandModes) {
            // Automatic mode selection must be supported
            if (mode == RadioBandMode::BAND_MODE_UNSPECIFIED) hasUnspecifiedBandMode = true;
        }
        ASSERT_TRUE(hasUnspecifiedBandMode);
    }
}

/*
 * Test IRadioNetwork.setIndicationFilter()
 */
TEST_P(RadioNetworkTest, setIndicationFilter) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setIndicationFilter "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res =
            radio_network->setIndicationFilter(serial, static_cast<int>(IndicationFilter::ALL));
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ALOGI("setIndicationFilter, rspInfo.error = %s\n",
          toString(radioRsp_network->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadioNetwork.setBarringPassword() for the response returned.
 */
TEST_P(RadioNetworkTest, setBarringPassword) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setBarringPassword "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();
    std::string facility = "";
    std::string oldPassword = "";
    std::string newPassword = "";

    radio_network->setBarringPassword(serial, facility, oldPassword, newPassword);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::FDN_CHECK_FAILURE,
                                      RadioError::INVALID_ARGUMENTS, RadioError::MODEM_ERR},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioNetwork.setSuppServiceNotifications() for the response returned.
 */
TEST_P(RadioNetworkTest, setSuppServiceNotifications) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CALLING)) {
            GTEST_SKIP() << "Skipping setSuppServiceNotifications "
                            "due to undefined FEATURE_TELEPHONY_CALLING";
        }
    }

    serial = GetRandomSerialNumber();
    bool enable = false;

    radio_network->setSuppServiceNotifications(serial, enable);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadioNetwork.getImsRegistrationState() for the response returned.
 */
TEST_P(RadioNetworkTest, getImsRegistrationState) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
            GTEST_SKIP() << "Skipping getImsRegistrationState "
                            "due to undefined FEATURE_TELEPHONY_IMS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->getImsRegistrationState(serial);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::NONE, RadioError::MODEM_ERR, RadioError::INVALID_MODEM_STATE},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioNetwork.getOperator() for the response returned.
 */
TEST_P(RadioNetworkTest, getOperator) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getOperator "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->getOperator(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp_network->rspInfo.error);
    }
}

/*
 * Test IRadioNetwork.getNetworkSelectionMode() for the response returned.
 */
TEST_P(RadioNetworkTest, getNetworkSelectionMode) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getNetworkSelectionMode "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->getNetworkSelectionMode(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp_network->rspInfo.error);
    }
}

/*
 * Test IRadioNetwork.setNetworkSelectionModeAutomatic() for the response returned.
 */
TEST_P(RadioNetworkTest, setNetworkSelectionModeAutomatic) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setNetworkSelectionModeAutomatic "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->setNetworkSelectionModeAutomatic(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::ILLEGAL_SIM_OR_ME,
                                      RadioError::OPERATION_NOT_ALLOWED},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioNetwork.getAvailableNetworks() for the response returned.
 */
TEST_P(RadioNetworkTest, getAvailableNetworks) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getAvailableNetworks "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->getAvailableNetworks(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ASSERT_TRUE(radioRsp_network->rspInfo.type == RadioResponseType::SOLICITED ||
                radioRsp_network->rspInfo.type == RadioResponseType::SOLICITED_ACK_EXP);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::NONE, RadioError::CANCELLED, RadioError::DEVICE_IN_USE,
                 RadioError::MODEM_ERR, RadioError::OPERATION_NOT_ALLOWED},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioNetwork.setBandMode() for the response returned.
 */
TEST_P(RadioNetworkTest, setBandMode) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setBandMode "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->setBandMode(serial, RadioBandMode::BAND_MODE_USA);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error, {RadioError::NONE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioNetwork.setLocationUpdates() for the response returned.
 */
TEST_P(RadioNetworkTest, setLocationUpdates) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setLocationUpdates "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->setLocationUpdates(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT}));
    }
}

/*
 * Test IRadioNetwork.setCdmaRoamingPreference() for the response returned.
 */
TEST_P(RadioNetworkTest, setCdmaRoamingPreference) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping setCdmaRoamingPreference "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->setCdmaRoamingPreference(serial, CdmaRoamingType::HOME_NETWORK);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::NONE, RadioError::SIM_ABSENT, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadioNetwork.getCdmaRoamingPreference() for the response returned.
 */
TEST_P(RadioNetworkTest, getCdmaRoamingPreference) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
            GTEST_SKIP() << "Skipping getCdmaRoamingPreference "
                            "due to undefined FEATURE_TELEPHONY_CDMA";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->getCdmaRoamingPreference(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(
                CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::SIM_ABSENT, RadioError::MODEM_ERR},
                                 CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadioNetwork.getVoiceRadioTechnology() for the response returned.
 */
TEST_P(RadioNetworkTest, getVoiceRadioTechnology) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping getVoiceRadioTechnology "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->getVoiceRadioTechnology(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp_network->rspInfo.error);
    }
}

/*
 * Test IRadioNetwork.setCellInfoListRate() for the response returned.
 */
TEST_P(RadioNetworkTest, setCellInfoListRate) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setCellInfoListRate "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->setCellInfoListRate(serial, 10);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadioNetwork.supplyNetworkDepersonalization() for the response returned.
 */
TEST_P(RadioNetworkTest, supplyNetworkDepersonalization) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping supplyNetworkDepersonalization "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    serial = GetRandomSerialNumber();

    radio_network->supplyNetworkDepersonalization(serial, std::string("test"));
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (cardStatus.cardState == CardStatus::STATE_ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::NONE, RadioError::INVALID_ARGUMENTS, RadioError::INTERNAL_ERR,
                 RadioError::INVALID_SIM_STATE, RadioError::MODEM_ERR, RadioError::NO_MEMORY,
                 RadioError::PASSWORD_INCORRECT, RadioError::SIM_ABSENT, RadioError::SYSTEM_ERR}));
    }
}

/*
 * Test IRadioNetwork.setEmergencyMode() for the response returned.
 */
TEST_P(RadioNetworkTest, setEmergencyMode) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setEmergencyMode "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 2) {
        ALOGI("Skipped the test since setEmergencyMode is not supported on version < 2.");
        GTEST_SKIP();
    }

    serial = GetRandomSerialNumber();

    radio_network->setEmergencyMode(serial, EmergencyMode::EMERGENCY_WWAN);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::MODEM_ERR, RadioError::INVALID_ARGUMENTS}));

    // exit emergency mode for other tests
    serial = GetRandomSerialNumber();
    radio_network->exitEmergencyMode(serial);
}

/*
 * Test IRadioNetwork.triggerEmergencyNetworkScan() for the response returned.
 */
TEST_P(RadioNetworkTest, triggerEmergencyNetworkScan) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping triggerEmergencyNetworkScan "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 2) {
        ALOGI("Skipped the test since"
                " triggerEmergencyNetworkScan is not supported on version < 2.");
        GTEST_SKIP();
    }

    serial = GetRandomSerialNumber();

    EmergencyNetworkScanTrigger scanRequest;
    scanRequest.accessNetwork = {AccessNetwork::EUTRAN};
    scanRequest.scanType = EmergencyScanType::NO_PREFERENCE;

    radio_network->triggerEmergencyNetworkScan(serial, scanRequest);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED, RadioError::RADIO_NOT_AVAILABLE,
             RadioError::MODEM_ERR, RadioError::INVALID_ARGUMENTS}));
}

/*
 * Test IRadioNetwork.cancelEmergencyNetworkScan() for the response returned.
 */
TEST_P(RadioNetworkTest, cancelEmergencyNetworkScan) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping cancelEmergencyNetworkScan "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 2) {
        ALOGI("Skipped the test since cancelEmergencyNetworkScan is not supported on version < 2.");
        GTEST_SKIP();
    }

    serial = GetRandomSerialNumber();

    radio_network->cancelEmergencyNetworkScan(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED,
                                  RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));
}

/*
 * Test IRadioNetwork.exitEmergencyMode() for the response returned.
 */
TEST_P(RadioNetworkTest, exitEmergencyMode) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping exitEmergencyMode "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 2) {
        ALOGI("Skipped the test since exitEmergencyMode is not supported on version < 2.");
        GTEST_SKIP();
    }

    serial = GetRandomSerialNumber();

    radio_network->exitEmergencyMode(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED,
                                  RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));
}

/*
 * Test IRadioNetwork.setN1ModeEnabled() for the response returned.
 */
TEST_P(RadioNetworkTest, setN1ModeEnabled) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setN1ModeEnabled "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 2) {
        ALOGI("Skipped the test since setN1ModeEnabled is not supported on version < 2.");
        GTEST_SKIP();
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res =
            radio_network->setN1ModeEnabled(serial, false);
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
 * Test IRadioNetwork.isN1ModeEnabled() for the response returned.
 */
TEST_P(RadioNetworkTest, isN1ModeEnabled) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping isN1ModeEnabled "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 2) {
        ALOGI("Skipped the test since isN1ModeEnabled is not supported on version < 2.");
        GTEST_SKIP();
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->isN1ModeEnabled(serial);
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
                 RadioError::REQUEST_NOT_SUPPORTED, RadioError::NONE}));
    }
}

/*
 * Test IRadioNetwork.setNullCipherAndIntegrityEnabled() for the response returned.
 */
TEST_P(RadioNetworkTest, setNullCipherAndIntegrityEnabled) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping setNullCipherAndIntegrityEnabled "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 2) {
        ALOGI("Skipped the test since"
                " setNullCipherAndIntegrityEnabled is not supported on version < 2.");
        GTEST_SKIP();
    }

    serial = GetRandomSerialNumber();

    radio_network->setNullCipherAndIntegrityEnabled(serial, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (aidl_version >= 3 && deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));
    } else {
        // For aidl_version 2, API is optional
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::MODEM_ERR, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/**
 * Test IRadioNetwork.isNullCipherAndIntegrityEnabled() for the response returned.
 */
TEST_P(RadioNetworkTest, isNullCipherAndIntegrityEnabled) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
            GTEST_SKIP() << "Skipping isNullCipherAndIntegrityEnabled "
                            "due to undefined FEATURE_TELEPHONY_RADIO_ACCESS";
        }
    }

    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 2) {
        ALOGI("Skipped the test since"
                " isNullCipherAndIntegrityEnabled is not supported on version < 2.");
        GTEST_SKIP();
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->isNullCipherAndIntegrityEnabled(serial);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    if (aidl_version >= 3 && deviceSupportsFeature(FEATURE_TELEPHONY_RADIO_ACCESS)) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_network->rspInfo.error,
                {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));
    } else {
        // For aidl_version 2, API is optional
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_network->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::MODEM_ERR, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

TEST_P(RadioNetworkTest, isCellularIdentifierTransparencyEnabled) {
    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 3) {
        ALOGI("Skipped the test since"
                " isCellularIdentifierTransparencyEnabled is not supported on version < 3.");
        GTEST_SKIP();
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->isCellularIdentifierTransparencyEnabled(serial);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));
}

TEST_P(RadioNetworkTest, setCellularIdentifierTransparencyEnabled) {
    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 3) {
        ALOGI("Skipped the test since"
                " setCellularIdentifierTransparencyEnabled is not supported on version < 3.");
        GTEST_SKIP();
    }

    // Get current value
    serial = GetRandomSerialNumber();
    radio_network->isCellularIdentifierTransparencyEnabled(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    bool originalTransparencySetting = radioRsp_network->isCellularIdentifierTransparencyEnabled;

    // We want to test flipping the value, so we are going to set it to the opposite of what
    // the existing setting is. The test for isCellularIdentifierTransparencyEnabled should check
    // for the right default value.
    bool valueToSet = !originalTransparencySetting;
    serial = GetRandomSerialNumber();
    radio_network->setCellularIdentifierTransparencyEnabled(serial, valueToSet);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));

    // Assert the value has changed
    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = radio_network->isCellularIdentifierTransparencyEnabled(serial);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));
    EXPECT_EQ(valueToSet, radioRsp_network->isCellularIdentifierTransparencyEnabled);

    // Reset original state
    radio_network->setCellularIdentifierTransparencyEnabled(serial, originalTransparencySetting);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
}

/*
 * Test IRadioNetwork.setSecurityAlgorithmsUpdatedEnabled for the response returned.
 */
TEST_P(RadioNetworkTest, setSecurityAlgorithmsUpdatedEnabled) {
    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 3) {
        ALOGI("Skipped the test since"
              " setSecurityAlgorithmsUpdatedEnabled is not supported on version < 3");
        GTEST_SKIP();
    }

    // Get current value
    serial = GetRandomSerialNumber();
    radio_network->isSecurityAlgorithmsUpdatedEnabled(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    bool originalSecuritySetting = radioRsp_network->isSecurityAlgorithmsUpdatedEnabled;

    // We want to test flipping the value, so we are going to set it to the opposite of what
    // the existing setting is. The test for isSecurityAlgorithmsUpdatedEnabled should check
    // for the right default value.
    bool valueToSet = !originalSecuritySetting;
    serial = GetRandomSerialNumber();
    radio_network->setSecurityAlgorithmsUpdatedEnabled(serial, valueToSet);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));

    // Assert the value has changed
    serial = GetRandomSerialNumber();
    ndk::ScopedAStatus res = radio_network->isSecurityAlgorithmsUpdatedEnabled(serial);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));
    EXPECT_EQ(valueToSet, radioRsp_network->isSecurityAlgorithmsUpdatedEnabled);

    // Reset original state
    radio_network->setSecurityAlgorithmsUpdatedEnabled(serial, originalSecuritySetting);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);
}

/**
 * Test IRadioNetwork.isSecurityAlgorithmsUpdatedEnabled for the response returned.
 */
TEST_P(RadioNetworkTest, isSecurityAlgorithmsUpdatedEnabled) {
    int32_t aidl_version;
    ndk::ScopedAStatus aidl_status = radio_network->getInterfaceVersion(&aidl_version);
    ASSERT_OK(aidl_status);
    if (aidl_version < 3) {
        ALOGI("Skipped the test since"
              " isSecurityAlgorithmsUpdatedEnabled is not supported on version < 3");
        GTEST_SKIP();
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_network->isSecurityAlgorithmsUpdatedEnabled(serial);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_network->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_network->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_network->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR}));
}
