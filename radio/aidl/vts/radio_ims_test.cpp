/*
 * Copyright (C) 2022 The Android Open Source Project
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
#include <android/binder_manager.h>

#include "radio_ims_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk())

void RadioImsTest::SetUp() {
    RadioServiceTest::SetUp();
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
    regInfo.regState = ImsRegistrationState::NOT_REGISTERED;
    regInfo.accessNetworkType = AccessNetwork::EUTRAN;
    regInfo.suggestedAction = SuggestedAction::NONE;
    regInfo.capabilities = ImsRegistration::IMS_MMTEL_CAPABILITY_NONE;

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
 * Test IRadioIms.startImsTraffic() for the response returned.
 */
TEST_P(RadioImsTest, startImsTraffic) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping startImsTraffic because ims is not supported in device");
        return;
    } else {
        ALOGI("Running startImsTraffic because ims is supported in device");
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res =
            radio_ims->startImsTraffic(serial, 1,
            ImsTrafficType::REGISTRATION, AccessNetwork::EUTRAN, ImsCall::Direction::OUTGOING);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_ims->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_ims->rspInfo.serial);

    ALOGI("startImsTraffic, rspInfo.error = %s\n",
              toString(radioRsp_ims->rspInfo.error).c_str());

    verifyError(radioRsp_ims->rspInfo.error);
}

/*
 * Test IRadioIms.stopImsTraffic() for the response returned.
 */
TEST_P(RadioImsTest, stopImsTraffic) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping stopImsTraffic because ims is not supported in device");
        return;
    } else {
        ALOGI("Running stopImsTraffic because ims is supported in device");
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = radio_ims->stopImsTraffic(serial, 2);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_ims->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_ims->rspInfo.serial);

    ALOGI("stopImsTraffic, rspInfo.error = %s\n",
              toString(radioRsp_ims->rspInfo.error).c_str());

    verifyError(radioRsp_ims->rspInfo.error);
}

/*
 * Test IRadioIms.triggerEpsFallback() for the response returned.
 */
TEST_P(RadioImsTest, triggerEpsFallback) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping triggerEpsFallback because ims is not supported in device");
        return;
    } else {
        ALOGI("Running triggerEpsFallback because ims is supported in device");
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res =
            radio_ims->triggerEpsFallback(serial, EpsFallbackReason::NO_NETWORK_TRIGGER);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_ims->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_ims->rspInfo.serial);

    ALOGI("triggerEpsFallback, rspInfo.error = %s\n",
              toString(radioRsp_ims->rspInfo.error).c_str());

    verifyError(radioRsp_ims->rspInfo.error);
}

/*
 * Test IRadioIms.sendAnbrQuery() for the response returned.
 */
TEST_P(RadioImsTest, sendAnbrQuery) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping sendAnbrQuery because ims is not supported in device");
        return;
    } else {
        ALOGI("Running sendAnbrQuery because ims is supported in device");
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res =
            radio_ims->sendAnbrQuery(serial, ImsStreamType::AUDIO, ImsStreamDirection::UPLINK, 13200);
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_ims->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_ims->rspInfo.serial);

    ALOGI("sendAnbrQuery, rspInfo.error = %s\n",
              toString(radioRsp_ims->rspInfo.error).c_str());

    verifyError(radioRsp_ims->rspInfo.error);
}

/*
 * Test IRadioIms.updateImsCallStatus() for the response returned.
 */
TEST_P(RadioImsTest, updateImsCallStatus) {
    if (!deviceSupportsFeature(FEATURE_TELEPHONY_IMS)) {
        ALOGI("Skipping updateImsCallStatus because ims is not supported in device");
        return;
    } else {
        ALOGI("Running updateImsCallStatus because ims is supported in device");
    }

    serial = GetRandomSerialNumber();

    ImsCall imsCall;

    ndk::ScopedAStatus res =
            radio_ims->updateImsCallStatus(serial, { imsCall });
    ASSERT_OK(res);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_ims->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_ims->rspInfo.serial);

    ALOGI("updateImsCallStatus, rspInfo.error = %s\n",
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
        case RadioError::NO_RESOURCES:
            SUCCEED();
            break;
        default:
            FAIL();
            break;
    }
}
