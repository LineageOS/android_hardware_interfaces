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

#include <aidl/android/hardware/radio/satellite/BnRadioSatelliteIndication.h>
#include <aidl/android/hardware/radio/satellite/BnRadioSatelliteResponse.h>
#include <aidl/android/hardware/radio/satellite/IRadioSatellite.h>

#include "radio_aidl_hal_utils.h"

using namespace aidl::android::hardware::radio::satellite;

class RadioSatelliteTest;

/* Callback class for Satellite response */
class RadioSatelliteResponse : public BnRadioSatelliteResponse {
  protected:
    RadioServiceTest& parent_satellite;

  public:
    RadioSatelliteResponse(RadioServiceTest& parent_satellite);
    virtual ~RadioSatelliteResponse() = default;

    RadioResponseInfo rspInfo;

    virtual ndk::ScopedAStatus acknowledgeRequest(int32_t serial) override;

    virtual ndk::ScopedAStatus getCapabilitiesResponse(
            const RadioResponseInfo& info, const SatelliteCapabilities& capabilities) override;

    virtual ndk::ScopedAStatus setPowerResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus getPowerStateResponse(const RadioResponseInfo& info,
                                                     bool on) override;

    virtual ndk::ScopedAStatus provisionServiceResponse(const RadioResponseInfo& info,
                                                        bool provisioned) override;

    virtual ndk::ScopedAStatus addAllowedSatelliteContactsResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus removeAllowedSatelliteContactsResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus sendMessagesResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus getPendingMessagesResponse(
            const RadioResponseInfo& info, const std::vector<std::string>& /*messages*/) override;

    virtual ndk::ScopedAStatus getSatelliteModeResponse(
            const RadioResponseInfo& info, SatelliteMode mode,
            satellite::NTRadioTechnology technology) override;

    virtual ndk::ScopedAStatus setIndicationFilterResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus startSendingSatellitePointingInfoResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus stopSendingSatellitePointingInfoResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus getMaxCharactersPerTextMessageResponse(const RadioResponseInfo& info,
                                                                      int32_t charLimit) override;

    virtual ndk::ScopedAStatus getTimeForNextSatelliteVisibilityResponse(
            const RadioResponseInfo& info, int32_t timeInSeconds) override;
};

/* Callback class for Satellite indication */
class RadioSatelliteIndication : public BnRadioSatelliteIndication {
  protected:
    RadioServiceTest& parent_satellite;

  public:
    RadioSatelliteIndication(RadioServiceTest& parent_satellite);
    virtual ~RadioSatelliteIndication() = default;

    virtual ndk::ScopedAStatus onPendingMessageCount(RadioIndicationType type,
                                                     int32_t count) override;

    virtual ndk::ScopedAStatus onNewMessages(RadioIndicationType type,
                                             const std::vector<std::string>& messages) override;

    virtual ndk::ScopedAStatus onMessagesTransferComplete(RadioIndicationType type,
                                                          bool complete) override;

    virtual ndk::ScopedAStatus onSatellitePointingInfoChanged(
            RadioIndicationType type, const PointingInfo& pointingInfo) override;

    virtual ndk::ScopedAStatus onSatelliteModeChanged(RadioIndicationType type,
                                                      SatelliteMode mode) override;

    virtual ndk::ScopedAStatus onSatelliteRadioTechnologyChanged(
            RadioIndicationType type, satellite::NTRadioTechnology technology) override;

    virtual ndk::ScopedAStatus onProvisionStateChanged(
            RadioIndicationType type, bool provisioned,
            const std::vector<SatelliteFeature>& features) override;
};

// The main test class for AIDL Satellite.
class RadioSatelliteTest : public ::testing::TestWithParam<std::string>, public RadioServiceTest {
  public:
    virtual void SetUp() override;

    /* Radio Satellite service handle */
    std::shared_ptr<IRadioSatellite> satellite;
    /* Radio Satellite response handle */
    std::shared_ptr<RadioSatelliteResponse> rsp_satellite;
    /* Radio Satellite indication handle */
    std::shared_ptr<RadioSatelliteIndication> ind_satellite;
};
