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
    radio_v1_2 = ::testing::VtsHalHidlTargetTestBase::getService<V1_2::IRadio>(
        hidl_string(RADIO_SERVICE_NAME));
    ASSERT_NE(nullptr, radio_v1_2.get());

    radioRsp_v1_2 = new (std::nothrow) RadioResponse_v1_2(*this);
    ASSERT_NE(nullptr, radioRsp_v1_2.get());

    count_ = 0;

    radioInd_v1_2 = new (std::nothrow) RadioIndication_v1_2(*this);
    ASSERT_NE(nullptr, radioInd_v1_2.get());

    radio_v1_2->setResponseFunctions(radioRsp_v1_2, radioInd_v1_2);

    int serial = GetRandomSerialNumber();
    radio_v1_2->getIccCardStatus(serial);
    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(RadioResponseType::SOLICITED, radioRsp_v1_2->rspInfo.type);
    EXPECT_EQ(serial, radioRsp_v1_2->rspInfo.serial);
    EXPECT_EQ(RadioError::NONE, radioRsp_v1_2->rspInfo.error);
}

/*
 * Notify that the response message is received.
 */
void RadioHidlTest_v1_2::notify() {
    std::unique_lock<std::mutex> lock(mtx_);
    count_++;
    cv_.notify_one();
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
