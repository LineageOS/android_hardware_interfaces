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

#include <android/binder_manager.h>

#include "radio_sap_utils.h"

#define ASSERT_OK(ret) ASSERT_TRUE((ret).isOk())
#define TIMEOUT_PERIOD 40

void SapTest::SetUp() {
    ALOGD("BEGIN %s#%s", ::testing::UnitTest::GetInstance()->current_test_info()->test_suite_name(),
          ::testing::UnitTest::GetInstance()->current_test_info()->name());
    count = 0;
    serial = -1;
    std::string serviceName = GetParam();
    if (!isServiceValidForDeviceConfiguration(serviceName)) {
        ALOGI("Skipped the test due to device configuration.");
        GTEST_SKIP();
    }
    sap = ISap::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService(serviceName.c_str())));
    ASSERT_NE(sap.get(), nullptr);

    sapCb = ndk::SharedRefBase::make<SapCallback>(*this);
    ASSERT_NE(sapCb.get(), nullptr);

    ndk::ScopedAStatus res = sap->setCallback(sapCb);
    ASSERT_OK(res) << res;
}

void SapTest::TearDown() {
    count_ = 0;
    serial = -1;
    ALOGD("END %s#%s", ::testing::UnitTest::GetInstance()->current_test_info()->test_suite_name(),
          ::testing::UnitTest::GetInstance()->current_test_info()->name());
}

::testing::AssertionResult SapTest::CheckAnyOfErrors(SapResultCode err,
                                                     std::vector<SapResultCode> errors) {
    for (size_t i = 0; i < errors.size(); i++) {
        if (err == errors[i]) {
            return testing::AssertionSuccess();
        }
    }
    return testing::AssertionFailure() << "SapError:" + toString(err) + " is returned";
}

void SapTest::notify(int receivedSerial) {
    std::lock_guard<std::mutex> lock(mtx);
    if (serial == receivedSerial) {
        count++;
        cv.notify_one();
    }
}

std::cv_status SapTest::wait() {
    std::unique_lock<std::mutex> lock(mtx);

    std::cv_status status = std::cv_status::no_timeout;
    auto now = std::chrono::system_clock::now();
    while (count == 0) {
        status = cv.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
        if (status == std::cv_status::timeout) {
            return status;
        }
    }
    count--;
    return status;
}

/*
 * Test ISap.connectReq() for the response returned.
 */
TEST_P(SapTest, connectReq) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping connectReq "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    int32_t maxMsgSize = 100;

    ndk::ScopedAStatus res = sap->connectReq(serial, maxMsgSize);
    ASSERT_OK(res) << res;

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseSerial, serial);

    // Modem side need time for connect to finish. Adding a waiting time to prevent
    // disconnect being requested right after connect request.
    sleep(1);
}

/*
 * Test ISap.disconnectReq() for the response returned
 */
TEST_P(SapTest, disconnectReq) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping disconnectReq "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = sap->disconnectReq(serial);
    ASSERT_OK(res) << res;

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseSerial, serial);
}

/*
 * Test ISap.apduReq() for the response returned.
 */
TEST_P(SapTest, apduReq) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping apduReq "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    SapApduType sapApduType = SapApduType::APDU;
    std::vector<uint8_t> command = {};

    ndk::ScopedAStatus res = sap->apduReq(serial, sapApduType, command);
    ASSERT_OK(res) << res;

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseSerial, serial);

    ASSERT_TRUE(CheckAnyOfErrors(
            sapCb->sapResultCode,
            {SapResultCode::GENERIC_FAILURE, SapResultCode::CARD_ALREADY_POWERED_OFF,
             SapResultCode::CARD_NOT_ACCESSSIBLE, SapResultCode::CARD_REMOVED,
             SapResultCode::SUCCESS}));
}

/*
 * Test ISap.transferAtrReq() for the response returned.
 */
TEST_P(SapTest, transferAtrReq) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping transferAtrReq "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = sap->transferAtrReq(serial);
    ASSERT_OK(res) << res;

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseSerial, serial);

    ASSERT_TRUE(CheckAnyOfErrors(sapCb->sapResultCode,
                                 {SapResultCode::GENERIC_FAILURE, SapResultCode::DATA_NOT_AVAILABLE,
                                  SapResultCode::CARD_ALREADY_POWERED_OFF,
                                  SapResultCode::CARD_REMOVED, SapResultCode::SUCCESS}));
}

/*
 * Test ISap.powerReq() for the response returned.
 */
TEST_P(SapTest, powerReq) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping powerReq "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    bool state = true;

    ndk::ScopedAStatus res = sap->powerReq(serial, state);
    ASSERT_OK(res) << res;

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseSerial, serial);

    ASSERT_TRUE(
            CheckAnyOfErrors(sapCb->sapResultCode,
                             {SapResultCode::GENERIC_FAILURE, SapResultCode::CARD_NOT_ACCESSSIBLE,
                              SapResultCode::CARD_ALREADY_POWERED_OFF, SapResultCode::CARD_REMOVED,
                              SapResultCode::CARD_ALREADY_POWERED_ON, SapResultCode::SUCCESS}));
}

/*
 * Test ISap.resetSimReq() for the response returned.
 */
TEST_P(SapTest, resetSimReq) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping resetSimReq "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = sap->resetSimReq(serial);
    ASSERT_OK(res) << res;

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseSerial, serial);

    ASSERT_TRUE(
            CheckAnyOfErrors(sapCb->sapResultCode,
                             {SapResultCode::GENERIC_FAILURE, SapResultCode::CARD_NOT_ACCESSSIBLE,
                              SapResultCode::CARD_ALREADY_POWERED_OFF, SapResultCode::CARD_REMOVED,
                              SapResultCode::SUCCESS}));
}

/*
 * Test ISap.transferCardReaderStatusReq() for the response returned.
 */
TEST_P(SapTest, transferCardReaderStatusReq) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping transferCardReaderStatusReq "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();

    ndk::ScopedAStatus res = sap->transferCardReaderStatusReq(serial);
    ASSERT_OK(res) << res;

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseSerial, serial);

    ASSERT_TRUE(CheckAnyOfErrors(sapCb->sapResultCode,
                                 {SapResultCode::GENERIC_FAILURE, SapResultCode::DATA_NOT_AVAILABLE,
                                  SapResultCode::SUCCESS}));
}

/*
 * Test ISap.setTransferProtocolReq() for the response returned.
 */
TEST_P(SapTest, setTransferProtocolReq) {
    if (telephony_flags::enforce_telephony_feature_mapping()) {
        if (!deviceSupportsFeature(FEATURE_TELEPHONY_SUBSCRIPTION)) {
            GTEST_SKIP() << "Skipping setTransferProtocolReq "
                            "due to undefined FEATURE_TELEPHONY_SUBSCRIPTION";
        }
    }

    serial = GetRandomSerialNumber();
    SapTransferProtocol sapTransferProtocol = SapTransferProtocol::T0;

    ndk::ScopedAStatus res = sap->setTransferProtocolReq(serial, sapTransferProtocol);
    ASSERT_OK(res) << res;

    EXPECT_EQ(std::cv_status::no_timeout, wait());
    EXPECT_EQ(sapCb->sapResponseSerial, serial);

    ASSERT_TRUE(CheckAnyOfErrors(sapCb->sapResultCode,
                                 {SapResultCode::NOT_SUPPORTED, SapResultCode::SUCCESS}));
}
