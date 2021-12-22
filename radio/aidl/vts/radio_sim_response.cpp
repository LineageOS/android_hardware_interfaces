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

#include "radio_sim_utils.h"

RadioSimResponse::RadioSimResponse(RadioResponseWaiter& parent) : parent_sim(parent) {}

ndk::ScopedAStatus RadioSimResponse::acknowledgeRequest(int32_t /*serial*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::areUiccApplicationsEnabledResponse(
        const RadioResponseInfo& /*info*/, bool /*enabled*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::changeIccPin2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                                 int32_t /*remainingRetries*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::changeIccPinForAppResponse(const RadioResponseInfo& /*info*/,
                                                                int32_t /*remainingRetries*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::enableUiccApplicationsResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::getAllowedCarriersResponse(
        const RadioResponseInfo& /*info*/, const CarrierRestrictions& /*carriers*/,
        SimLockMultiSimPolicy /*multiSimPolicy*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::getCdmaSubscriptionResponse(
        const RadioResponseInfo& /*info*/, const std::string& /*mdn*/, const std::string& /*hSid*/,
        const std::string& /*hNid*/, const std::string& /*min*/, const std::string& /*prl*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::getCdmaSubscriptionSourceResponse(
        const RadioResponseInfo& /*info*/, CdmaSubscriptionSource /*source*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::getFacilityLockForAppResponse(
        const RadioResponseInfo& /*info*/, int32_t /*response*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::getIccCardStatusResponse(const RadioResponseInfo& info,
                                                              const CardStatus& card_status) {
    rspInfo = info;
    cardStatus = card_status;
    parent_sim.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::getImsiForAppResponse(const RadioResponseInfo& /*info*/,
                                                           const std::string& /*imsi*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::getSimPhonebookCapacityResponse(
        const RadioResponseInfo& info, const PhonebookCapacity& pbCapacity) {
    rspInfo = info;
    capacity = pbCapacity;
    parent_sim.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::getSimPhonebookRecordsResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_sim.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::iccCloseLogicalChannelResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::iccIoForAppResponse(const RadioResponseInfo& /*info*/,
                                                         const IccIoResult& /*iccIo*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::iccOpenLogicalChannelResponse(
        const RadioResponseInfo& /*info*/, int32_t /*channelId*/,
        const std::vector<uint8_t>& /*selectResponse*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::iccTransmitApduBasicChannelResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::iccTransmitApduLogicalChannelResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::reportStkServiceIsRunningResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::requestIccSimAuthenticationResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::sendEnvelopeResponse(const RadioResponseInfo& /*info*/,
                                                          const std::string& /*commandResponse*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::sendEnvelopeWithStatusResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*iccIo*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::sendTerminalResponseToSimResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::setAllowedCarriersResponse(const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::setCarrierInfoForImsiEncryptionResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_sim.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::setCdmaSubscriptionSourceResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::setFacilityLockForAppResponse(
        const RadioResponseInfo& /*info*/, int32_t /*retry*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::setSimCardPowerResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_sim.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::setUiccSubscriptionResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::supplyIccPin2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                                 int32_t /*remainingRetries*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::supplyIccPinForAppResponse(const RadioResponseInfo& /*info*/,
                                                                int32_t /*remainingRetries*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::supplyIccPuk2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                                 int32_t /*remainingRetries*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::supplyIccPukForAppResponse(const RadioResponseInfo& /*info*/,
                                                                int32_t /*remainingRetries*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::supplySimDepersonalizationResponse(
        const RadioResponseInfo& /*info*/, PersoSubstate /*persoType*/,
        int32_t /*remainingRetries*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimResponse::updateSimPhonebookRecordsResponse(
        const RadioResponseInfo& info, int32_t recordIndex) {
    rspInfo = info;
    updatedRecordIndex = recordIndex;
    parent_sim.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}
