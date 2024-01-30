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

#include <aidl/android/hardware/radio/modem/BnRadioModemIndication.h>
#include <aidl/android/hardware/radio/modem/BnRadioModemResponse.h>
#include <aidl/android/hardware/radio/modem/IRadioModem.h>
#include <aidl/android/hardware/radio/modem/ImeiInfo.h>

#include "radio_aidl_hal_utils.h"

using namespace aidl::android::hardware::radio::modem;

class RadioModemTest;

/* Callback class for radio modem response */
class RadioModemResponse : public BnRadioModemResponse {
  protected:
    RadioServiceTest& parent_modem;

  public:
    RadioModemResponse(RadioServiceTest& parent_modem);
    virtual ~RadioModemResponse() = default;

    RadioResponseInfo rspInfo;
    bool isModemEnabled;
    bool enableModemResponseToggle = false;

    virtual ndk::ScopedAStatus acknowledgeRequest(int32_t serial) override;

    virtual ndk::ScopedAStatus enableModemResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus getBasebandVersionResponse(const RadioResponseInfo& info,
                                                          const std::string& version) override;

    virtual ndk::ScopedAStatus getDeviceIdentityResponse(const RadioResponseInfo& info,
                                                         const std::string& imei,
                                                         const std::string& imeisv,
                                                         const std::string& esn,
                                                         const std::string& meid) override;

    virtual ndk::ScopedAStatus getImeiResponse(const RadioResponseInfo& info,
            const std::optional<ImeiInfo>& config) override;

    virtual ndk::ScopedAStatus getHardwareConfigResponse(
            const RadioResponseInfo& info, const std::vector<HardwareConfig>& config) override;

    virtual ndk::ScopedAStatus getModemActivityInfoResponse(
            const RadioResponseInfo& info, const ActivityStatsInfo& activityInfo) override;

    virtual ndk::ScopedAStatus getModemStackStatusResponse(const RadioResponseInfo& info,
                                                           const bool enabled) override;

    virtual ndk::ScopedAStatus getRadioCapabilityResponse(const RadioResponseInfo& info,
                                                          const RadioCapability& rc) override;

    virtual ndk::ScopedAStatus nvReadItemResponse(const RadioResponseInfo& info,
                                                  const std::string& result) override;

    virtual ndk::ScopedAStatus nvResetConfigResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus nvWriteCdmaPrlResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus nvWriteItemResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus requestShutdownResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus sendDeviceStateResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setRadioCapabilityResponse(const RadioResponseInfo& info,
                                                          const RadioCapability& rc) override;

    virtual ndk::ScopedAStatus setRadioPowerResponse(const RadioResponseInfo& info) override;
};

/* Callback class for radio modem indication */
class RadioModemIndication : public BnRadioModemIndication {
  protected:
    RadioServiceTest& parent_modem;

  public:
    RadioModemIndication(RadioServiceTest& parent_modem);
    virtual ~RadioModemIndication() = default;

    virtual ndk::ScopedAStatus hardwareConfigChanged(
            RadioIndicationType type, const std::vector<HardwareConfig>& configs) override;

    virtual ndk::ScopedAStatus modemReset(RadioIndicationType type,
                                          const std::string& reason) override;

    virtual ndk::ScopedAStatus radioCapabilityIndication(RadioIndicationType type,
                                                         const RadioCapability& rc) override;

    virtual ndk::ScopedAStatus radioStateChanged(RadioIndicationType type,
                                                 RadioState radioState) override;

    virtual ndk::ScopedAStatus rilConnected(RadioIndicationType type) override;

    virtual ndk::ScopedAStatus onImeiMappingChanged(RadioIndicationType type,
            const ::aidl::android::hardware::radio::modem::ImeiInfo& imeiInfo) override;
};

// The main test class for Radio AIDL Modem.
class RadioModemTest : public RadioServiceTest {
  public:
    void SetUp() override;

    /* radio modem service handle */
    std::shared_ptr<IRadioModem> radio_modem;
    /* radio modem response handle */
    std::shared_ptr<RadioModemResponse> radioRsp_modem;
    /* radio modem indication handle */
    std::shared_ptr<RadioModemIndication> radioInd_modem;
};
