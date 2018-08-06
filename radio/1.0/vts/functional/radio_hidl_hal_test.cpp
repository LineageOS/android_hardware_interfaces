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

#include <radio_hidl_hal_utils_v1_0.h>

void RadioHidlTest::SetUp() {
    radio = ::testing::VtsHalHidlTargetTestBase::getService<IRadio>(
        RadioHidlEnvironment::Instance()->getServiceName<IRadio>(hidl_string(RADIO_SERVICE_NAME)));
    if (radio == NULL) {
        sleep(60);
        radio = ::testing::VtsHalHidlTargetTestBase::getService<IRadio>(
            RadioHidlEnvironment::Instance()->getServiceName<IRadio>(
                hidl_string(RADIO_SERVICE_NAME)));
    }
    ASSERT_NE(nullptr, radio.get());

    radioRsp = new (std::nothrow) RadioResponse(*this);
    ASSERT_NE(nullptr, radioRsp.get());

    count = 0;

    radioInd = new (std::nothrow) RadioIndication(*this);
    ASSERT_NE(nullptr, radioInd.get());

    radio->setResponseFunctions(radioRsp, radioInd);

    updateSimCardStatus();
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp->rspInfo.type);
    EXPECT_EQ(serial, radioRsp->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp->rspInfo.error);

    /* Enforce Vts Testing with Sim Status Present only. */
    EXPECT_EQ(CardState::PRESENT, cardStatus.cardState);
}

void RadioHidlTest::notify(int receivedSerial) {
    std::unique_lock<std::mutex> lock(mtx);
    if (serial == receivedSerial) {
        count++;
        cv.notify_one();
    }
}

std::cv_status RadioHidlTest::wait(int sec) {
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

void RadioHidlTest::updateSimCardStatus() {
    serial = GetRandomSerialNumber();
    radio->getIccCardStatus(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
}