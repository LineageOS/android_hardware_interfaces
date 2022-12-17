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
#pragma once

#include <aidl/Gtest.h>
#include <aidl/android/hardware/radio/sap/BnSapCallback.h>
#include <aidl/android/hardware/radio/sap/ISap.h>

#include "radio_aidl_hal_utils.h"

using namespace aidl::android::hardware::radio::sap;

class SapTest;

/* Callback class for radio sap response */
class SapCallback : public BnSapCallback {
  protected:
    SapTest& parent_sap;

  public:
    SapCallback(SapTest& parent_config);
    virtual ~SapCallback() = default;

    int32_t sapResponseSerial;
    SapResultCode sapResultCode;

    virtual ::ndk::ScopedAStatus apduResponse(int32_t serial, SapResultCode resultCode,
                                              const std::vector<uint8_t>& adpuRsp) override;

    virtual ::ndk::ScopedAStatus connectResponse(int32_t serial, SapConnectRsp sapConnectRsp,
                                                 int32_t maxMsgSize) override;

    virtual ::ndk::ScopedAStatus disconnectIndication(int32_t serial,
                                                      SapDisconnectType sapDisconnectType) override;

    virtual ::ndk::ScopedAStatus disconnectResponse(int32_t serial) override;

    virtual ::ndk::ScopedAStatus errorResponse(int32_t serial) override;

    virtual ::ndk::ScopedAStatus powerResponse(int32_t serial, SapResultCode resultCode) override;

    virtual ::ndk::ScopedAStatus resetSimResponse(int32_t serial,
                                                  SapResultCode resultCode) override;

    virtual ::ndk::ScopedAStatus statusIndication(int32_t serial, SapStatus sapStatus) override;

    virtual ::ndk::ScopedAStatus transferAtrResponse(int32_t serial, SapResultCode resultCode,
                                                     const std::vector<uint8_t>& atr) override;

    virtual ::ndk::ScopedAStatus transferCardReaderStatusResponse(
            int32_t serial, SapResultCode resultCode, int32_t cardReaderStatus) override;

    virtual ::ndk::ScopedAStatus transferProtocolResponse(int32_t serial,
                                                          SapResultCode resultCode) override;
};

// The main test class for  AIDL SAP.
class SapTest : public ::testing::TestWithParam<std::string> {
  private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;

  public:
    virtual void SetUp() override;

    virtual void TearDown() override;

    ::testing::AssertionResult CheckAnyOfErrors(SapResultCode err,
                                                std::vector<SapResultCode> errors);

    /* Used as a mechanism to inform the test about data/event callback */
    void notify(int receivedSerial);

    /* Test code calls this function to wait for response */
    std::cv_status wait();

    /* Sap service */
    std::shared_ptr<ISap> sap;

    /* Sap Callback object */
    std::shared_ptr<SapCallback> sapCb;

    /* Serial for sap request */
    int32_t serial;
};
