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

#include "radio_network_utils.h"

RadioNetworkResponse::RadioNetworkResponse(RadioResponseWaiter& parent) : parent_network(parent) {}

ndk::ScopedAStatus RadioNetworkResponse::acknowledgeRequest(int32_t /*serial*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getAllowedNetworkTypesBitmapResponse(
        const RadioResponseInfo& info, const RadioAccessFamily networkTypeBitmap) {
    rspInfo = info;
    networkTypeBitmapResponse = networkTypeBitmap;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getAvailableBandModesResponse(
        const RadioResponseInfo& /*info*/, const std::vector<RadioBandMode>& /*bandModes*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getAvailableNetworksResponse(
        const RadioResponseInfo& info, const std::vector<OperatorInfo>& operatorInfos) {
    rspInfo = info;
    networkInfos = operatorInfos;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getBarringInfoResponse(
        const RadioResponseInfo& /*info*/, const CellIdentity& /*cellIdentity*/,
        const std::vector<BarringInfo>& /*barringInfos*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getCdmaRoamingPreferenceResponse(
        const RadioResponseInfo& /*info*/, CdmaRoamingType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getCellInfoListResponse(
        const RadioResponseInfo& /*info*/, const std::vector<CellInfo>& /*cellInfo*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getDataRegistrationStateResponse(
        const RadioResponseInfo& info, const RegStateResult& /*regResponse*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getImsRegistrationStateResponse(
        const RadioResponseInfo& /*info*/, bool /*isRegistered*/,
        RadioTechnologyFamily /*ratFamily*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getNetworkSelectionModeResponse(
        const RadioResponseInfo& /*info*/, bool /*manual*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getOperatorResponse(const RadioResponseInfo& /*info*/,
                                                             const std::string& /*longName*/,
                                                             const std::string& /*shortName*/,
                                                             const std::string& /*numeric*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getSignalStrengthResponse(
        const RadioResponseInfo& /*info*/, const SignalStrength& /*sig_strength*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getSystemSelectionChannelsResponse(
        const RadioResponseInfo& info, const std::vector<RadioAccessSpecifier>& /*specifier*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getUsageSettingResponse(const RadioResponseInfo& info,
                                                                 const UsageSetting usageSetting) {
    rspInfo = info;
    this->usageSetting = usageSetting;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getVoiceRadioTechnologyResponse(
        const RadioResponseInfo& /*info*/, RadioTechnology /*rat*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getVoiceRegistrationStateResponse(
        const RadioResponseInfo& info, const RegStateResult& regResponse) {
    rspInfo = info;
    regStateResp.regState = regResponse.regState;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::isNrDualConnectivityEnabledResponse(
        const RadioResponseInfo& info, bool isEnabled) {
    rspInfo = info;
    isNrDualConnectivityEnabled = isEnabled;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setAllowedNetworkTypesBitmapResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setBandModeResponse(const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setBarringPasswordResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setCdmaRoamingPreferenceResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setCellInfoListRateResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setIndicationFilterResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setLinkCapacityReportingCriteriaResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setLocationUpdatesResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setNetworkSelectionModeAutomaticResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setNetworkSelectionModeManualResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setNrDualConnectivityStateResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setSignalStrengthReportingCriteriaResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setSuppServiceNotificationsResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setSystemSelectionChannelsResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setUsageSettingResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::startNetworkScanResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::stopNetworkScanResponse(
        const RadioResponseInfo& /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::supplyNetworkDepersonalizationResponse(
        const RadioResponseInfo& /*info*/, int32_t /*remainingRetries*/) {
    return ndk::ScopedAStatus::ok();
}
