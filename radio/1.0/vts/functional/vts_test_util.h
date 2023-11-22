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

#pragma once

#include <android-base/logging.h>

#include <android/hardware/radio/1.0/types.h>
#include <android/log.h>
#include <gtest/gtest.h>

using ::android::hardware::radio::V1_0::RadioError;
using ::android::hardware::radio::V1_0::RegState;
using ::android::hardware::radio::V1_0::SapResultCode;
using namespace std;

/*
 * MACRO used to skip test case when radio response return error REQUEST_NOT_SUPPORTED
 * on HAL versions which has deprecated the request interfaces. The MACRO can only be used
 * AFTER receiving radio response.
 */
#define SKIP_TEST_IF_REQUEST_NOT_SUPPORTED_WITH_HAL(__ver__, __radio__, __radioRsp__)      \
    do {                                                                                   \
        sp<::android::hardware::radio::V##__ver__::IRadio> __radio =                       \
                ::android::hardware::radio::V##__ver__::IRadio::castFrom(__radio__);       \
        if (__radio && __radioRsp__->rspInfo.error == RadioError::REQUEST_NOT_SUPPORTED) { \
            GTEST_SKIP() << "REQUEST_NOT_SUPPORTED";                                       \
        }                                                                                  \
    } while (0)

enum CheckFlag {
    CHECK_DEFAULT = 0,
    CHECK_GENERAL_ERROR = 1,
    CHECK_OEM_ERROR = 2,
    CHECK_OEM_AND_GENERAL_ERROR = 3,
    CHECK_SAP_ERROR = 4,
};

static constexpr const char* FEATURE_VOICE_CALL = "android.software.connectionservice";

static constexpr const char* FEATURE_TELEPHONY = "android.hardware.telephony";

static constexpr const char* FEATURE_TELEPHONY_GSM = "android.hardware.telephony.gsm";

static constexpr const char* FEATURE_TELEPHONY_CDMA = "android.hardware.telephony.cdma";

/*
 * Generate random serial number for radio test
 */
int GetRandomSerialNumber();

/*
 * Check multiple radio error codes which are possibly returned because of the different
 * vendor/devices implementations. It allows optional checks for general errors or/and oem errors.
 */
::testing::AssertionResult CheckAnyOfErrors(RadioError err, std::vector<RadioError> generalError,
                                            CheckFlag flag = CHECK_DEFAULT);
/*
 * Check multiple sap error codes which are possibly returned because of the different
 * vendor/devices implementations.
 */
::testing::AssertionResult CheckAnyOfErrors(SapResultCode err, std::vector<SapResultCode> errors);

/*
 * Check if device supports feature.
 */
bool deviceSupportsFeature(const char* feature);

/*
 * Check if device is in SsSs (Single SIM Single Standby).
 */
bool isSsSsEnabled();

/*
 * Check if device is in DSDS (Dual SIM Dual Standby).
 */
bool isDsDsEnabled();

/*
 * Check if device is in TSTS (Triple SIM Triple Standby).
 */
bool isTsTsEnabled();

/*
 * Check if voice status is in emergency only.
 */
bool isVoiceEmergencyOnly(RegState state);

/*
 * Check if voice status is in service.
 */
bool isVoiceInService(RegState state);

/*
 * Check if device is in Lte Connected status.
 */
bool isLteConnected();

/**
 * Used when waiting for an asynchronous response from the HAL.
 */
class RadioResponseWaiter {
  protected:
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_;

  public:
    /* Serial number for radio request */
    int serial;

    /* Used as a mechanism to inform the test about data/event callback */
    void notify(int receivedSerial);

    /* Test code calls this function to wait for response */
    std::cv_status wait();
};
