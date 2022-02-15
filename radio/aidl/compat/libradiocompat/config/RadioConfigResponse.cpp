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

#include <libradiocompat/RadioConfigResponse.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "ConfigResponse"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::config;

void RadioConfigResponse::setResponseFunction(
        std::shared_ptr<aidl::IRadioConfigResponse> callback) {
    mCallback = callback;
}

std::shared_ptr<aidl::IRadioConfigResponse> RadioConfigResponse::respond() {
    return mCallback.get();
}

Return<void> RadioConfigResponse::getSimSlotsStatusResponse(
        const V1_0::RadioResponseInfo& info,
        const hidl_vec<config::V1_0::SimSlotStatus>& slotStatus) {
    LOG_CALL << info.serial;
    respond()->getSimSlotsStatusResponse(toAidl(info), toAidl(slotStatus));
    return {};
};

Return<void> RadioConfigResponse::getSimSlotsStatusResponse_1_2(
        const V1_0::RadioResponseInfo& info,
        const hidl_vec<config::V1_2::SimSlotStatus>& slotStatus) {
    LOG_CALL << info.serial;
    respond()->getSimSlotsStatusResponse(toAidl(info), toAidl(slotStatus));
    return {};
};

Return<void> RadioConfigResponse::setSimSlotsMappingResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    respond()->setSimSlotsMappingResponse(toAidl(info));
    return {};
};

Return<void> RadioConfigResponse::getPhoneCapabilityResponse(
        const V1_0::RadioResponseInfo& info, const config::V1_1::PhoneCapability& phoneCapability) {
    LOG_CALL << info.serial;
    respond()->getPhoneCapabilityResponse(toAidl(info), toAidl(phoneCapability));
    return {};
};

Return<void> RadioConfigResponse::setPreferredDataModemResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    respond()->setPreferredDataModemResponse(toAidl(info));
    return {};
};

Return<void> RadioConfigResponse::setModemsConfigResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    respond()->setNumOfLiveModemsResponse(toAidl(info));
    return {};
};

Return<void> RadioConfigResponse::getModemsConfigResponse(
        const V1_0::RadioResponseInfo& info, const config::V1_1::ModemsConfig& modemsConfig) {
    LOG_CALL << info.serial;
    respond()->getNumOfLiveModemsResponse(toAidl(info), modemsConfig.numOfLiveModems);
    return {};
};

Return<void> RadioConfigResponse::getHalDeviceCapabilitiesResponse(
        const V1_6::RadioResponseInfo& info, bool modemReducedFeatureSet1) {
    LOG_CALL << info.serial;
    respond()->getHalDeviceCapabilitiesResponse(toAidl(info), modemReducedFeatureSet1);
    return {};
};

}  // namespace android::hardware::radio::compat
