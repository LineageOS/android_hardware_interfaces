/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <aidl/android/hardware/radio/messaging/BnRadioMessagingIndication.h>
#include <aidl/android/hardware/radio/messaging/BnRadioMessagingResponse.h>
#include <aidl/android/hardware/radio/messaging/IRadioMessaging.h>

#include "radio_aidl_hal_utils.h"

using namespace aidl::android::hardware::radio::messaging;

class RadioMessagingTest;

/* Callback class for radio messaging response */
class RadioMessagingResponse : public BnRadioMessagingResponse {
  protected:
    RadioServiceTest& parent_messaging;

  public:
    RadioMessagingResponse(RadioServiceTest& parent_messaging);
    virtual ~RadioMessagingResponse() = default;

    RadioResponseInfo rspInfo;
    SendSmsResult sendSmsResult;

    virtual ndk::ScopedAStatus acknowledgeIncomingGsmSmsWithPduResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus acknowledgeLastIncomingCdmaSmsResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus acknowledgeLastIncomingGsmSmsResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus acknowledgeRequest(int32_t serial) override;

    virtual ndk::ScopedAStatus deleteSmsOnRuimResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus deleteSmsOnSimResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus getCdmaBroadcastConfigResponse(
            const RadioResponseInfo& info,
            const std::vector<CdmaBroadcastSmsConfigInfo>& configs) override;

    virtual ndk::ScopedAStatus getGsmBroadcastConfigResponse(
            const RadioResponseInfo& info,
            const std::vector<GsmBroadcastSmsConfigInfo>& configs) override;

    virtual ndk::ScopedAStatus getSmscAddressResponse(const RadioResponseInfo& info,
                                                      const std::string& smsc) override;

    virtual ndk::ScopedAStatus reportSmsMemoryStatusResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus sendCdmaSmsExpectMoreResponse(const RadioResponseInfo& info,
                                                             const SendSmsResult& sms) override;

    virtual ndk::ScopedAStatus sendCdmaSmsResponse(const RadioResponseInfo& info,
                                                   const SendSmsResult& sms) override;

    virtual ndk::ScopedAStatus sendImsSmsResponse(const RadioResponseInfo& info,
                                                  const SendSmsResult& sms) override;

    virtual ndk::ScopedAStatus sendSmsExpectMoreResponse(const RadioResponseInfo& info,
                                                         const SendSmsResult& sms) override;

    virtual ndk::ScopedAStatus sendSmsResponse(const RadioResponseInfo& info,
                                               const SendSmsResult& sms) override;

    virtual ndk::ScopedAStatus setCdmaBroadcastActivationResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setCdmaBroadcastConfigResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setGsmBroadcastActivationResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setGsmBroadcastConfigResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setSmscAddressResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus writeSmsToRuimResponse(const RadioResponseInfo& info,
                                                      int32_t index) override;

    virtual ndk::ScopedAStatus writeSmsToSimResponse(const RadioResponseInfo& info,
                                                     int32_t index) override;
};

/* Callback class for radio messaging indication */
class RadioMessagingIndication : public BnRadioMessagingIndication {
  protected:
    RadioServiceTest& parent_messaging;

  public:
    RadioMessagingIndication(RadioServiceTest& parent_messaging);
    virtual ~RadioMessagingIndication() = default;

    virtual ndk::ScopedAStatus cdmaNewSms(RadioIndicationType type,
                                          const CdmaSmsMessage& msg) override;

    virtual ndk::ScopedAStatus cdmaRuimSmsStorageFull(RadioIndicationType type) override;

    virtual ndk::ScopedAStatus newBroadcastSms(RadioIndicationType type,
                                               const std::vector<uint8_t>& data) override;

    virtual ndk::ScopedAStatus newSms(RadioIndicationType type,
                                      const std::vector<uint8_t>& pdu) override;

    virtual ndk::ScopedAStatus newSmsOnSim(RadioIndicationType type, int32_t recordNumber) override;

    virtual ndk::ScopedAStatus newSmsStatusReport(RadioIndicationType type,
                                                  const std::vector<uint8_t>& pdu) override;

    virtual ndk::ScopedAStatus simSmsStorageFull(RadioIndicationType type) override;
};

// The main test class for Radio AIDL Messaging.
class RadioMessagingTest : public RadioServiceTest {
  public:
    void SetUp() override;

    /* radio messaging service handle */
    std::shared_ptr<IRadioMessaging> radio_messaging;
    /* radio messaging response handle */
    std::shared_ptr<RadioMessagingResponse> radioRsp_messaging;
    /* radio messaging indication handle */
    std::shared_ptr<RadioMessagingIndication> radioInd_messaging;
};
