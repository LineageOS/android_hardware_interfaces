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

#pragma once

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/radio/RadioError.h>
#include <aidl/android/hardware/radio/config/IRadioConfig.h>
#include <aidl/android/hardware/radio/config/SimSlotStatus.h>
#include <aidl/android/hardware/radio/network/RegState.h>
#include <aidl/android/hardware/radio/sim/CardStatus.h>
#include <aidl/android/hardware/radio/sim/IRadioSim.h>
#include <utils/Log.h>
#include <vector>

using namespace aidl::android::hardware::radio;
using aidl::android::hardware::radio::config::SimSlotStatus;
using aidl::android::hardware::radio::network::RegState;
using aidl::android::hardware::radio::sim::CardStatus;

extern CardStatus cardStatus;
extern SimSlotStatus slotStatus;
extern int serial;
extern int count_;

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

#define MODEM_EMERGENCY_CALL_ESTABLISH_TIME 3
#define MODEM_EMERGENCY_CALL_DISCONNECT_TIME 3
#define MODEM_SET_SIM_POWER_DELAY_IN_SECONDS 2
#define MODEM_SET_SIM_SLOT_MAPPING_DELAY_IN_SECONDS 6

#define RADIO_SERVICE_SLOT1_NAME "slot1"  // HAL instance name for SIM slot 1 or single SIM device
#define RADIO_SERVICE_SLOT2_NAME "slot2"  // HAL instance name for SIM slot 2 on dual SIM device
#define RADIO_SERVICE_SLOT3_NAME "slot3"  // HAL instance name for SIM slot 3 on triple SIM device

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
 * Check if device is in DSDA (Dual SIM Dual Active).
 */
bool isDsDaEnabled();

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
 * Check if service is valid for device configuration
 */
bool isServiceValidForDeviceConfiguration(std::string& serviceName);

/**
 * RadioServiceTest base class
 */
class RadioServiceTest {
  protected:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<config::IRadioConfig> radio_config;
    std::shared_ptr<sim::IRadioSim> radio_sim;

  public:
    /* Used as a mechanism to inform the test about data/event callback */
    void notify(int receivedSerial);

    /* Test code calls this function to wait for response */
    std::cv_status wait();

    /* Get the radio HAL capabilities */
    bool getRadioHalCapabilities();

    /* Update SIM card status */
    void updateSimCardStatus();

    /* Update SIM slot status */
    void updateSimSlotStatus(int physicalSlotId);
};
