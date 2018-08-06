/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_2.h>
#include <vector>

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

/*
 * Test IRadio.startNetworkScan() for the response returned.
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {
        .radioAccessNetwork = RadioAccessNetworks::GERAN,
        .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
        .channels = {1,2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
        .type = ScanType::ONE_SHOT, .interval = 60, .specifiers = {specifier}};

    Return<void> res = radio_v1_2->startNetworkScan_1_2(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan, rspInfo.error = %s\n", toString(radioRsp_v1_2->rspInfo.error).c_str());
    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error, {RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        // REQUEST_NOT_SUPPORTED should not be allowed as it is not an optional API. However, the
        // comments in the hal were not updated to indicate that, hence allowing it as a valid
        // error for now. This should be fixed correctly, possibly in a future version of the hal
        // (b/110421924). This is being allowed because some vendors do not support
        // this request on dual sim devices.
        // OPERATION_NOT_ALLOWED should not be allowed; however, some vendors do not support the
        // required manual GSM search functionality. This is tracked in b/112206766.
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                     {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED,
                                      RadioError::OPERATION_NOT_ALLOWED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid specifier.
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidArgument) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
                                                                    .interval = 60};

    Return<void> res = radio_v1_2->startNetworkScan_1_2(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                             {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid interval (lower boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidInterval1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {
        .radioAccessNetwork = RadioAccessNetworks::GERAN,
        .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
        .channels = {1,2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
        .type = ScanType::ONE_SHOT,
        .interval = 4,
        .specifiers = {specifier},
        .maxSearchTime = 60,
        .incrementalResults = false,
        .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_2->startNetworkScan_1_2(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidInterval1, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                             {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid interval (upper boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidInterval2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {
        .radioAccessNetwork = RadioAccessNetworks::GERAN,
        .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
        .channels = {1,2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
        .type = ScanType::ONE_SHOT,
        .interval = 301,
        .specifiers = {specifier},
        .maxSearchTime = 60,
        .incrementalResults = false,
        .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_2->startNetworkScan_1_2(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidInterval2, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                             {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid max search time (lower boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidMaxSearchTime1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {
        .radioAccessNetwork = RadioAccessNetworks::GERAN,
        .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
        .channels = {1,2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
        .type = ScanType::ONE_SHOT,
        .interval = 60,
        .specifiers = {specifier},
        .maxSearchTime = 59,
        .incrementalResults = false,
        .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_2->startNetworkScan_1_2(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidMaxSearchTime1, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                             {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid max search time (upper boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidMaxSearchTime2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {
        .radioAccessNetwork = RadioAccessNetworks::GERAN,
        .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
        .channels = {1,2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
        .type = ScanType::ONE_SHOT,
        .interval = 60,
        .specifiers = {specifier},
        .maxSearchTime = 3601,
        .incrementalResults = false,
        .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_2->startNetworkScan_1_2(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidMaxSearchTime2, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                             {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid periodicity (lower boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidPeriodicity1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {
        .radioAccessNetwork = RadioAccessNetworks::GERAN,
        .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
        .channels = {1,2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
        .type = ScanType::ONE_SHOT,
        .interval = 60,
        .specifiers = {specifier},
        .maxSearchTime = 600,
        .incrementalResults = false,
        .incrementalResultsPeriodicity = 0};

    Return<void> res = radio_v1_2->startNetworkScan_1_2(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidPeriodicity1, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                             {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid periodicity (upper boundary).
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_InvalidPeriodicity2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {
        .radioAccessNetwork = RadioAccessNetworks::GERAN,
        .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
        .channels = {1,2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
        .type = ScanType::ONE_SHOT,
        .interval = 60,
        .specifiers = {specifier},
        .maxSearchTime = 600,
        .incrementalResults = false,
        .incrementalResultsPeriodicity = 11};

    Return<void> res = radio_v1_2->startNetworkScan_1_2(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidPeriodicity2, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(
            CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                             {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with valid periodicity
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_GoodRequest1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {
        .radioAccessNetwork = RadioAccessNetworks::GERAN,
        .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
        .channels = {1,2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
        .type = ScanType::ONE_SHOT,
        .interval = 60,
        .specifiers = {specifier},
        // Some vendor may not support max search time of 360s.
        // This issue is tracked in b/112205669.
        .maxSearchTime = 300,
        .incrementalResults = false,
        .incrementalResultsPeriodicity = 10};

    Return<void> res = radio_v1_2->startNetworkScan_1_2(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_2->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with valid periodicity and plmns
 */
TEST_F(RadioHidlTest_v1_2, startNetworkScan_GoodRequest2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {
        .radioAccessNetwork = RadioAccessNetworks::GERAN,
        .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
        .channels = {1,2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
        .type = ScanType::ONE_SHOT,
        .interval = 60,
        .specifiers = {specifier},
        // Some vendor may not support max search time of 360s.
        // This issue is tracked in b/112205669.
        .maxSearchTime = 300,
        .incrementalResults = false,
        .incrementalResultsPeriodicity = 10,
        .mccMncs = {"310410"}};

    Return<void> res = radio_v1_2->startNetworkScan_1_2(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_2->rspInfo.error,
            {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.setIndicationFilter_1_2()
 */
TEST_F(RadioHidlTest_v1_2, setIndicationFilter_1_2) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setIndicationFilter_1_2(
        serial, static_cast<int>(::android::hardware::radio::V1_2::IndicationFilter::ALL));
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setIndicationFilter_1_2, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria() with invalid hysteresisDb
 */
TEST_F(RadioHidlTest_v1_2, setSignalStrengthReportingCriteria_invalidHysteresisDb) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setSignalStrengthReportingCriteria(
        serial, 5000,
        10,  // hysteresisDb too large given threshold list deltas
        {-109, -103, -97, -89}, ::android::hardware::radio::V1_2::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_invalidHysteresisDb, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error, {RadioError::INVALID_ARGUMENTS}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria() with empty parameters
 */
TEST_F(RadioHidlTest_v1_2, setSignalStrengthReportingCriteria_EmptyParams) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setSignalStrengthReportingCriteria(
        serial, 0, 0, {}, ::android::hardware::radio::V1_2::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_EmptyParams, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria() for GERAN
 */
TEST_F(RadioHidlTest_v1_2, setSignalStrengthReportingCriteria_Geran) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setSignalStrengthReportingCriteria(
        serial, 5000, 2, {-109, -103, -97, -89},
        ::android::hardware::radio::V1_2::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_Geran, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria() for UTRAN
 */
TEST_F(RadioHidlTest_v1_2, setSignalStrengthReportingCriteria_Utran) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setSignalStrengthReportingCriteria(
        serial, 5000, 2, {-110, -97, -73, -49, -25},
        ::android::hardware::radio::V1_2::AccessNetwork::UTRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_Utran, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria() for EUTRAN
 */
TEST_F(RadioHidlTest_v1_2, setSignalStrengthReportingCriteria_Eutran) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setSignalStrengthReportingCriteria(
        serial, 5000, 2, {-140, -128, -118, -108, -98, -44},
        ::android::hardware::radio::V1_2::AccessNetwork::EUTRAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_Eutran, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setSignalStrengthReportingCriteria() for CDMA2000
 */
TEST_F(RadioHidlTest_v1_2, setSignalStrengthReportingCriteria_Cdma2000) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setSignalStrengthReportingCriteria(
        serial, 5000, 2, {-105, -90, -75, -65},
        ::android::hardware::radio::V1_2::AccessNetwork::CDMA2000);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setSignalStrengthReportingCriteria_Cdma2000, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error, {RadioError::NONE}));
}

/*
 * Test IRadio.setLinkCapacityReportingCriteria() invalid hysteresisDlKbps
 */
TEST_F(RadioHidlTest_v1_2, setLinkCapacityReportingCriteria_invalidHysteresisDlKbps) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setLinkCapacityReportingCriteria(
        serial, 5000,
        5000,  // hysteresisDlKbps too big for thresholds delta
        100, {1000, 5000, 10000, 20000}, {500, 1000, 5000, 10000},
        ::android::hardware::radio::V1_2::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_invalidHysteresisDlKbps, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    // Allow REQUEST_NOT_SUPPORTED as setLinkCapacityReportingCriteria() may not be supported for
    // GERAN
    ASSERT_TRUE(
        CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                         {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadio.setLinkCapacityReportingCriteria() invalid hysteresisUlKbps
 */
TEST_F(RadioHidlTest_v1_2, setLinkCapacityReportingCriteria_invalidHysteresisUlKbps) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setLinkCapacityReportingCriteria(
        serial, 5000, 500,
        1000,  // hysteresisUlKbps too big for thresholds delta
        {1000, 5000, 10000, 20000}, {500, 1000, 5000, 10000},
        ::android::hardware::radio::V1_2::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_invalidHysteresisUlKbps, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    // Allow REQUEST_NOT_SUPPORTED as setLinkCapacityReportingCriteria() may not be supported for
    // GERAN
    ASSERT_TRUE(
        CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                         {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadio.setLinkCapacityReportingCriteria() empty params
 */
TEST_F(RadioHidlTest_v1_2, setLinkCapacityReportingCriteria_emptyParams) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setLinkCapacityReportingCriteria(
        serial, 0, 0, 0, {}, {}, ::android::hardware::radio::V1_2::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_emptyParams, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    // Allow REQUEST_NOT_SUPPORTED as setLinkCapacityReportingCriteria() may not be supported for
    // GERAN
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadio.setLinkCapacityReportingCriteria() GERAN
 */
TEST_F(RadioHidlTest_v1_2, setLinkCapacityReportingCriteria_Geran) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->setLinkCapacityReportingCriteria(
        serial, 5000, 500, 100, {1000, 5000, 10000, 20000}, {500, 1000, 5000, 10000},
        ::android::hardware::radio::V1_2::AccessNetwork::GERAN);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("setLinkCapacityReportingCriteria_invalidHysteresisUlKbps, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    // Allow REQUEST_NOT_SUPPORTED as setLinkCapacityReportingCriteria() may not be supported for
    // GERAN
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/*
 * Test IRadio.setupDataCall_1_2() for the response returned.
 */
TEST_F(RadioHidlTest_v1_2, setupDataCall_1_2) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::AccessNetwork accessNetwork =
        ::android::hardware::radio::V1_2::AccessNetwork::EUTRAN;

    DataProfileInfo dataProfileInfo;
    memset(&dataProfileInfo, 0, sizeof(dataProfileInfo));
    dataProfileInfo.profileId = DataProfileId::IMS;
    dataProfileInfo.apn = hidl_string("VZWIMS");
    dataProfileInfo.protocol = hidl_string("IPV4V6");
    dataProfileInfo.roamingProtocol = hidl_string("IPV6");
    dataProfileInfo.authType = ApnAuthType::NO_PAP_NO_CHAP;
    dataProfileInfo.user = "";
    dataProfileInfo.password = "";
    dataProfileInfo.type = DataProfileInfoType::THREE_GPP2;
    dataProfileInfo.maxConnsTime = 300;
    dataProfileInfo.maxConns = 20;
    dataProfileInfo.waitTime = 0;
    dataProfileInfo.enabled = true;
    dataProfileInfo.supportedApnTypesBitmap = 320;
    dataProfileInfo.bearerBitmap = 161543;
    dataProfileInfo.mtu = 0;
    dataProfileInfo.mvnoType = MvnoType::NONE;
    dataProfileInfo.mvnoMatchData = hidl_string();

    bool modemCognitive = false;
    bool roamingAllowed = false;
    bool isRoaming = false;

    ::android::hardware::radio::V1_2::DataRequestReason reason =
        ::android::hardware::radio::V1_2::DataRequestReason::NORMAL;
    std::vector<hidl_string> addresses = {""};
    std::vector<hidl_string> dnses = {""};

    Return<void> res = radio_v1_2->setupDataCall_1_2(serial, accessNetwork, dataProfileInfo,
                                                     modemCognitive, roamingAllowed, isRoaming,
                                                     reason, addresses, dnses);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_2->rspInfo.error,
            {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE, RadioError::INVALID_ARGUMENTS,
             RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW, RadioError::REQUEST_NOT_SUPPORTED}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_2->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INVALID_ARGUMENTS,
             RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.deactivateDataCall_1_2() for the response returned.
 */
TEST_F(RadioHidlTest_v1_2, deactivateDataCall_1_2) {
    serial = GetRandomSerialNumber();
    int cid = 1;
    ::android::hardware::radio::V1_2::DataRequestReason reason =
        ::android::hardware::radio::V1_2::DataRequestReason::NORMAL;

    Return<void> res = radio_v1_2->deactivateDataCall_1_2(serial, cid, reason);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    if (cardStatus.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_2->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INVALID_CALL_ID,
             RadioError::INVALID_STATE, RadioError::INVALID_ARGUMENTS,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::CANCELLED, RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(
            radioRsp_v1_2->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INVALID_CALL_ID,
             RadioError::INVALID_STATE, RadioError::INVALID_ARGUMENTS,
             RadioError::REQUEST_NOT_SUPPORTED, RadioError::CANCELLED}));
    }
}

/*
 * Test IRadio.getCellInfoList() for the response returned.
 */
TEST_F(RadioHidlTest_v1_2, getCellInfoList_1_2) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->getCellInfoList(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("getCellInfoList_1_2, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                 {RadioError::NONE, RadioError::NO_NETWORK_FOUND}));
}

/*
 * Test IRadio.getVoiceRegistrationState() for the response returned.
 */
TEST_F(RadioHidlTest_v1_2, getVoiceRegistrationState) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->getVoiceRegistrationState(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("getVoiceRegistrationStateResponse_1_2, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                                 {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE}));
}

/*
 * Test IRadio.getDataRegistrationState() for the response returned.
 */
TEST_F(RadioHidlTest_v1_2, getDataRegistrationState) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->getDataRegistrationState(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);

    ALOGI("getVoiceRegistrationStateResponse_1_2, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(CheckAnyOfErrors(
        radioRsp_v1_2->rspInfo.error,
        {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::NOT_PROVISIONED}));
}

/*
 * Test IRadio.getAvailableBandModes() for the response returned.
 */
TEST_F(RadioHidlTest_v1_2, getAvailableBandModes) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_2->getAvailableBandModes(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);
    ALOGI("getAvailableBandModes, rspInfo.error = %s\n",
          toString(radioRsp_v1_2->rspInfo.error).c_str());
    ASSERT_TRUE(
        CheckAnyOfErrors(radioRsp_v1_2->rspInfo.error,
                         {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::MODEM_ERR,
                          RadioError::INTERNAL_ERR,
                          // If REQUEST_NOT_SUPPORTED is returned, then it should also be returned
                          // for setRandMode().
                          RadioError::REQUEST_NOT_SUPPORTED}));
    bool hasUnspecifiedBandMode = false;
    if (radioRsp_v1_2->rspInfo.error == RadioError::NONE) {
        for (const RadioBandMode& mode : radioRsp_v1_2->radioBandModes) {
            // Automatic mode selection must be supported
            if (mode == RadioBandMode::BAND_MODE_UNSPECIFIED) hasUnspecifiedBandMode = true;
        }
        ASSERT_TRUE(hasUnspecifiedBandMode);
    }
}
