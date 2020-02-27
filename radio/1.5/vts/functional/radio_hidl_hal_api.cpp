/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_5.h>

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() with invalid hysteresisDb
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_invalidHysteresisDb) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSI;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 10;  // hysteresisDb too large given threshold list deltas
    signalThresholdInfo.thresholds = {-109, -103, -97, -89};
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_1_5_invalidHysteresisDb, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() with empty thresholds
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_EmptyThresholds) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSI;
    signalThresholdInfo.hysteresisMs = 0;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_1_5_EmptyParams, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for GERAN
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Geran) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSI;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-109, -103, -97, -89};
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_1_5_Geran, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for UTRAN
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Utran) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSCP;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-110, -97, -73, -49, -25};
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::UTRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_1_5_Utran, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for EUTRAN
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Eutran_RSRP) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSRP;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-128, -108, -88, -68};
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::EUTRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_1_5_Eutran, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for EUTRAN
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Eutran_RSRQ) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSRQ;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-27, -20, -13, -6};
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::EUTRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_1_5_Eutran, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for EUTRAN
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Eutran_RSSNR) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSNR;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-10, 0, 10, 20};
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::EUTRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for CDMA2000
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Cdma2000) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSI;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-105, -90, -75, -65};
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::CDMA2000);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_1_5_Cdma2000, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for NGRAN_SSRSRP
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_NGRAN_SSRSRP) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::SSRSRP;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.thresholds = {-105, -90, -75, -65};
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::NGRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_1_5_NGRAN_SSRSRP, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for NGRAN_SSRSRQ
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_NGRAN_SSRSRQ) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::SSRSRQ;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.thresholds = {-15, -10, -5, -4};
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::NGRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_1_5_NGRAN_SSRSRQ, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for EUTRAN
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Disable_RSSNR) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSNR;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-10, 0, 10, 20};
    signalThresholdInfo.isEnabled = false;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::EUTRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for NGRAN_SSSINR
 */
TEST_P(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_NGRAN_SSSINR) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::SSSINR;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.thresholds = {-10, 3, 16, 18};
    signalThresholdInfo.isEnabled = true;

    Return<void> res = radio_v1_5->setSignalStrengthReportingCriteria_1_5(
            serial, signalThresholdInfo, ::android::hardware::radio::V1_5::AccessNetwork::NGRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_1_5_NGRAN_SSSINR, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setLinkCapacityReportingCriteria_1_5() invalid hysteresisDlKbps
 */
TEST_P(RadioHidlTest_v1_5, setLinkCapacityReportingCriteria_1_5_invalidHysteresisDlKbps) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_5->setLinkCapacityReportingCriteria_1_5(
            serial, 5000,
            5000,  // hysteresisDlKbps too big for thresholds delta
            100, {1000, 5000, 10000, 20000}, {500, 1000, 5000, 10000},
            ::android::hardware::radio::V1_5::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_1_5_invalidHysteresisDlKbps, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    // Allow REQUEST_NOT_SUPPORTED as setLinkCapacityReportingCriteria_1_5() may not be supported
    // for GERAN
    ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                             {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadio.setLinkCapacityReportingCriteria_1_5() invalid hysteresisUlKbps
 */
TEST_P(RadioHidlTest_v1_5, setLinkCapacityReportingCriteria_1_5_invalidHysteresisUlKbps) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_5->setLinkCapacityReportingCriteria_1_5(
            serial, 5000, 500,
            1000,  // hysteresisUlKbps too big for thresholds delta
            {1000, 5000, 10000, 20000}, {500, 1000, 5000, 10000},
            ::android::hardware::radio::V1_5::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_1_5_invalidHysteresisUlKbps, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    // Allow REQUEST_NOT_SUPPORTED as setLinkCapacityReportingCriteria_1_5() may not be supported
    // for GERAN
    ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                             {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadio.setLinkCapacityReportingCriteria_1_5() empty params
 */
TEST_P(RadioHidlTest_v1_5, setLinkCapacityReportingCriteria_1_5_emptyParams) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_5->setLinkCapacityReportingCriteria_1_5(
            serial, 0, 0, 0, {}, {}, ::android::hardware::radio::V1_5::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_1_5_emptyParams, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    // Allow REQUEST_NOT_SUPPORTED as setLinkCapacityReportingCriteria_1_5() may not be supported
    // for GERAN
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadio.setLinkCapacityReportingCriteria_1_5() for GERAN
 */
TEST_P(RadioHidlTest_v1_5, setLinkCapacityReportingCriteria_1_5_Geran) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_5->setLinkCapacityReportingCriteria_1_5(
            serial, 5000, 500, 100, {1000, 5000, 10000, 20000}, {500, 1000, 5000, 10000},
            ::android::hardware::radio::V1_5::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_1_5_Geran, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    // Allow REQUEST_NOT_SUPPORTED as setLinkCapacityReportingCriteria_1_5() may not be supported
    // for GERAN
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadio.enableUiccApplications() for the response returned.
 * For SIM ABSENT case.
 */
TEST_P(RadioHidlTest_v1_5, togglingUiccApplicationsSimAbsent) {
    // This test case only test SIM ABSENT case.
    if (cardStatus.base.base.cardState != CardState::ABSENT) return;

    // Disable Uicc applications.
    serial = GetRandomSerialNumber();
    radio_v1_5->enableUiccApplications(serial, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    // As SIM is absent, RadioError::SIM_ABSENT should be thrown.
    EXPECT_EQ(RadioError::SIM_ABSENT, radioRsp_v1_5->rspInfo.error);

    // Query Uicc application enablement.
    serial = GetRandomSerialNumber();
    radio_v1_5->areUiccApplicationsEnabled(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    // As SIM is absent, RadioError::SIM_ABSENT should be thrown.
    EXPECT_EQ(RadioError::SIM_ABSENT, radioRsp_v1_5->rspInfo.error);
}

/*
 * Test IRadio.enableUiccApplications() for the response returned.
 * For SIM PRESENT case.
 */
TEST_P(RadioHidlTest_v1_5, togglingUiccApplicationsSimPresent) {
    // This test case only test SIM ABSENT case.
    if (cardStatus.base.base.cardState != CardState::PRESENT) return;

    // Disable Uicc applications.
    serial = GetRandomSerialNumber();
    radio_v1_5->enableUiccApplications(serial, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    // As SIM is present, there shouldn't be error.
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_5->rspInfo.error);

    // Query Uicc application enablement.
    serial = GetRandomSerialNumber();
    radio_v1_5->areUiccApplicationsEnabled(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    // As SIM is present, there shouldn't be error.
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_5->rspInfo.error);
    ASSERT_FALSE(radioRsp_v1_5->areUiccApplicationsEnabled);

    // Enable Uicc applications.
    serial = GetRandomSerialNumber();
    radio_v1_5->enableUiccApplications(serial, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    // As SIM is present, there shouldn't be error.
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_5->rspInfo.error);

    // Query Uicc application enablement.
    serial = GetRandomSerialNumber();
    radio_v1_5->areUiccApplicationsEnabled(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    // As SIM is present, there shouldn't be error.
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_5->rspInfo.error);
    ASSERT_TRUE(radioRsp_v1_5->areUiccApplicationsEnabled);
}

/*
 * Test IRadio.areUiccApplicationsEnabled() for the response returned.
 */
TEST_P(RadioHidlTest_v1_5, areUiccApplicationsEnabled) {
    // Disable Uicc applications.
    serial = GetRandomSerialNumber();
    radio_v1_5->areUiccApplicationsEnabled(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    // If SIM is absent, RadioError::SIM_ABSENT should be thrown. Otherwise there shouldn't be any
    // error.
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::SIM_ABSENT, radioRsp_v1_5->rspInfo.error);
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp_v1_5->rspInfo.error);
    }
}

/*
 * Test IRadio.setSystemSelectionChannels_1_5() for the response returned.
 */
TEST_P(RadioHidlTest_v1_5, setSystemSelectionChannels_1_5) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands rasBands;
    rasBands.geranBands() = {GeranBands::BAND_450, GeranBands::BAND_480};

    ::android::hardware::radio::V1_5::RadioAccessSpecifier specifier = {
            .radioAccessNetwork = ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN,
            .bands = rasBands,
            .channels = {1, 2}};

    Return<void> res = radio_v1_5->setSystemSelectionChannels_1_5(serial, true, {specifier});
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("setSystemSelectionChannels, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_5->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));

    if (radioRsp_v1_5->rspInfo.error == RadioError::NONE) {
        serial = GetRandomSerialNumber();
        Return<void> res = radio_v1_5->setSystemSelectionChannels_1_5(serial, false, {specifier});
        ASSERT_OK(res);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
        ALOGI("setSystemSelectionChannels, rspInfo.error = %s\n",
              toString(radioRsp_v1_5->rspInfo.error).c_str());
        EXPECT_EQ(RadioError::NONE, radioRsp_v1_5->rspInfo.error);
    }
}

/*
 * Test IRadio.startNetworkScan_1_5() for the response returned.
 */
TEST_P(RadioHidlTest_v1_5, startNetworkScan) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands rasBands;
    rasBands.geranBands() = {GeranBands::BAND_450, GeranBands::BAND_480};

    ::android::hardware::radio::V1_5::RadioAccessSpecifier specifier = {
            .radioAccessNetwork = ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN,
            .bands = rasBands,
            .channels = {1, 2}};

    ::android::hardware::radio::V1_5::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 60,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_5->startNetworkScan_1_5(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("startNetworkScan, rspInfo.error = %s\n", toString(radioRsp_v1_5->rspInfo.error).c_str());

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error, {RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        // OPERATION_NOT_ALLOWED should not be allowed; however, some vendors do
        // not support the required manual GSM search functionality. This is
        // tracked in b/112206766. Modems have "GSM" rat scan need to
        // support scanning requests combined with some parameters.
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::NONE, RadioError::OPERATION_NOT_ALLOWED}));
    }
}

/*
 * Test IRadio.startNetworkScan_1_5() with invalid specifier.
 */
TEST_P(RadioHidlTest_v1_5, startNetworkScan_InvalidArgument) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
                                                                    .interval = 60};

    Return<void> res = radio_v1_5->startNetworkScan_1_5(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_5->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan_1_5() with invalid interval (lower boundary).
 */
TEST_P(RadioHidlTest_v1_5, startNetworkScan_InvalidInterval1) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands rasBands;
    rasBands.geranBands() = {GeranBands::BAND_450, GeranBands::BAND_480};

    ::android::hardware::radio::V1_5::RadioAccessSpecifier specifier = {
            .radioAccessNetwork = ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN,
            .bands = rasBands,
            .channels = {1, 2}};

    ::android::hardware::radio::V1_5::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 4,
            .specifiers = {specifier},
            .maxSearchTime = 60,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_5->startNetworkScan_1_5(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidInterval1, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_5->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan_1_5() with invalid interval (upper boundary).
 */
TEST_P(RadioHidlTest_v1_5, startNetworkScan_InvalidInterval2) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands rasBands;
    rasBands.geranBands() = {GeranBands::BAND_450, GeranBands::BAND_480};

    ::android::hardware::radio::V1_5::RadioAccessSpecifier specifier = {
            .radioAccessNetwork = ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN,
            .bands = rasBands,
            .channels = {1, 2}};

    ::android::hardware::radio::V1_5::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 301,
            .specifiers = {specifier},
            .maxSearchTime = 60,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_5->startNetworkScan_1_5(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidInterval2, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_5->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan_1_5() with invalid max search time (lower boundary).
 */
TEST_P(RadioHidlTest_v1_5, startNetworkScan_InvalidMaxSearchTime1) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands rasBands;
    rasBands.geranBands() = {GeranBands::BAND_450, GeranBands::BAND_480};

    ::android::hardware::radio::V1_5::RadioAccessSpecifier specifier = {
            .radioAccessNetwork = ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN,
            .bands = rasBands,
            .channels = {1, 2}};

    ::android::hardware::radio::V1_5::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 59,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_5->startNetworkScan_1_5(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidMaxSearchTime1, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_5->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan_1_5() with invalid max search time (upper boundary).
 */
TEST_P(RadioHidlTest_v1_5, startNetworkScan_InvalidMaxSearchTime2) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands rasBands;
    rasBands.geranBands() = {GeranBands::BAND_450, GeranBands::BAND_480};

    ::android::hardware::radio::V1_5::RadioAccessSpecifier specifier = {
            .radioAccessNetwork = ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN,
            .bands = rasBands,
            .channels = {1, 2}};

    ::android::hardware::radio::V1_5::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 3601,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_5->startNetworkScan_1_5(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidMaxSearchTime2, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_5->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan_1_5() with invalid periodicity (lower boundary).
 */
TEST_P(RadioHidlTest_v1_5, startNetworkScan_InvalidPeriodicity1) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands rasBands;
    rasBands.geranBands() = {GeranBands::BAND_450, GeranBands::BAND_480};

    ::android::hardware::radio::V1_5::RadioAccessSpecifier specifier = {
            .radioAccessNetwork = ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN,
            .bands = rasBands,
            .channels = {1, 2}};

    ::android::hardware::radio::V1_5::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 600,
            .incrementalResults = true,
            .incrementalResultsPeriodicity = 0};

    Return<void> res = radio_v1_5->startNetworkScan_1_5(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidPeriodicity1, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_5->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan_1_5() with invalid periodicity (upper boundary).
 */
TEST_P(RadioHidlTest_v1_5, startNetworkScan_InvalidPeriodicity2) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands rasBands;
    rasBands.geranBands() = {GeranBands::BAND_450, GeranBands::BAND_480};

    ::android::hardware::radio::V1_5::RadioAccessSpecifier specifier = {
            .radioAccessNetwork = ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN,
            .bands = rasBands,
            .channels = {1, 2}};

    ::android::hardware::radio::V1_5::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 600,
            .incrementalResults = true,
            .incrementalResultsPeriodicity = 11};

    Return<void> res = radio_v1_5->startNetworkScan_1_5(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("startNetworkScan_InvalidPeriodicity2, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_5->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan_1_5() with valid periodicity
 */
TEST_P(RadioHidlTest_v1_5, startNetworkScan_GoodRequest1) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands rasBands;
    rasBands.geranBands() = {GeranBands::BAND_450, GeranBands::BAND_480};

    ::android::hardware::radio::V1_5::RadioAccessSpecifier specifier = {
            .radioAccessNetwork = ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN,
            .bands = rasBands,
            .channels = {1, 2}};

    ::android::hardware::radio::V1_5::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 360,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 10};

    Return<void> res = radio_v1_5->startNetworkScan_1_5(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("startNetworkScan_GoodRequest1, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                      RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan_1_5() with valid periodicity and plmns
 */
TEST_P(RadioHidlTest_v1_5, startNetworkScan_GoodRequest2) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::RadioAccessSpecifier::Bands rasBands;
    rasBands.geranBands() = {GeranBands::BAND_450, GeranBands::BAND_480};

    ::android::hardware::radio::V1_5::RadioAccessSpecifier specifier = {
            .radioAccessNetwork = ::android::hardware::radio::V1_5::RadioAccessNetworks::GERAN,
            .bands = rasBands,
            .channels = {1, 2}};

    ::android::hardware::radio::V1_5::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 360,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 10,
            .mccMncs = {"310410"}};

    Return<void> res = radio_v1_5->startNetworkScan_1_5(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    ALOGI("startNetworkScan_GoodRequest2, rspInfo.error = %s\n",
          toString(radioRsp_v1_5->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                                      RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.setupDataCall_1_5() for the response returned.
 */
TEST_P(RadioHidlTest_v1_5, setupDataCall_1_5) {
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

    Return<void> res = radio_v1_5->setupDataCall_1_5(serial, accessNetwork, dataProfileInfo,
                                                     roamingAllowed, reason, addresses, dnses);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    }
}

/*
 * Test IRadio.setInitialAttachApn_1_5() for the response returned.
 */
TEST_P(RadioHidlTest_v1_5, setInitialAttachApn_1_5) {
    serial = GetRandomSerialNumber();

    // Create a dataProfileInfo
    android::hardware::radio::V1_5::DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileId::DEFAULT;
    dataProfileInfo.apn = hidl_string("internet");
    dataProfileInfo.protocol = PdpProtocolType::IPV4V6;
    dataProfileInfo.roamingProtocol = PdpProtocolType::IPV4V6;
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

    radio_v1_5->setInitialAttachApn_1_5(serial, dataProfileInfo);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE}));
    }
}

/*
 * Test IRadio.setDataProfile_1_5() for the response returned.
 */
TEST_P(RadioHidlTest_v1_5, setDataProfile_1_5) {
    serial = GetRandomSerialNumber();

    // Create a dataProfileInfo
    android::hardware::radio::V1_5::DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileId::DEFAULT;
    dataProfileInfo.apn = hidl_string("internet");
    dataProfileInfo.protocol = PdpProtocolType::IPV4V6;
    dataProfileInfo.roamingProtocol = PdpProtocolType::IPV4V6;
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
    dataProfileInfo.persistent = true;

    // Create a dataProfileInfoList
    android::hardware::hidl_vec<android::hardware::radio::V1_5::DataProfileInfo>
            dataProfileInfoList = {dataProfileInfo};

    radio_v1_5->setDataProfile_1_5(serial, dataProfileInfoList);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE}));
    }
}

/*
 * Test IRadio.setRadioPower_1_5() for the response returned.
 */
TEST_P(RadioHidlTest_v1_5, setRadioPower_1_5_emergencyCall_cancelled) {
    // Set radio power to off.
    serial = GetRandomSerialNumber();
    radio_v1_5->setRadioPower_1_5(serial, false, false, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_5->rspInfo.error);

    // Set radio power to on with forEmergencyCall being true. This should put modem to only scan
    // emergency call bands.
    serial = GetRandomSerialNumber();
    radio_v1_5->setRadioPower_1_5(serial, true, true, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_5->rspInfo.error);

    // Set radio power to on with forEmergencyCall being false. This should put modem in regular
    // operation modem.
    serial = GetRandomSerialNumber();
    radio_v1_5->setRadioPower_1_5(serial, true, false, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_5->rspInfo.error);
}

/*
 * Test IRadio.setNetworkSelectionModeManual_1_5() for the response returned.
 */
TEST_P(RadioHidlTest_v1_5, setNetworkSelectionModeManual_1_5) {
    serial = GetRandomSerialNumber();

    // can't camp on nonexistent MCCMNC, so we expect this to fail.
    Return<void> res = radio_v1_5->setNetworkSelectionModeManual_1_5(
            serial, "123456", android::hardware::radio::V1_5::RadioAccessNetworks::GERAN);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::NONE, RadioError::ILLEGAL_SIM_OR_ME,
                                      RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE},
                                     CHECK_GENERAL_ERROR));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_5->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE},
                                     CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.sendCdmaSmsExpectMore() for the response returned.
 */
TEST_P(RadioHidlTest_v1_5, sendCdmaSmsExpectMore) {
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

    radio_v1_5->sendCdmaSmsExpectMore(serial, cdmaSmsMessage);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
                radioRsp_v1_5->rspInfo.error,
                {RadioError::INVALID_ARGUMENTS, RadioError::INVALID_STATE, RadioError::SIM_ABSENT},
                CHECK_GENERAL_ERROR));
    }
}

/*
 * Test IRadio.getBarringInfo() for the response returned.
 */
TEST_P(RadioHidlTest_v1_5, getBarringInfo) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_5->getBarringInfo(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_5->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_5->rspInfo.serial);

    ASSERT_TRUE(radioRsp_v1_5->barringInfos.size() > 0);

    std::set<BarringInfo::ServiceType> reportedServices;

    // validate that the service types are in range
    for (const auto& info : radioRsp_v1_5->barringInfos) {
        ASSERT_TRUE((info.serviceType >= BarringInfo::ServiceType::CS_SERVICE &&
                     info.serviceType <= BarringInfo::ServiceType::SMS) ||
                    (info.serviceType >= BarringInfo::ServiceType::OPERATOR_1 &&
                     info.serviceType <= BarringInfo::ServiceType::OPERATOR_32));
        reportedServices.insert(info.serviceType);

        // Any type that is "conditional" must have sane values for conditional barring
        // factor and time.
        switch (info.barringType) {
            case BarringInfo::BarringType::NONE:  // fall through
            case BarringInfo::BarringType::UNCONDITIONAL:
                break;
            case BarringInfo::BarringType::CONDITIONAL: {
                const int32_t barringFactor = info.barringTypeSpecificInfo.conditional().factor;
                ASSERT_TRUE(barringFactor >= 0 && barringFactor <= 100);
                ASSERT_TRUE(info.barringTypeSpecificInfo.conditional().timeSeconds > 0);
                break;
            }
            default:
                FAIL();
        }
    }

    // Certain types of barring are relevant for certain RANs. Ensure that only the right
    // types are reported. Note that no types are required, simply that for a given technology
    // only certain types are valid. This is one way to sanity check that implementations are
    // not providing information that they don't have.
    static const std::set<BarringInfo::ServiceType> UTRA_SERVICES{
            BarringInfo::ServiceType::CS_SERVICE, BarringInfo::ServiceType::PS_SERVICE,
            BarringInfo::ServiceType::CS_VOICE,   BarringInfo::ServiceType::EMERGENCY,
            BarringInfo::ServiceType::SMS,
    };

    static const std::set<BarringInfo::ServiceType> EUTRA_SERVICES{
            BarringInfo::ServiceType::MO_SIGNALLING, BarringInfo::ServiceType::MO_DATA,
            BarringInfo::ServiceType::CS_FALLBACK,   BarringInfo::ServiceType::MMTEL_VOICE,
            BarringInfo::ServiceType::MMTEL_VIDEO,   BarringInfo::ServiceType::EMERGENCY,
            BarringInfo::ServiceType::SMS,
    };

    static const std::set<BarringInfo::ServiceType> NGRA_SERVICES = {
            BarringInfo::ServiceType::MO_SIGNALLING, BarringInfo::ServiceType::MO_DATA,
            BarringInfo::ServiceType::CS_FALLBACK,   BarringInfo::ServiceType::MMTEL_VOICE,
            BarringInfo::ServiceType::MMTEL_VIDEO,   BarringInfo::ServiceType::EMERGENCY,
            BarringInfo::ServiceType::SMS,           BarringInfo::ServiceType::OPERATOR_1,
            BarringInfo::ServiceType::OPERATOR_2,    BarringInfo::ServiceType::OPERATOR_3,
            BarringInfo::ServiceType::OPERATOR_4,    BarringInfo::ServiceType::OPERATOR_5,
            BarringInfo::ServiceType::OPERATOR_6,    BarringInfo::ServiceType::OPERATOR_7,
            BarringInfo::ServiceType::OPERATOR_8,    BarringInfo::ServiceType::OPERATOR_9,
            BarringInfo::ServiceType::OPERATOR_10,   BarringInfo::ServiceType::OPERATOR_11,
            BarringInfo::ServiceType::OPERATOR_12,   BarringInfo::ServiceType::OPERATOR_13,
            BarringInfo::ServiceType::OPERATOR_14,   BarringInfo::ServiceType::OPERATOR_15,
            BarringInfo::ServiceType::OPERATOR_16,   BarringInfo::ServiceType::OPERATOR_17,
            BarringInfo::ServiceType::OPERATOR_18,   BarringInfo::ServiceType::OPERATOR_19,
            BarringInfo::ServiceType::OPERATOR_20,   BarringInfo::ServiceType::OPERATOR_21,
            BarringInfo::ServiceType::OPERATOR_22,   BarringInfo::ServiceType::OPERATOR_23,
            BarringInfo::ServiceType::OPERATOR_24,   BarringInfo::ServiceType::OPERATOR_25,
            BarringInfo::ServiceType::OPERATOR_26,   BarringInfo::ServiceType::OPERATOR_27,
            BarringInfo::ServiceType::OPERATOR_28,   BarringInfo::ServiceType::OPERATOR_29,
            BarringInfo::ServiceType::OPERATOR_30,   BarringInfo::ServiceType::OPERATOR_31,
    };

    const std::set<BarringInfo::ServiceType>* compareTo = nullptr;

    switch (radioRsp_v1_5->barringCellIdentity.getDiscriminator()) {
        case android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::wcdma:
            // fall through
        case android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::tdscdma:
            compareTo = &UTRA_SERVICES;
            break;
        case android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::lte:
            compareTo = &EUTRA_SERVICES;
            break;
        case android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::nr:
            compareTo = &NGRA_SERVICES;
            break;

        case android::hardware::radio::V1_5::CellIdentity::hidl_discriminator::cdma:
            // fall through
        default:
            FAIL();
            break;
    }

    std::set<BarringInfo::ServiceType> diff;

    std::set_difference(reportedServices.begin(), reportedServices.end(), compareTo->begin(),
                        compareTo->end(), std::inserter(diff, diff.begin()));
}
