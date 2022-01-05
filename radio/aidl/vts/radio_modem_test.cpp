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

#include "radio_modem_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioModemTest::SetUp() {
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_modem = IRadioModem::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_modem.get());

    radioRsp_modem = ndk::SharedRefBase::make<RadioModemResponse>(*this);
    ASSERT_NE(nullptr, radioRsp_modem.get());

    count_ = 0;

    radioInd_modem = ndk::SharedRefBase::make<RadioModemIndication>(*this);
    ASSERT_NE(nullptr, radioInd_modem.get());

    radio_modem->setResponseFunctions(radioRsp_modem, radioInd_modem);

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

/*
 * Test IRadioModem.setRadioPower() for the response returned.
 */
TEST_P(RadioModemTest, setRadioPower_emergencyCall_cancelled) {
    // Set radio power to off.
    serial = GetRandomSerialNumber();
    radio_modem->setRadioPower(serial, false, false, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_modem->rspInfo.error);

    // Set radio power to on with forEmergencyCall being true. This should put modem to only scan
    // emergency call bands.
    serial = GetRandomSerialNumber();
    radio_modem->setRadioPower(serial, true, true, true);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_modem->rspInfo.error);

    // Set radio power to on with forEmergencyCall being false. This should put modem in regular
    // operation modem.
    serial = GetRandomSerialNumber();
    radio_modem->setRadioPower(serial, true, false, false);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_modem->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_modem->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_modem->rspInfo.error);
}
