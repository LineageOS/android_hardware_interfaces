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

#include <aidl/android/hardware/radio/config/BnRadioConfigIndication.h>
#include <aidl/android/hardware/radio/config/BnRadioConfigResponse.h>
#include <aidl/android/hardware/radio/config/IRadioConfig.h>

#include "radio_aidl_hal_utils.h"

using namespace aidl::android::hardware::radio::config;

class RadioConfigTest;

/* Callback class for radio config response */
class RadioConfigResponse : public BnRadioConfigResponse {
  protected:
    RadioServiceTest& parent_config;

  public:
    RadioConfigResponse(RadioServiceTest& parent_config);
    virtual ~RadioConfigResponse() = default;

    RadioResponseInfo rspInfo;
    PhoneCapability phoneCap;
    bool modemReducedFeatureSet1;
    std::vector<SimSlotStatus> simSlotStatus;

    virtual ndk::ScopedAStatus getSimSlotsStatusResponse(
            const RadioResponseInfo& info, const std::vector<SimSlotStatus>& slotStatus) override;

    virtual ndk::ScopedAStatus setSimSlotsMappingResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus getPhoneCapabilityResponse(
            const RadioResponseInfo& info, const PhoneCapability& phoneCapability) override;

    virtual ndk::ScopedAStatus setPreferredDataModemResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus getNumOfLiveModemsResponse(const RadioResponseInfo& info,
                                                          const int8_t numOfLiveModems) override;

    virtual ndk::ScopedAStatus setNumOfLiveModemsResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus getHalDeviceCapabilitiesResponse(
            const RadioResponseInfo& info, bool modemReducedFeatureSet1) override;
};

/* Callback class for radio config indication */
class RadioConfigIndication : public BnRadioConfigIndication {
  protected:
    RadioServiceTest& parent_config;

  public:
    RadioConfigIndication(RadioServiceTest& parent_config);
    virtual ~RadioConfigIndication() = default;

    virtual ndk::ScopedAStatus simSlotsStatusChanged(
            RadioIndicationType type, const std::vector<SimSlotStatus>& slotStatus) override;
};

// The main test class for Radio AIDL Config.
class RadioConfigTest : public RadioServiceTest {
  public:
    void SetUp() override;

    ndk::ScopedAStatus updateSimCardStatus();
    /* Override updateSimSlotStatus in RadioServiceTest to not call setResponseFunctions */
    void updateSimSlotStatus();

    /* radio config service handle in RadioServiceTest */
    /* radio config response handle */
    std::shared_ptr<RadioConfigResponse> radioRsp_config;
    /* radio config indication handle */
    std::shared_ptr<RadioConfigIndication> radioInd_config;
};
