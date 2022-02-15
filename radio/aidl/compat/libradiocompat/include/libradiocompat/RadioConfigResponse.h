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

#include "GuaranteedCallback.h"

#include <aidl/android/hardware/radio/config/IRadioConfigResponse.h>
#include <android/hardware/radio/config/1.3/IRadioConfigResponse.h>

namespace android::hardware::radio::compat {

class RadioConfigResponse : public config::V1_3::IRadioConfigResponse {
    GuaranteedCallback<aidl::android::hardware::radio::config::IRadioConfigResponse,
                       aidl::android::hardware::radio::config::IRadioConfigResponseDefault>
            mCallback;

    Return<void> getSimSlotsStatusResponse(
            const V1_0::RadioResponseInfo& info,
            const hidl_vec<config::V1_0::SimSlotStatus>& slotStatus) override;
    Return<void> setSimSlotsMappingResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getPhoneCapabilityResponse(
            const V1_0::RadioResponseInfo& info,
            const config::V1_1::PhoneCapability& phoneCapability) override;
    Return<void> setPreferredDataModemResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setModemsConfigResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getModemsConfigResponse(const V1_0::RadioResponseInfo& info,
                                         const config::V1_1::ModemsConfig& modemsConfig) override;
    Return<void> getSimSlotsStatusResponse_1_2(
            const V1_0::RadioResponseInfo& info,
            const hidl_vec<config::V1_2::SimSlotStatus>& slotStatus) override;
    Return<void> getHalDeviceCapabilitiesResponse(const V1_6::RadioResponseInfo& info,
                                                  bool modemReducedFeatureSet1) override;

  public:
    void setResponseFunction(
            std::shared_ptr<aidl::android::hardware::radio::config::IRadioConfigResponse> callback);

    std::shared_ptr<aidl::android::hardware::radio::config::IRadioConfigResponse> respond();
};

}  // namespace android::hardware::radio::compat
