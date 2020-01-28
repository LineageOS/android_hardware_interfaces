/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <radio_config_hidl_hal_utils.h>

void RadioConfigHidlTest::SetUp() {
    radioConfig = V1_3::IRadioConfig::getService(GetParam());
    ASSERT_NE(nullptr, radioConfig.get());

    radioConfigRsp = new (std::nothrow) RadioConfigResponse(*this);
    ASSERT_NE(nullptr, radioConfigRsp.get());

    count_ = 0;

    radioConfig->setResponseFunctions(radioConfigRsp, nullptr);
}

/*
 * Notify that the response message is received.
 */
void RadioConfigHidlTest::notify(int receivedSerial) {
    std::unique_lock<std::mutex> lock(mtx_);
    if (serial == receivedSerial) {
        count_++;
        cv_.notify_one();
    }
}

/*
 * Wait till the response message is notified or till TIMEOUT_PERIOD.
 */
std::cv_status RadioConfigHidlTest::wait() {
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
