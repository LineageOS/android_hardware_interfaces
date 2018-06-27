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

#include <radio_hidl_hal_utils_v1_1.h>

void RadioHidlTest_v1_1::SetUp() {
    radio_v1_1 =
        ::testing::VtsHalHidlTargetTestBase::getService<::android::hardware::radio::V1_1::IRadio>(
            RadioHidlEnvironment::Instance()
                ->getServiceName<::android::hardware::radio::V1_1::IRadio>(
                    hidl_string(RADIO_SERVICE_NAME)));
    if (radio_v1_1 == NULL) {
        sleep(60);
        radio_v1_1 = ::testing::VtsHalHidlTargetTestBase::getService<
            ::android::hardware::radio::V1_1::IRadio>(
            RadioHidlEnvironment::Instance()
                ->getServiceName<::android::hardware::radio::V1_1::IRadio>(
                    hidl_string(RADIO_SERVICE_NAME)));
    }
    ASSERT_NE(nullptr, radio_v1_1.get());

    radioRsp_v1_1 = new (std::nothrow) RadioResponse_v1_1(*this);
    ASSERT_NE(nullptr, radioRsp_v1_1.get());

    count = 0;

    radioInd_v1_1 = new (std::nothrow) RadioIndication_v1_1(*this);
    ASSERT_NE(nullptr, radioInd_v1_1.get());

    radio_v1_1->setResponseFunctions(radioRsp_v1_1, radioInd_v1_1);

    updateSimCardStatus();
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_1->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_1->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_1->rspInfo.error);

    /* Enforce Vts Testing with Sim Status Present only. */
    EXPECT_EQ(CardState::PRESENT, cardStatus.cardState);
}

void RadioHidlTest_v1_1::notify(int receivedSerial) {
    std::unique_lock<std::mutex> lock(mtx);
    if (serial == receivedSerial) {
        count++;
        cv.notify_one();
    }
}

std::cv_status RadioHidlTest_v1_1::wait(int sec) {
    std::unique_lock<std::mutex> lock(mtx);

    std::cv_status status = std::cv_status::no_timeout;
    auto now = std::chrono::system_clock::now();
    while (count == 0) {
        status = cv.wait_until(lock, now + std::chrono::seconds(sec));
        if (status == std::cv_status::timeout) {
            return status;
        }
    }
    count--;
    return status;
}

void RadioHidlTest_v1_1::updateSimCardStatus() {
    serial = GetRandomSerialNumber();
    radio_v1_1->getIccCardStatus(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}