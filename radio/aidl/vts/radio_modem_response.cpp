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

ndk::ScopedAStatus RadioModemResponse::enableModemResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    enableModemResponseToggle = !enableModemResponseToggle;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getBasebandVersionResponse(const RadioResponseInfo& info,
                                                                  const std::string& /*version*/) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getDeviceIdentityResponse(const RadioResponseInfo& info,
                                                                 const std::string& /*imei*/,
                                                                 const std::string& /*imeisv*/,
                                                                 const std::string& /*esn*/,
                                                                 const std::string& /*meid*/) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getImeiResponse(const RadioResponseInfo& info,
                   const std::optional<ImeiInfo>& /*imeiInfo*/) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getHardwareConfigResponse(
        const RadioResponseInfo& info, const std::vector<HardwareConfig>& /*config*/) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getModemActivityInfoResponse(
        const RadioResponseInfo& info, const ActivityStatsInfo& /*activityInfo*/) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getModemStackStatusResponse(const RadioResponseInfo& info,
                                                                   const bool enabled) {
    rspInfo = info;
    isModemEnabled = enabled;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::getRadioCapabilityResponse(const RadioResponseInfo& info,
                                                                  const RadioCapability& /*rc*/) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::nvReadItemResponse(const RadioResponseInfo& info,
                                                          const std::string& /*result*/) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::nvResetConfigResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::nvWriteCdmaPrlResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::nvWriteItemResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::requestShutdownResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::sendDeviceStateResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::setRadioCapabilityResponse(const RadioResponseInfo& info,
                                                                  const RadioCapability& /*rc*/) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemResponse::setRadioPowerResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_modem.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}
