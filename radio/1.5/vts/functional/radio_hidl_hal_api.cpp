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
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_invalidHysteresisDb) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSI;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 10;  // hysteresisDb too large given threshold list deltas
    signalThresholdInfo.thresholds = {-109, -103, -97, -89};

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
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_EmptyThresholds) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSI;
    signalThresholdInfo.hysteresisMs = 0;
    signalThresholdInfo.hysteresisDb = 0;

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
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Geran) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSI;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-109, -103, -97, -89};

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
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Utran) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSCP;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-110, -97, -73, -49, -25};

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
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Eutran_RSRP) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSRP;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-128, -108, -88, -68};

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
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Eutran_RSRQ) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSRQ;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-27, -20, -13, -6};

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
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Eutran_RSSNR) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSNR;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-10, 0, 10, 20};

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
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_Cdma2000) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::RSSI;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 2;
    signalThresholdInfo.thresholds = {-105, -90, -75, -65};

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
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_NGRAN_SSRSRP) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::SSRSRP;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.thresholds = {-105, -90, -75, -65};

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
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_NGRAN_SSRSRQ) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::SSRSRQ;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.thresholds = {-15, -10, -5, -4};

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
 * Test IRadio.setSignalStrengthReportingCriteria_1_5() for NGRAN_SSSINR
 */
TEST_F(RadioHidlTest_v1_5, setSignalStrengthReportingCriteria_1_5_NGRAN_SSSINR) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_5::SignalThresholdInfo signalThresholdInfo;
    signalThresholdInfo.signalMeasurement = SignalMeasurementType::SSSINR;
    signalThresholdInfo.hysteresisMs = 5000;
    signalThresholdInfo.hysteresisDb = 0;
    signalThresholdInfo.thresholds = {-10, 3, 16, 18};

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
