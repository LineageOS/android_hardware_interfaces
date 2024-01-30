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

RadioNetworkResponse::RadioNetworkResponse(RadioServiceTest& parent) : parent_network(parent) {}

ndk::ScopedAStatus RadioNetworkResponse::acknowledgeRequest(int32_t /*serial*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getAllowedNetworkTypesBitmapResponse(
        const RadioResponseInfo& info, const int32_t networkTypeBitmap) {
    rspInfo = info;
    networkTypeBitmapResponse = networkTypeBitmap;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getAvailableBandModesResponse(
        const RadioResponseInfo& info, const std::vector<RadioBandMode>& bandModes) {
    rspInfo = info;
    radioBandModes = bandModes;
    parent_network.notify(info.serial);
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
        const RadioResponseInfo& info, const CellIdentity& cellIdentity,
        const std::vector<BarringInfo>& barringInfos) {
    rspInfo = info;
    barringCellIdentity = cellIdentity;
    barringInfoList = barringInfos;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getCdmaRoamingPreferenceResponse(
        const RadioResponseInfo& info, CdmaRoamingType /*type*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getCellInfoListResponse(
        const RadioResponseInfo& info, const std::vector<CellInfo>& /*cellInfo*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getDataRegistrationStateResponse(
        const RadioResponseInfo& info, const RegStateResult& regResponse) {
    rspInfo = info;
    dataRegResp = regResponse;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getImsRegistrationStateResponse(
        const RadioResponseInfo& info, bool /*isRegistered*/, RadioTechnologyFamily /*ratFamily*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getNetworkSelectionModeResponse(
        const RadioResponseInfo& info, bool /*manual*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getOperatorResponse(const RadioResponseInfo& info,
                                                             const std::string& /*longName*/,
                                                             const std::string& /*shortName*/,
                                                             const std::string& /*numeric*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getSignalStrengthResponse(
        const RadioResponseInfo& info, const SignalStrength& /*sig_strength*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getSystemSelectionChannelsResponse(
        const RadioResponseInfo& info, const std::vector<RadioAccessSpecifier>& specifiers) {
    rspInfo = info;
    this->specifiers = specifiers;
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
        const RadioResponseInfo& info, RadioTechnology /*rat*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::getVoiceRegistrationStateResponse(
        const RadioResponseInfo& info, const RegStateResult& regResponse) {
    rspInfo = info;
    voiceRegResp = regResponse;
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

ndk::ScopedAStatus RadioNetworkResponse::setBandModeResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setBarringPasswordResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setCdmaRoamingPreferenceResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setCellInfoListRateResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setIndicationFilterResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setLinkCapacityReportingCriteriaResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setLocationUpdatesResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setNetworkSelectionModeAutomaticResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setNetworkSelectionModeManualResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setNrDualConnectivityStateResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setSignalStrengthReportingCriteriaResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setSuppServiceNotificationsResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setSystemSelectionChannelsResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setUsageSettingResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::startNetworkScanResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::stopNetworkScanResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::supplyNetworkDepersonalizationResponse(
        const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setEmergencyModeResponse(
        const RadioResponseInfo& info, const EmergencyRegResult& /*regState*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::triggerEmergencyNetworkScanResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::exitEmergencyModeResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::cancelEmergencyNetworkScanResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setNullCipherAndIntegrityEnabledResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::isNullCipherAndIntegrityEnabledResponse(
        const RadioResponseInfo& info, bool /*isEnabled*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::isN1ModeEnabledResponse(
        const RadioResponseInfo& info, bool /*isEnabled*/) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setN1ModeEnabledResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setCellularIdentifierTransparencyEnabledResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::isCellularIdentifierTransparencyEnabledResponse(
        const RadioResponseInfo& info, bool enabled) {
    rspInfo = info;
    this->isCellularIdentifierTransparencyEnabled = enabled;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::setSecurityAlgorithmsUpdatedEnabledResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkResponse::isSecurityAlgorithmsUpdatedEnabledResponse(
        const RadioResponseInfo& info, bool enabled) {
    rspInfo = info;
    this->isSecurityAlgorithmsUpdatedEnabled = enabled;
    parent_network.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}
