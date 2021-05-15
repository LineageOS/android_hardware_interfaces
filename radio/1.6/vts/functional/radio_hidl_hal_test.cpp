/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_6.h>

bool isServiceValidForDeviceConfiguration(hidl_string& serviceName) {
    if (isSsSsEnabled()) {
        // Device is configured as SSSS.
        if (serviceName != RADIO_SERVICE_SLOT1_NAME) {
            ALOGI("%s instance is not valid for SSSS device.", serviceName.c_str());
            return false;
        }
    } else if (isDsDsEnabled()) {
        // Device is configured as DSDS.
        if (serviceName != RADIO_SERVICE_SLOT1_NAME && serviceName != RADIO_SERVICE_SLOT2_NAME) {
            ALOGI("%s instance is not valid for DSDS device.", serviceName.c_str());
            return false;
        }
    } else if (isTsTsEnabled()) {
        // Device is configured as TSTS.
        if (serviceName != RADIO_SERVICE_SLOT1_NAME && serviceName != RADIO_SERVICE_SLOT2_NAME &&
            serviceName != RADIO_SERVICE_SLOT3_NAME) {
            ALOGI("%s instance is not valid for TSTS device.", serviceName.c_str());
            return false;
        }
    }
    return true;
}

void RadioHidlTest_v1_6::SetUp() {
    hidl_string serviceName = GetParam();

    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }

    radio_v1_6 = android::hardware::radio::V1_6::IRadio::getService(serviceName);
    ASSERT_NE(nullptr, radio_v1_6.get());

    radioRsp_v1_6 = new (std::nothrow) RadioResponse_v1_6(*this);
    ASSERT_NE(nullptr, radioRsp_v1_6.get());

    count_ = 0;

    radioInd_v1_6 = new (std::nothrow) RadioIndication_v1_6(*this);
    ASSERT_NE(nullptr, radioInd_v1_6.get());

    radio_v1_6->setResponseFunctions(radioRsp_v1_6, radioInd_v1_6);

    updateSimCardStatus();
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_6->rspInfo_v1_0.type);
    EXPECT_EQ(serial, radioRsp_v1_6->rspInfo_v1_0.serial);
    EXPECT_EQ(::android::hardware::radio::V1_0::RadioError::NONE,
              radioRsp_v1_6->rspInfo_v1_0.error);

    sp<::android::hardware::radio::config::V1_1::IRadioConfig> radioConfig =
            ::android::hardware::radio::config::V1_1::IRadioConfig::getService();
    /* Enforce Vts testing with RadioConfig is existed. */
    ASSERT_NE(nullptr, radioConfig.get());

    /* Enforce Vts Testing with Sim Status Present only. */
    EXPECT_EQ(CardState::PRESENT, cardStatus.base.base.base.cardState);
}

void RadioHidlTest_v1_6::clearPotentialEstablishedCalls() {
    // Get the current call Id to hangup the established emergency call.
    serial = GetRandomSerialNumber();
    radio_v1_6->getCurrentCalls_1_6(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());

    // Hang up to disconnect the established call channels.
    for (const ::android::hardware::radio::V1_6::Call& call : radioRsp_v1_6->currentCalls) {
        serial = GetRandomSerialNumber();
        radio_v1_6->hangup(serial, call.base.base.index);
        ALOGI("Hang up to disconnect the established call channel: %d", call.base.base.index);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        // Give some time for modem to disconnect the established call channel.
        sleep(MODEM_EMERGENCY_CALL_DISCONNECT_TIME);
    }

    // Verify there are no more current calls.
    serial = GetRandomSerialNumber();
    radio_v1_6->getCurrentCalls_1_6(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(0, radioRsp_v1_6->currentCalls.size());
}

void RadioHidlTest_v1_6::updateSimCardStatus() {
    serial = GetRandomSerialNumber();
    radio_v1_6->getIccCardStatus(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}

void RadioHidlTest_v1_6::getDataCallList() {
    serial = GetRandomSerialNumber();
    radio_v1_6->getDataCallList_1_6(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}

/**
 * Specific features on the Radio Hal rely on Radio Hal Capabilities.  The VTS
 * tests related to that features must not run if the related capability is
 * disabled.
 * <p/>
 * Typical usage within VTS:
 * if (getRadioHalCapabilities()) return;
 */
bool RadioHidlTest_v1_6::getRadioHalCapabilities() {
    sp<::android::hardware::radio::config::V1_3::IRadioConfig> radioConfig_v1_3 =
            ::android::hardware::radio::config::V1_3::IRadioConfig::getService();
    if (radioConfig_v1_3.get() == nullptr) {
        // If v1_3 isn't present, the values are initialized to false
        return false;
    } else {
        // Get radioHalDeviceCapabilities from the radio config
        sp<RadioConfigResponse> radioConfigRsp = new (std::nothrow) RadioConfigResponse(*this);
        radioConfig_v1_3->setResponseFunctions(radioConfigRsp, nullptr);
        serial = GetRandomSerialNumber();

        radioConfig_v1_3->getHalDeviceCapabilities(serial);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        return radioConfigRsp->modemReducedFeatureSet1;
    }
}
