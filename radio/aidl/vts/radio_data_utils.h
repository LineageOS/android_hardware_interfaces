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

#include <aidl/android/hardware/radio/data/BnRadioDataIndication.h>
#include <aidl/android/hardware/radio/data/BnRadioDataResponse.h>
#include <aidl/android/hardware/radio/data/IRadioData.h>

#include "radio_aidl_hal_utils.h"

using namespace aidl::android::hardware::radio::data;

class RadioDataTest;

/* Callback class for radio data response */
class RadioDataResponse : public BnRadioDataResponse {
  protected:
    RadioServiceTest& parent_data;

  public:
    RadioDataResponse(RadioServiceTest& parent_data);
    virtual ~RadioDataResponse() = default;

    RadioResponseInfo rspInfo;
    int32_t allocatedPduSessionId;
    SetupDataCallResult setupDataCallResult;

    virtual ndk::ScopedAStatus acknowledgeRequest(int32_t serial) override;

    virtual ndk::ScopedAStatus allocatePduSessionIdResponse(const RadioResponseInfo& info,
                                                            int32_t id) override;

    virtual ndk::ScopedAStatus cancelHandoverResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus deactivateDataCallResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus getDataCallListResponse(
            const RadioResponseInfo& info,
            const std::vector<SetupDataCallResult>& dcResponse) override;

    virtual ndk::ScopedAStatus getSlicingConfigResponse(
            const RadioResponseInfo& info, const SlicingConfig& slicingConfig) override;

    virtual ndk::ScopedAStatus releasePduSessionIdResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setDataAllowedResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setDataProfileResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setDataThrottlingResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setInitialAttachApnResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setupDataCallResponse(
            const RadioResponseInfo& info, const SetupDataCallResult& dcResponse) override;

    virtual ndk::ScopedAStatus startHandoverResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus startKeepaliveResponse(const RadioResponseInfo& info,
                                                      const KeepaliveStatus& status) override;

    virtual ndk::ScopedAStatus stopKeepaliveResponse(const RadioResponseInfo& info) override;
};

/* Callback class for radio data indication */
class RadioDataIndication : public BnRadioDataIndication {
  protected:
    RadioServiceTest& parent_data;

  public:
    RadioDataIndication(RadioServiceTest& parent_data);
    virtual ~RadioDataIndication() = default;

    virtual ndk::ScopedAStatus dataCallListChanged(
            RadioIndicationType type, const std::vector<SetupDataCallResult>& dcList) override;

    virtual ndk::ScopedAStatus keepaliveStatus(RadioIndicationType type,
                                               const KeepaliveStatus& status) override;

    virtual ndk::ScopedAStatus pcoData(RadioIndicationType type, const PcoDataInfo& pco) override;

    virtual ndk::ScopedAStatus unthrottleApn(RadioIndicationType type,
                                             const DataProfileInfo& dataProfile) override;
    virtual ndk::ScopedAStatus slicingConfigChanged(RadioIndicationType type,
                                                    const SlicingConfig& slicingConfig) override;
};

// The main test class for Radio AIDL Data.
class RadioDataTest : public RadioServiceTest {
  protected:
    /* Get current data call list */
    ndk::ScopedAStatus getDataCallList();

  public:
    void SetUp() override;

    /* radio data service handle */
    std::shared_ptr<IRadioData> radio_data;
    /* radio data response handle */
    std::shared_ptr<RadioDataResponse> radioRsp_data;
    /* radio data indication handle */
    std::shared_ptr<RadioDataIndication> radioInd_data;
};
