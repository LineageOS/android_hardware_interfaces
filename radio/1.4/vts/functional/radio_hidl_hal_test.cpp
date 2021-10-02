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

void RadioHidlTest_v1_4::SetUp() {
    hidl_string serviceName = GetParam();
    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }
    radio_v1_4 = ::android::hardware::radio::V1_4::IRadio::getService(serviceName);

    if (radio_v1_4 == NULL) {
        sleep(60);
        radio_v1_4 = ::android::hardware::radio::V1_4::IRadio::getService(serviceName);
    }
    ASSERT_NE(nullptr, radio_v1_4.get());

    radioRsp_v1_4 = new (std::nothrow) RadioResponse_v1_4(*this);
    ASSERT_NE(nullptr, radioRsp_v1_4.get());

    count_ = 0;

    radioInd_v1_4 = new (std::nothrow) RadioIndication_v1_4(*this);
    ASSERT_NE(nullptr, radioInd_v1_4.get());

    radio_v1_4->setResponseFunctions(radioRsp_v1_4, radioInd_v1_4);

    updateSimCardStatus();
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_4->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_4->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_4->rspInfo.error);

    sp<::android::hardware::radio::config::V1_1::IRadioConfig> radioConfig =
            ::android::hardware::radio::config::V1_1::IRadioConfig::getService();

    /* Enforce Vts tesing with RadioConfig is existed. */
    ASSERT_NE(nullptr, radioConfig.get());

    /* Enforce Vts Testing with Sim Status Present only. */
    EXPECT_EQ(CardState::PRESENT, cardStatus.base.base.cardState);
}

/*
 * Notify that the response message is received.
 */
void RadioHidlTest_v1_4::notify(int receivedSerial) {
    std::unique_lock<std::mutex> lock(mtx_);
    if (serial == receivedSerial) {
        count_++;
        cv_.notify_one();
    }
}

/*
 * Wait till the response message is notified or till TIMEOUT_PERIOD.
 */
std::cv_status RadioHidlTest_v1_4::wait() {
    std::unique_lock<std::mutex> lock(mtx_);

    std::cv_status status = std::cv_status::no_timeout;
    auto now = std::chrono::system_clock::now();
    while (count_ == 0) {
        status = cv_.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
        if (status == std::cv_status::timeout) {
            return status;
        }
    }
    count_--;
    return status;
}

void RadioHidlTest_v1_4::clearPotentialEstablishedCalls() {
    // Get the current call Id to hangup the established emergency call.
    serial = GetRandomSerialNumber();
    radio_v1_4->getCurrentCalls(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());

    // Hang up to disconnect the established call channels.
    for (const ::android::hardware::radio::V1_2::Call& call : radioRsp_v1_4->currentCalls) {
        serial = GetRandomSerialNumber();
        radio_v1_4->hangup(serial, call.base.index);
        ALOGI("Hang up to disconnect the established call channel: %d", call.base.index);
        EXPECT_EQ(std::cv_status::no_timeout, wait());
        // Give some time for modem to disconnect the established call channel.
        sleep(MODEM_EMERGENCY_CALL_DISCONNECT_TIME);
    }

    // Verify there are no more current calls.
    serial = GetRandomSerialNumber();
    radio_v1_4->getCurrentCalls(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(0, radioRsp_v1_4->currentCalls.size());
}

void RadioHidlTest_v1_4::updateSimCardStatus() {
    serial = GetRandomSerialNumber();
    radio_v1_4->getIccCardStatus(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}

void RadioHidlTest_v1_4::stopNetworkScan() {
    serial = GetRandomSerialNumber();
    radio_v1_4->stopNetworkScan(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}
