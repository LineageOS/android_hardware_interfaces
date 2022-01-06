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

#include "radio_config_utils.h"

RadioConfigResponse::RadioConfigResponse(RadioServiceTest& parent) : parent_config(parent) {}

ndk::ScopedAStatus RadioConfigResponse::getSimSlotsStatusResponse(
        const RadioResponseInfo& /* info */, const std::vector<SimSlotStatus>& /* slotStatus */) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioConfigResponse::setSimSlotsMappingResponse(
        const RadioResponseInfo& /* info */) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioConfigResponse::getPhoneCapabilityResponse(
        const RadioResponseInfo& info, const PhoneCapability& phoneCapability) {
    rspInfo = info;
    phoneCap = phoneCapability;
    parent_config.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioConfigResponse::setPreferredDataModemResponse(
        const RadioResponseInfo& /* info */) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioConfigResponse::getNumOfLiveModemsResponse(
        const RadioResponseInfo& /* info */, const int8_t /* numOfLiveModems */) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioConfigResponse::setNumOfLiveModemsResponse(
        const RadioResponseInfo& /* info */) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioConfigResponse::getHalDeviceCapabilitiesResponse(
        const RadioResponseInfo& info, bool modemReducedFeatures) {
    modemReducedFeatureSet1 = modemReducedFeatures;
    parent_config.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}
