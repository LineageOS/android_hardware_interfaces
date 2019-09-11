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

void RadioHidlTest_v1_2::SetUp() {
    radio_v1_2 =
        ::testing::VtsHalHidlTargetTestBase::getService<::android::hardware::radio::V1_2::IRadio>(
            RadioHidlEnvironment::Instance()
                ->getServiceName<::android::hardware::radio::V1_2::IRadio>(
                    hidl_string(RADIO_SERVICE_NAME)));
    if (radio_v1_2 == NULL) {
        sleep(60);
        radio_v1_2 = ::testing::VtsHalHidlTargetTestBase::getService<
            ::android::hardware::radio::V1_2::IRadio>(
            RadioHidlEnvironment::Instance()
                ->getServiceName<::android::hardware::radio::V1_2::IRadio>(
                    hidl_string(RADIO_SERVICE_NAME)));
    }
    ASSERT_NE(nullptr, radio_v1_2.get());

    radioRsp_v1_2 = new (std::nothrow) RadioResponse_v1_2(*this);
    ASSERT_NE(nullptr, radioRsp_v1_2.get());

    count_ = 0;
    logicalSlotId = -1;

    radioInd_v1_2 = new (std::nothrow) RadioIndication_v1_2(*this);
    ASSERT_NE(nullptr, radioInd_v1_2.get());

    radio_v1_2->setResponseFunctions(radioRsp_v1_2, radioInd_v1_2);

    updateSimCardStatus();
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_2->rspInfo.error);

    /* Enforce Vts Testing with Sim Status Present only. */
    EXPECT_EQ(CardState::PRESENT, cardStatus.base.cardState);

    radioConfig = ::testing::VtsHalHidlTargetTestBase::getService<
            ::android::hardware::radio::config::V1_1::IRadioConfig>();

    /* Enforce Vts tesing with RadioConfig for network scan excemption. */
    // Some devices can only perform network scan on logical modem that currently used for packet
    // data. This exemption is removed in HAL version 1.4. See b/135243177 for additional info.
    if (radioConfig != NULL) {
        // RadioConfig 1.1 available, some devices fall in excepmtion category.
        ASSERT_NE(nullptr, radioConfig.get());

        radioConfigRsp = new (std::nothrow) RadioConfigResponse(*this);
        ASSERT_NE(nullptr, radioConfigRsp.get());

        /* Set radio config response functions */
        radioConfig->setResponseFunctions(radioConfigRsp, nullptr);

        /* set preferred data modem */
        setPreferredDataModem();

        /* get current logical sim id */
        getLogicalSimId();
    }
}

void RadioHidlTest_v1_2::getLogicalSimId() {
    serial = GetRandomSerialNumber();
    radioConfig->getSimSlotsStatus(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(radioConfigRsp->rspInfo.error,
                                 {RadioError::NONE, RadioError::REQUEST_NOT_SUPPORTED}));

    if (radioConfigRsp->rspInfo.error != RadioError ::NONE) {
        ALOGI("Failed to get sim slot status, rspInfo.error = %s\n",
              toString(radioConfigRsp->rspInfo.error).c_str());
        return;
    }

    if (cardStatus.physicalSlotId < 0 ||
        cardStatus.physicalSlotId >= radioConfigRsp->simSlotStatus.size()) {
        ALOGI("Physical slot id: %d is out of range", cardStatus.physicalSlotId);
        return;
    }

    logicalSlotId = radioConfigRsp->simSlotStatus[cardStatus.physicalSlotId].logicalSlotId;
}

/*
 * Set preferred data modem
 */
void RadioHidlTest_v1_2::setPreferredDataModem() {
    serial = GetRandomSerialNumber();
    // Even for single sim device, the setPreferredDataModem should still success. Enforce dds on
    // first logical modem.
    radioConfig->setPreferredDataModem(serial, DDS_LOGICAL_SLOT_INDEX);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioConfigRsp->rspInfo.type);
    EXPECT_EQ(serial, radioConfigRsp->rspInfo.serial);

    ASSERT_TRUE(CheckAnyOfErrors(
            radioConfigRsp->rspInfo.error,
            {RadioError::NONE, RadioError::RADIO_NOT_AVAILABLE, RadioError::INTERNAL_ERR}));
}

/*
 * Notify that the response message is received.
 */
void RadioHidlTest_v1_2::notify(int receivedSerial) {
    std::unique_lock<std::mutex> lock(mtx_);
    if (serial == receivedSerial) {
        count_++;
        cv_.notify_one();
    }
}

/*
 * Wait till the response message is notified or till TIMEOUT_PERIOD.
 */
std::cv_status RadioHidlTest_v1_2::wait() {
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

void RadioHidlTest_v1_2::updateSimCardStatus() {
    serial = GetRandomSerialNumber();
    radio_v1_2->getIccCardStatus(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}

void RadioHidlTest_v1_2::stopNetworkScan() {
    serial = GetRandomSerialNumber();
    radio_v1_2->stopNetworkScan(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}
