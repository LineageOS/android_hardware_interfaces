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

#include "radio_modem_utils.h"

RadioModemResponse::RadioModemResponse(RadioServiceTest& parent) : parent_modem(parent) {}

ndk::ScopedAStatus RadioModemResponse::acknowledgeRequest(int32_t /*serial*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::enableModemResponse(const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getBasebandVersionResponse(const RadioResponseInfo& /*info*/,
                                                                  const std::string& /*version*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getDeviceIdentityResponse(const RadioResponseInfo& /*info*/,
                                                                 const std::string& /*imei*/,
                                                                 const std::string& /*imeisv*/,
                                                                 const std::string& /*esn*/,
                                                                 const std::string& /*meid*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getHardwareConfigResponse(
        const RadioResponseInfo& /*info*/, const std::vector<HardwareConfig>& /*config*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getModemActivityInfoResponse(
        const RadioResponseInfo& /*info*/, const ActivityStatsInfo& /*activityInfo*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getModemStackStatusResponse(
        const RadioResponseInfo& /*info*/, const bool /*enabled*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getRadioCapabilityResponse(const RadioResponseInfo& /*info*/,
                                                                  const RadioCapability& /*rc*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::nvReadItemResponse(const RadioResponseInfo& /*info*/,
                                                          const std::string& /*result*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::nvResetConfigResponse(const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::nvWriteCdmaPrlResponse(const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::nvWriteItemResponse(const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::requestShutdownResponse(const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::sendDeviceStateResponse(const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::setRadioCapabilityResponse(const RadioResponseInfo& /*info*/,
                                                                  const RadioCapability& /*rc*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::setRadioPowerResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}
