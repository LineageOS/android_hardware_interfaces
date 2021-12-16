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

#include "radio_ims_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioImsTest::SetUp() {
    std::string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_ims = IRadioIms::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(nullptr, radio_ims.get());

    radioRsp_ims = ndk::SharedRefBase::make<RadioImsResponse>(*this);
    ASSERT_NE(nullptr, radioRsp_ims.get());

    count_ = 0;

    radioInd_ims = ndk::SharedRefBase::make<RadioImsIndication>(*this);
    ASSERT_NE(nullptr, radioInd_ims.get());

    radio_ims->setResponseFunctions(radioRsp_ims, radioInd_ims);

    // Assert IRadioConfig exists before testing
    radio_config = config::IRadioConfig::fromBinder(ndk::SpAIBinder(
            AServiceManager_waitForService("android.hardware.radio.config.IRadioConfig/default")));
    ASSERT_NE(nullptr, radio_config.get());
}

/*
 * Test IRadioIms.setSrvccCallInfo() for the response returned.
 */
TEST_P(RadioImsTest, setSrvccCallInfo) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping setSrvccCallInfo because ims is not supported in device");
        return;
    } else {
        ALOGI("Running setSrvccCallInfo because ims is supported in device");
    }

    serial = GetRandomSerialNumber();

    SrvccCall srvccCall;

    ndk::ScopedAStatus res =
            radio_ims->setSrvccCallInfo(serial, { srvccCall });
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_ims->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_ims->rspInfo.serial);

    ALOGI("setSrvccCallInfo, rspInfo.error = %s\n",
              toString(radioRsp_ims->rspInfo.error).c_str());

    verifyError(radioRsp_ims->rspInfo.error);
}

/*
 * Test IRadioIms.updateImsRegistrationInfo() for the response returned.
 */
TEST_P(RadioImsTest, updateImsRegistrationInfo) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping updateImsRegistrationInfo because ims is not supported in device");
        return;
    } else {
        ALOGI("Running updateImsRegistrationInfo because ims is supported in device");
    }

    serial = GetRandomSerialNumber();

    ImsRegistration regInfo;
    regInfo.state = ImsRegistration::State::NOT_REGISTERED;
    regInfo.ipcan = ImsRegistration::ImsAccessNetwork::NONE;
    regInfo.reason = ImsRegistration::FailureReason::NONE;
    regInfo.features = ImsRegistration::FEATURE_NONE;

    ndk::ScopedAStatus res =
            radio_ims->updateImsRegistrationInfo(serial, regInfo);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_ims->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_ims->rspInfo.serial);

    ALOGI("updateImsRegistrationInfo, rspInfo.error = %s\n",
              toString(radioRsp_ims->rspInfo.error).c_str());

    verifyError(radioRsp_ims->rspInfo.error);
}

/*
 * Test IRadioIms.notifyImsTraffic() for the response returned.
 */
TEST_P(RadioImsTest, notifyImsTraffic) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping notifyImsTraffic because ims is not supported in device");
        return;
    } else {
        ALOGI("Running notifyImsTraffic because ims is supported in device");
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res =
            radio_ims->notifyImsTraffic(serial, 1, ImsTrafficType::REGISTRATION, false);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_ims->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_ims->rspInfo.serial);

    ALOGI("notifyImsTraffic, rspInfo.error = %s\n",
              toString(radioRsp_ims->rspInfo.error).c_str());

    verifyError(radioRsp_ims->rspInfo.error);
}

/*
 * Test IRadioIms.performAcbCheck() for the response returned.
 */
TEST_P(RadioImsTest, performAcbCheck) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping performAcbCheck because ims is not supported in device");
        return;
    } else {
        ALOGI("Running performAcbCheck because ims is supported in device");
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res =
            radio_ims->performAcbCheck(serial, 1, ImsTrafficType::REGISTRATION);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_ims->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_ims->rspInfo.serial);

    ALOGI("performAcbCheck, rspInfo.error = %s\n",
              toString(radioRsp_ims->rspInfo.error).c_str());

    verifyError(radioRsp_ims->rspInfo.error);
}

void RadioImsTest::verifyError(RadioError resp) {
    switch (resp) {
        case RadioError::NONE:
        case RadioError::RADIO_NOT_AVAILABLE:
        case RadioError::INVALID_STATE:
        case RadioError::NO_MEMORY:
        case RadioError::SYSTEM_ERR:
        case RadioError::MODEM_ERR:
        case RadioError::INTERNAL_ERR:
        case RadioError::INVALID_ARGUMENTS:
        case RadioError::REQUEST_NOT_SUPPORTED:
        case RadioError::NO_RESOURCES:
            SUCCEED();
            break;
        default:
            FAIL();
            break;
    }
}
