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

#include "radio_satellite_utils.h"

RadioSatelliteResponse::RadioSatelliteResponse(RadioServiceTest& parent)
    : parent_satellite(parent) {}

ndk::ScopedAStatus RadioSatelliteResponse::acknowledgeRequest(int32_t /*serial*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::getCapabilitiesResponse(
        const RadioResponseInfo& info, const SatelliteCapabilities& /*capabilities*/) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::setPowerResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::getPowerStateResponse(const RadioResponseInfo& info,
                                                                 bool /*on*/) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::provisionServiceResponse(const RadioResponseInfo& info,
                                                                    bool /*provisioned*/) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::addAllowedSatelliteContactsResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::removeAllowedSatelliteContactsResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::sendMessagesResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::getPendingMessagesResponse(
        const RadioResponseInfo& info, const std::vector<std::string>& /*messages*/) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::getSatelliteModeResponse(
        const RadioResponseInfo& info, SatelliteMode /*mode*/, NTRadioTechnology /*technology*/) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::setIndicationFilterResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::startSendingSatellitePointingInfoResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::stopSendingSatellitePointingInfoResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::getMaxCharactersPerTextMessageResponse(
        const RadioResponseInfo& info, int32_t /*charLimit*/) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteResponse::getTimeForNextSatelliteVisibilityResponse(
        const RadioResponseInfo& info, int32_t /*timeInSeconds*/) {
    rspInfo = info;
    parent_satellite.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}