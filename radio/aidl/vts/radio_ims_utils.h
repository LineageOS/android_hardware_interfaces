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

#include <aidl/android/hardware/radio/ims/BnRadioImsIndication.h>
#include <aidl/android/hardware/radio/ims/BnRadioImsResponse.h>
#include <aidl/android/hardware/radio/ims/IRadioIms.h>

#include "radio_aidl_hal_utils.h"

using namespace aidl::android::hardware::radio::ims;

class RadioImsTest;

/* Callback class for radio ims response */
class RadioImsResponse : public BnRadioImsResponse {
  protected:
    RadioServiceTest& parent_ims;

  public:
    RadioImsResponse(RadioServiceTest& parent_ims);
    virtual ~RadioImsResponse() = default;

    RadioResponseInfo rspInfo;
    std::optional<ConnectionFailureInfo> startImsTrafficResp;

    virtual ndk::ScopedAStatus setSrvccCallInfoResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus updateImsRegistrationInfoResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus startImsTrafficResponse(
            const RadioResponseInfo& info,
            const std::optional<ConnectionFailureInfo>& response) override;

    virtual ndk::ScopedAStatus stopImsTrafficResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus triggerEpsFallbackResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus sendAnbrQueryResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus updateImsCallStatusResponse(const RadioResponseInfo& info) override;
};

/* Callback class for radio ims indication */
class RadioImsIndication : public BnRadioImsIndication {
  protected:
    RadioServiceTest& parent_ims;

  public:
    RadioImsIndication(RadioServiceTest& parent_ims);
    virtual ~RadioImsIndication() = default;

    virtual ndk::ScopedAStatus onConnectionSetupFailure(RadioIndicationType type,
            int32_t token, const ConnectionFailureInfo& info) override;

    virtual ndk::ScopedAStatus notifyAnbr(RadioIndicationType type, ImsStreamType mediaType,
            ImsStreamDirection direction, int bitsPerSecond) override;

    virtual ndk::ScopedAStatus triggerImsDeregistration(RadioIndicationType type,
            ImsDeregistrationReason reason) override;
};

// The main test class for Radio AIDL Ims.
class RadioImsTest : public RadioServiceTest {
  protected:
    virtual void verifyError(RadioError resp);

  public:
    void SetUp() override;

    /* radio ims service handle */
    std::shared_ptr<IRadioIms> radio_ims;
    /* radio ims response handle */
    std::shared_ptr<RadioImsResponse> radioRsp_ims;
    /* radio ims indication handle */
    std::shared_ptr<RadioImsIndication> radioInd_ims;
};
