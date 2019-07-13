/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_4.h>

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

/*
 * Test IRadio.emergencyDial() for the response returned.
 */
TEST_F(RadioHidlTest_v1_4, emergencyDial) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_0::Dial dialInfo;
    dialInfo.address = hidl_string("911");
    int categories = static_cast<int>(
            ::android::hardware::radio::V1_4::EmergencyServiceCategory::UNSPECIFIED);
    std::vector<hidl_string> urns = {""};
    ::android::hardware::radio::V1_4::EmergencyCallRouting routing =
            ::android::hardware::radio::V1_4::EmergencyCallRouting::UNKNOWN;

    Return<void> res =
            radio_v1_4->emergencyDial(serial, dialInfo, categories, urns, routing, true, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("emergencyDial, rspInfo.error = %s\n", toString(radioRsp_v1_4->rspInfo.error).c_str());
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_4->rspInfo.error);

    // Give some time for modem to establish the emergency call channel.
    sleep(MODEM_EMERGENCY_CALL_ESTABLISH_TIME);

    // Disconnect all the potential established calls to prevent them affecting other tests.
    clearPotentialEstablishedCalls();
}

/*
 * Test IRadio.emergencyDial() with specified service and its response returned.
 */
TEST_F(RadioHidlTest_v1_4, emergencyDial_withServices) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_0::Dial dialInfo;
    dialInfo.address = hidl_string("911");
    int categories =
            static_cast<int>(::android::hardware::radio::V1_4::EmergencyServiceCategory::AMBULANCE);
    std::vector<hidl_string> urns = {"urn:service:sos.ambulance"};
    ::android::hardware::radio::V1_4::EmergencyCallRouting routing =
            ::android::hardware::radio::V1_4::EmergencyCallRouting::UNKNOWN;

    Return<void> res =
            radio_v1_4->emergencyDial(serial, dialInfo, categories, urns, routing, true, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("emergencyDial_withServices, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_4->rspInfo.error);

    // Give some time for modem to establish the emergency call channel.
    sleep(MODEM_EMERGENCY_CALL_ESTABLISH_TIME);

    // Disconnect all the potential established calls to prevent them affecting other tests.
    clearPotentialEstablishedCalls();
}

/*
 * Test IRadio.emergencyDial() with known emergency call routing and its response returned.
 */
TEST_F(RadioHidlTest_v1_4, emergencyDial_withEmergencyRouting) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_0::Dial dialInfo;
    dialInfo.address = hidl_string("911");
    int categories = static_cast<int>(
            ::android::hardware::radio::V1_4::EmergencyServiceCategory::UNSPECIFIED);
    std::vector<hidl_string> urns = {""};
    ::android::hardware::radio::V1_4::EmergencyCallRouting routing =
            ::android::hardware::radio::V1_4::EmergencyCallRouting::EMERGENCY;

    Return<void> res =
            radio_v1_4->emergencyDial(serial, dialInfo, categories, urns, routing, true, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("emergencyDial_withEmergencyRouting, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_4->rspInfo.error);

    // Give some time for modem to establish the emergency call channel.
    sleep(MODEM_EMERGENCY_CALL_ESTABLISH_TIME);

    // Disconnect all the potential established calls to prevent them affecting other tests.
    clearPotentialEstablishedCalls();
}

/*
 * Test IRadio.getPreferredNetworkTypeBitmap() for the response returned.
 */
TEST_F(RadioHidlTest_v1_4, getPreferredNetworkTypeBitmap) {
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_4->getPreferredNetworkTypeBitmap(serial);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);
    ALOGI("getPreferredNetworkTypeBitmap, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_4->rspInfo.error);
}

TEST_F(RadioHidlTest_v1_4, setPreferredNetworkTypeBitmap) {
    serial = GetRandomSerialNumber();
    ::android::hardware::hidl_bitfield<::android::hardware::radio::V1_4::RadioAccessFamily>
            network_type_bitmap{};

    network_type_bitmap |= ::android::hardware::radio::V1_4::RadioAccessFamily::LTE;

    // TODO(b/131634656): LTE_CA will be sent to modem as a RAF in Q, but LTE_CA is not a RAF,
    // we will not send it to modem as a RAF in R.
    network_type_bitmap |= ::android::hardware::radio::V1_4::RadioAccessFamily::LTE_CA;

    Return<void> res = radio_v1_4->setPreferredNetworkTypeBitmap(serial, network_type_bitmap);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);
    ALOGI("setPreferredNetworkTypeBitmap, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_4->rspInfo.error);
    if (radioRsp_v1_4->rspInfo.error == RadioError::NONE) {
         // give some time for modem to set the value.
        sleep(3);
        serial = GetRandomSerialNumber();
        Return<void> res = radio_v1_4->getPreferredNetworkTypeBitmap(serial);

        ASSERT_OK(res);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);
        ALOGI("getPreferredNetworkTypeBitmap, rspInfo.error = %s\n",
              toString(radioRsp_v1_4->rspInfo.error).c_str());
        EXPECT_EQ(RadioError::NONE, radioRsp_v1_4->rspInfo.error);
        EXPECT_EQ(network_type_bitmap, radioRsp_v1_4->networkTypeBitmapResponse);
    }
}

/*
 * Test IRadio.startNetworkScan() for the response returned.
 *
 * REQUEST_NOT_SUPPORTED is temporarily returned because of vendors failed to fully implement
 * startNetworkScan in HAL @1.4 (see b/137298570 and b/135595082). Starting from @1.5, however,
 * REQUEST_NOT_SUPPORTED will be disallowed for all tests. Modems have "GSM" rat scan need to
 * support scanning requests combined with some parameters.
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
                                      .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
                                      .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT, .interval = 60, .specifiers = {specifier}};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan, rspInfo.error = %s\n", toString(radioRsp_v1_4->rspInfo.error).c_str());

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error, {RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
      // OPERATION_NOT_ALLOWED should not be allowed; however, some vendors do
      // not support the required manual GSM search functionality. This is
      // tracked in b/112206766. REQUEST_NOT_SUPPORTED is temporarily added back
      // because of vendors failed to implement startNetworkScan in HAL 1.4 (see
      // b/137298570 and b/135595082). Starting from 1.5, however,
      // REQUEST_NOT_SUPPORTED will be disallowed. Modems have "GSM" rat scan
      // need to support scanning requests combined with some parameters.
      ASSERT_TRUE(
          CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                           {RadioError::NONE, RadioError::OPERATION_NOT_ALLOWED,
                            RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid specifier.
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidArgument) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {.type = ScanType::ONE_SHOT,
                                                                    .interval = 60};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidArgument, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
      ASSERT_TRUE(CheckAnyOfErrors(
          radioRsp_v1_4->rspInfo.error,
          {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid interval (lower boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidInterval1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
                                      .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
                                      .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 4,
            .specifiers = {specifier},
            .maxSearchTime = 60,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidInterval1, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
      ASSERT_TRUE(CheckAnyOfErrors(
          radioRsp_v1_4->rspInfo.error,
          {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid interval (upper boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidInterval2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
                                      .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
                                      .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 301,
            .specifiers = {specifier},
            .maxSearchTime = 60,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidInterval2, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
      ASSERT_TRUE(CheckAnyOfErrors(
          radioRsp_v1_4->rspInfo.error,
          {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid max search time (lower boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidMaxSearchTime1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
                                      .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
                                      .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 59,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidMaxSearchTime1, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
      ASSERT_TRUE(CheckAnyOfErrors(
          radioRsp_v1_4->rspInfo.error,
          {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid max search time (upper boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidMaxSearchTime2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
                                      .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
                                      .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 3601,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 1};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidMaxSearchTime2, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
      ASSERT_TRUE(CheckAnyOfErrors(
          radioRsp_v1_4->rspInfo.error,
          {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid periodicity (lower boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidPeriodicity1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
                                      .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
                                      .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 600,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 0};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidPeriodicity1, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
      ASSERT_TRUE(CheckAnyOfErrors(
          radioRsp_v1_4->rspInfo.error,
          {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with invalid periodicity (upper boundary).
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_InvalidPeriodicity2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
                                      .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
                                      .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            .maxSearchTime = 600,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 11};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_InvalidPeriodicity2, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::INVALID_ARGUMENTS}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
      ASSERT_TRUE(CheckAnyOfErrors(
          radioRsp_v1_4->rspInfo.error,
          {RadioError::INVALID_ARGUMENTS, RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with valid periodicity
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_GoodRequest1) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
                                      .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
                                      .channels = {1, 2}};

    ::android::hardware::radio::V1_2::NetworkScanRequest request = {
            .type = ScanType::ONE_SHOT,
            .interval = 60,
            .specifiers = {specifier},
            // Some vendor may not support max search time of 360s.
            // This issue is tracked in b/112205669.
            .maxSearchTime = 300,
            .incrementalResults = false,
            .incrementalResultsPeriodicity = 10};

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_GoodRequest1, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
      ASSERT_TRUE(
          CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                           {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                            RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.startNetworkScan() with valid periodicity and plmns
 */
TEST_F(RadioHidlTest_v1_4, startNetworkScan_GoodRequest2) {
    serial = GetRandomSerialNumber();

    RadioAccessSpecifier specifier = {.radioAccessNetwork = RadioAccessNetworks::GERAN,
                                      .geranBands = {GeranBands::BAND_450, GeranBands::BAND_480},
                                      .channels = {1, 2}};

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

    Return<void> res = radio_v1_4->startNetworkScan_1_4(serial, request);

    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("startNetworkScan_GoodRequest2, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());
    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::NONE, RadioError::SIM_ABSENT}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
      ASSERT_TRUE(
          CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                           {RadioError::NONE, RadioError::INVALID_ARGUMENTS,
                            RadioError::REQUEST_NOT_SUPPORTED}));
    }
}

/*
 * Test IRadio.getSignalStrength_1_4() for the response returned.
 */
TEST_F(RadioHidlTest_v1_4, getSignalStrength_1_4) {
    serial = GetRandomSerialNumber();

    radio_v1_4->getSignalStrength_1_4(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        EXPECT_EQ(RadioError::NONE, radioRsp_v1_4->rspInfo.error);
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE}));
    }
}

/*
 * Test IRadio.setupDataCall_1_4() for the response returned.
 */
TEST_F(RadioHidlTest_v1_4, setupDataCall_1_4) {
    serial = GetRandomSerialNumber();

    ::android::hardware::radio::V1_4::AccessNetwork accessNetwork =
            ::android::hardware::radio::V1_4::AccessNetwork::EUTRAN;

    android::hardware::radio::V1_4::DataProfileInfo dataProfileInfo;
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
    dataProfileInfo.mtu = 0;
    dataProfileInfo.preferred = true;
    dataProfileInfo.persistent = false;

    bool roamingAllowed = false;

    ::android::hardware::radio::V1_2::DataRequestReason reason =
            ::android::hardware::radio::V1_2::DataRequestReason::NORMAL;
    std::vector<hidl_string> addresses = {""};
    std::vector<hidl_string> dnses = {""};

    Return<void> res = radio_v1_4->setupDataCall_1_4(serial, accessNetwork, dataProfileInfo,
                                                     roamingAllowed, reason, addresses, dnses);
    ASSERT_OK(res);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE,
                                      RadioError::OP_NOT_ALLOWED_BEFORE_REG_TO_NW}));
    }
}

/*
 * Test IRadio.getAllowedCarriers_1_4() for the response returned.
 */
TEST_F(RadioHidlTest_v1_4, getAllowedCarriers_1_4) {
    serial = GetRandomSerialNumber();

    radio_v1_4->getAllowedCarriers_1_4(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));
}

/**
 * Test IRadio.setAllowedCarriers_1_4() for the response returned.
 */
TEST_F(RadioHidlTest_v1_4, setAllowedCarriers_1_4) {
    serial = GetRandomSerialNumber();
    CarrierRestrictionsWithPriority carrierRestrictions;
    memset(&carrierRestrictions, 0, sizeof(carrierRestrictions));
    carrierRestrictions.allowedCarriers.resize(1);
    carrierRestrictions.excludedCarriers.resize(0);
    carrierRestrictions.allowedCarriers[0].mcc = hidl_string("123");
    carrierRestrictions.allowedCarriers[0].mnc = hidl_string("456");
    carrierRestrictions.allowedCarriers[0].matchType = CarrierMatchType::ALL;
    carrierRestrictions.allowedCarriers[0].matchData = hidl_string();
    carrierRestrictions.allowedCarriersPrioritized = true;
    SimLockMultiSimPolicy multisimPolicy = SimLockMultiSimPolicy::NO_MULTISIM_POLICY;

    radio_v1_4->setAllowedCarriers_1_4(serial, carrierRestrictions, multisimPolicy);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));

    if (radioRsp_v1_4->rspInfo.error == RadioError::NONE) {
        /* Verify the update of the SIM status. This might need some time */
        if (cardStatus.base.base.cardState != CardState::ABSENT) {
            updateSimCardStatus();
            auto startTime = std::chrono::system_clock::now();
            while (cardStatus.base.base.cardState != CardState::RESTRICTED &&
                   std::chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now() -
                                                               startTime)
                                   .count() < 10) {
                /* Set 2 seconds as interval to check card status */
                sleep(2);
                updateSimCardStatus();
            }
            EXPECT_EQ(CardState::RESTRICTED, cardStatus.base.base.cardState);
        }

        /* Verify that configuration was set correctly, retrieving it from the modem */
        serial = GetRandomSerialNumber();

        radio_v1_4->getAllowedCarriers_1_4(serial);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);
        EXPECT_EQ(RadioError::NONE, radioRsp_v1_4->rspInfo.error);

        EXPECT_EQ(1, radioRsp_v1_4->carrierRestrictionsResp.allowedCarriers.size());
        EXPECT_EQ(0, radioRsp_v1_4->carrierRestrictionsResp.excludedCarriers.size());
        ASSERT_TRUE(hidl_string("123") ==
                    radioRsp_v1_4->carrierRestrictionsResp.allowedCarriers[0].mcc);
        ASSERT_TRUE(hidl_string("456") ==
                    radioRsp_v1_4->carrierRestrictionsResp.allowedCarriers[0].mnc);
        EXPECT_EQ(CarrierMatchType::ALL,
                  radioRsp_v1_4->carrierRestrictionsResp.allowedCarriers[0].matchType);
        ASSERT_TRUE(radioRsp_v1_4->carrierRestrictionsResp.allowedCarriersPrioritized);
        EXPECT_EQ(SimLockMultiSimPolicy::NO_MULTISIM_POLICY, radioRsp_v1_4->multiSimPolicyResp);

        sleep(10);

        /**
         * Another test case of the API to cover to allow carrier.
         * If the API is supported, this is also used to reset to no carrier restriction
         * status for cardStatus.
         */
        memset(&carrierRestrictions, 0, sizeof(carrierRestrictions));
        carrierRestrictions.allowedCarriers.resize(0);
        carrierRestrictions.excludedCarriers.resize(0);
        carrierRestrictions.allowedCarriersPrioritized = false;

        serial = GetRandomSerialNumber();
        radio_v1_4->setAllowedCarriers_1_4(serial, carrierRestrictions, multisimPolicy);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
        EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

        EXPECT_EQ(RadioError::NONE, radioRsp_v1_4->rspInfo.error);

        if (cardStatus.base.base.cardState != CardState::ABSENT) {
            /* Resetting back to no carrier restriction needs some time */
            updateSimCardStatus();
            auto startTime = std::chrono::system_clock::now();
            while (cardStatus.base.base.cardState == CardState::RESTRICTED &&
                   std::chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now() -
                                                               startTime)
                                   .count() < 10) {
                /* Set 2 seconds as interval to check card status */
                sleep(2);
                updateSimCardStatus();
            }
            EXPECT_NE(CardState::RESTRICTED, cardStatus.base.base.cardState);
            sleep(10);
        }
    }
}

TEST_F(RadioHidlTest_v1_4, setDataProfile_1_4) {
    serial = GetRandomSerialNumber();

    // Create a dataProfileInfo
    android::hardware::radio::V1_4::DataProfileInfo dataProfileInfo;
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
    dataProfileInfo.mtu = 0;
    dataProfileInfo.preferred = true;
    dataProfileInfo.persistent = true;

    // Create a dataProfileInfoList
    android::hardware::hidl_vec<android::hardware::radio::V1_4::DataProfileInfo>
            dataProfileInfoList = {dataProfileInfo};

    radio_v1_4->setDataProfile_1_4(serial, dataProfileInfoList);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE}));
    }
}

TEST_F(RadioHidlTest_v1_4, setInitialAttachApn_1_4) {
    serial = GetRandomSerialNumber();

    // Create a dataProfileInfo
    android::hardware::radio::V1_4::DataProfileInfo dataProfileInfo;
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
    dataProfileInfo.mtu = 0;
    dataProfileInfo.preferred = true;
    dataProfileInfo.persistent = false;

    radio_v1_4->setInitialAttachApn_1_4(serial, dataProfileInfo);

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    if (cardStatus.base.base.cardState == CardState::ABSENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::SIM_ABSENT, RadioError::RADIO_NOT_AVAILABLE}));
    } else if (cardStatus.base.base.cardState == CardState::PRESENT) {
        ASSERT_TRUE(CheckAnyOfErrors(radioRsp_v1_4->rspInfo.error,
                                     {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE}));
    }
}

/*
 * Test IRadio.getDataRegistrationStateResponse_1_4() for the response returned.
 */
TEST_F(RadioHidlTest_v1_4, getDataRegistrationState_1_4) {
    int rat;
    serial = GetRandomSerialNumber();

    Return<void> res = radio_v1_4->getDataRegistrationState(serial);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);

    ALOGI("getDataRegistrationStateResponse_1_4, rspInfo.error = %s\n",
          toString(radioRsp_v1_4->rspInfo.error).c_str());

    ASSERT_TRUE(CheckAnyOfErrors(
        radioRsp_v1_4->rspInfo.error,
        {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::NOT_PROVISIONED}));

    rat = radioRsp_v1_4->dataRegResp.base.rat;
    /*
     *  - Expect Valid vopsinfo when device is on LTE
     *  - Expect empty vopsInfo when device is not on LTE
     */
    if (rat == ((int )::android::hardware::radio::V1_4::RadioTechnology::LTE)
        || (rat == (int )::android::hardware::radio::V1_4::RadioTechnology::LTE_CA)) {

        EXPECT_EQ(::android::hardware::radio::V1_4::DataRegStateResult::VopsInfo::hidl_discriminator
                  ::lteVopsInfo, radioRsp_v1_4->dataRegResp.vopsInfo.getDiscriminator());
    } else {

        EXPECT_EQ(::android::hardware::radio::V1_4::DataRegStateResult::VopsInfo::hidl_discriminator
                  ::noinit, radioRsp_v1_4->dataRegResp.vopsInfo.getDiscriminator());
    }
}
