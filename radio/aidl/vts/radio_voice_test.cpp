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

#include "radio_voice_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioVoiceTest::SetUp() {
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_voice = IRadioVoice::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_voice.get());

    radioRsp_voice = ndk::SharedRefBase::make<RadioVoiceResponse>(*this);
    ASSERT_NE(nullptr, radioRsp_voice.get());

    count_ = 0;

    radioInd_voice = ndk::SharedRefBase::make<RadioVoiceIndication>(*this);
    ASSERT_NE(nullptr, radioInd_voice.get());

    radio_voice->setResponseFunctions(radioRsp_voice, radioInd_voice);

    // Assert IRadioConfig exists before testing
    std::shared_ptr<aidl::android::hardware::radio::config::IRadioConfig> radioConfig =
            aidl::android::hardware::radio::config::IRadioConfig::fromBinder(
                    ndk::SpAIBinder(AServiceManager_waitForService(
                            "android.hardware.radio.config.IRadioConfig/default")));
    ASSERT_NE(nullptr, radioConfig.get());
}

ndk::ScopedAStatus RadioVoiceTest::clearPotentialEstablishedCalls() {
    // Get the current call Id to hangup the established emergency call.
    serial = GetRandomSerialNumber();
    radio_voice->getCurrentCalls(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());

    // Hang up to disconnect the established call channels.
    for (const Call& call : radioRsp_voice->currentCalls) {
        serial = GetRandomSerialNumber();
        radio_voice->hangup(serial, call.index);
        ALOGI("Hang up to disconnect the established call channel: %d", call.index);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        // Give some time for modem to disconnect the established call channel.
        sleep(MODEM_EMERGENCY_CALL_DISCONNECT_TIME);
    }

    // Verify there are no more current calls.
    serial = GetRandomSerialNumber();
    radio_voice->getCurrentCalls(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(0, radioRsp_voice->currentCalls.size());
    return ndk::ScopedAStatus::ok();
}

/*
 * Test IRadioVoice.emergencyDial() for the response returned.
 */
TEST_P(RadioVoiceTest, emergencyDial) {
    if (!deviceSupportsFeature(FEATURE_VOICE_CALL)) {
        ALOGI("Skipping emergencyDial because voice call is not supported in device");
        return;
    } else if (!deviceSupportsFeature(FEATURE_TELEPHONY_GSM) &&
               !deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
        ALOGI("Skipping emergencyDial because gsm/cdma radio is not supported in device");
        return;
    } else {
        ALOGI("Running emergencyDial because voice call is supported in device");
    }

    serial = GetRandomSerialNumber();

    Dial dialInfo;
    dialInfo.address = std::string("911");
    EmergencyServiceCategory categories = EmergencyServiceCategory::UNSPECIFIED;
    std::vector<std::string> urns = {""};
    EmergencyCallRouting routing = EmergencyCallRouting::UNKNOWN;

    ndk::ScopedAStatus res =
            radio_voice->emergencyDial(serial, dialInfo, categories, urns, routing, true, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_voice->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_voice->rspInfo.serial);

    ALOGI("emergencyDial, rspInfo.error = %s\n", toString(radioRsp_voice->rspInfo.error).c_str());

    RadioError rspEmergencyDial = radioRsp_voice->rspInfo.error;
    // In DSDS or TSTS, we only check the result if the current slot is IN_SERVICE
    // or Emergency_Only.
    if (isDsDsEnabled() || isTsTsEnabled()) {
        // TODO(b/210712359): maybe create a local RadioNetwork instance
        /**
        serial = GetRandomSerialNumber();
        radio_v1_6->getVoiceRegistrationState(serial);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        if (isVoiceEmergencyOnly(radioRsp_v1_6->voiceRegResp.regState) ||
            isVoiceInService(radioRsp_v1_6->voiceRegResp.regState)) {
            EXPECT_EQ(RadioError::NONE, rspEmergencyDial);
        }
        **/
    } else {
        EXPECT_EQ(RadioError::NONE, rspEmergencyDial);
    }

    // Give some time for modem to establish the emergency call channel.
    sleep(MODEM_EMERGENCY_CALL_ESTABLISH_TIME);

    // Disconnect all the potential established calls to prevent them affecting other tests.
    clearPotentialEstablishedCalls();
}

/*
 * Test IRadioVoice.emergencyDial() with specified service and its response returned.
 */
TEST_P(RadioVoiceTest, emergencyDial_withServices) {
    if (!deviceSupportsFeature(FEATURE_VOICE_CALL)) {
        ALOGI("Skipping emergencyDial because voice call is not supported in device");
        return;
    } else if (!deviceSupportsFeature(FEATURE_TELEPHONY_GSM) &&
               !deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
        ALOGI("Skipping emergencyDial because gsm/cdma radio is not supported in device");
        return;
    } else {
        ALOGI("Running emergencyDial because voice call is supported in device");
    }

    serial = GetRandomSerialNumber();

    Dial dialInfo;
    dialInfo.address = std::string("911");
    EmergencyServiceCategory categories = EmergencyServiceCategory::AMBULANCE;
    std::vector<std::string> urns = {"urn:service:sos.ambulance"};
    EmergencyCallRouting routing = EmergencyCallRouting::UNKNOWN;

    ndk::ScopedAStatus res =
            radio_voice->emergencyDial(serial, dialInfo, categories, urns, routing, true, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_voice->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_voice->rspInfo.serial);

    ALOGI("emergencyDial_withServices, rspInfo.error = %s\n",
          toString(radioRsp_voice->rspInfo.error).c_str());
    RadioError rspEmergencyDial = radioRsp_voice->rspInfo.error;

    // In DSDS or TSTS, we only check the result if the current slot is IN_SERVICE
    // or Emergency_Only.
    if (isDsDsEnabled() || isTsTsEnabled()) {
        // TODO(b/210712359): maybe create a local RadioNetwork instance
        /**
        serial = GetRandomSerialNumber();
        radio_v1_6->getVoiceRegistrationState_1_6(serial);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        if (isVoiceEmergencyOnly(radioRsp_v1_6->voiceRegResp.regState) ||
            isVoiceInService(radioRsp_v1_6->voiceRegResp.regState)) {
            EXPECT_EQ(RadioError::NONE, rspEmergencyDial);
        }
        **/
    } else {
        EXPECT_EQ(RadioError::NONE, rspEmergencyDial);
    }
    // Give some time for modem to establish the emergency call channel.
    sleep(MODEM_EMERGENCY_CALL_ESTABLISH_TIME);

    // Disconnect all the potential established calls to prevent them affecting other tests.
    clearPotentialEstablishedCalls();
}

/*
 * Test IRadioVoice.emergencyDial() with known emergency call routing and its response returned.
 */
TEST_P(RadioVoiceTest, emergencyDial_withEmergencyRouting) {
    if (!deviceSupportsFeature(FEATURE_VOICE_CALL)) {
        ALOGI("Skipping emergencyDial because voice call is not supported in device");
        return;
    } else if (!deviceSupportsFeature(FEATURE_TELEPHONY_GSM) &&
               !deviceSupportsFeature(FEATURE_TELEPHONY_CDMA)) {
        ALOGI("Skipping emergencyDial because gsm/cdma radio is not supported in device");
        return;
    } else {
        ALOGI("Running emergencyDial because voice call is supported in device");
    }

    serial = GetRandomSerialNumber();

    Dial dialInfo;
    dialInfo.address = std::string("911");
    EmergencyServiceCategory categories = EmergencyServiceCategory::UNSPECIFIED;
    std::vector<std::string> urns = {""};
    EmergencyCallRouting routing = EmergencyCallRouting::EMERGENCY;

    ndk::ScopedAStatus res =
            radio_voice->emergencyDial(serial, dialInfo, categories, urns, routing, true, true);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_voice->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_voice->rspInfo.serial);

    ALOGI("emergencyDial_withEmergencyRouting, rspInfo.error = %s\n",
          toString(radioRsp_voice->rspInfo.error).c_str());
    RadioError rspEmergencyDial = radioRsp_voice->rspInfo.error;

    // In DSDS or TSTS, we only check the result if the current slot is IN_SERVICE
    // or Emergency_Only.
    if (isDsDsEnabled() || isTsTsEnabled()) {
        // TODO(b/210712359): maybe create a local RadioNetwork instance
        /**
        serial = GetRandomSerialNumber();
        radio_v1_6->getVoiceRegistrationState_1_6(serial);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        if (isVoiceEmergencyOnly(radioRsp_v1_6->voiceRegResp.regState) ||
            isVoiceInService(radioRsp_v1_6->voiceRegResp.regState)) {
            EXPECT_EQ(RadioError::NONE, rspEmergencyDial);
        }
        **/
    } else {
        EXPECT_EQ(RadioError::NONE, rspEmergencyDial);
    }

    // Give some time for modem to establish the emergency call channel.
    sleep(MODEM_EMERGENCY_CALL_ESTABLISH_TIME);

    // Disconnect all the potential established calls to prevent them affecting other tests.
    clearPotentialEstablishedCalls();
}

/*
 * Test IRadioVoice.getCurrentCalls() for the response returned.
 */
TEST_P(RadioVoiceTest, getCurrentCalls) {
    serial = GetRandomSerialNumber();
    radio_voice->getCurrentCalls(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_voice->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_voice->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_voice->rspInfo.error);
}
