/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_6.h>

::android::hardware::radio::V1_5::CardStatus cardStatus;

RadioResponse_v1_6::RadioResponse_v1_6(RadioResponseWaiter& parent) : parent_v1_6(parent) {}

/* 1.0 Apis */
Return<void> RadioResponse_v1_6::getIccCardStatusResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::CardStatus& /*card_status*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplyIccPinForAppResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplyIccPukForAppResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplyIccPin2ForAppResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplyIccPuk2ForAppResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::changeIccPinForAppResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::changeIccPin2ForAppResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplyNetworkDepersonalizationResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCurrentCallsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::Call>& /*calls*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::dialResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getIMSIForAppResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*imsi*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::hangupConnectionResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& info) {
    rspInfo_v1_0 = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::hangupWaitingOrBackgroundResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::hangupForegroundResumeBackgroundResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::switchWaitingOrHoldingAndActiveResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::conferenceResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::rejectCallResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getLastCallFailCauseResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const LastCallFailCauseInfo& /*failCauseInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getSignalStrengthResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::SignalStrength& /*sig_strength*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getVoiceRegistrationStateResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::VoiceRegStateResult& /*voiceRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getDataRegistrationStateResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::DataRegStateResult& /*dataRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getOperatorResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*longName*/,
        const ::android::hardware::hidl_string& /*shortName*/,
        const ::android::hardware::hidl_string& /*numeric*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setRadioPowerResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendDtmfResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendSmsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendSMSExpectMoreResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setupDataCallResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_0::SetupDataCallResult& /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::iccIOForAppResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendUssdResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::cancelPendingUssdResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getClirResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, int32_t /*n*/,
        int32_t /*m*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setClirResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCallForwardStatusResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<CallForwardInfo>&
        /*callForwardInfos*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCallForwardResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCallWaitingResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, bool /*enable*/,
        int32_t /*serviceClass*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCallWaitingResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::acknowledgeLastIncomingGsmSmsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::acceptCallResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::deactivateDataCallResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getFacilityLockForAppResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, int32_t /*response*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setFacilityLockForAppResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, int32_t /*retry*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setBarringPasswordResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getNetworkSelectionModeResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, bool /*manual*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setNetworkSelectionModeAutomaticResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setNetworkSelectionModeManualResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getAvailableNetworksResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<OperatorInfo>& networkInfos) {
    rspInfo_v1_0 = info;
    this->networkInfos = networkInfos;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::startDtmfResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::stopDtmfResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getBasebandVersionResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*version*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::separateConnectionResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setMuteResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getMuteResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getClipResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        ClipStatus /*status*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getDataCallListResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<android::hardware::radio::V1_0::SetupDataCallResult>&
        /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendOemRilRequestRawResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<uint8_t>& /*data*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendOemRilRequestStringsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& /*data*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setSuppServiceNotificationsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::writeSmsToSimResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, int32_t /*index*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::deleteSmsOnSimResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setBandModeResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getAvailableBandModesResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<RadioBandMode>& /*bandModes*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendEnvelopeResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*commandResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendTerminalResponseToSimResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::handleStkCallSetupRequestFromSimResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::explicitCallTransferResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setPreferredNetworkTypeResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getPreferredNetworkTypeResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        PreferredNetworkType /*nw_type*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getNeighboringCidsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<NeighboringCell>& /*cells*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setLocationUpdatesResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCdmaSubscriptionSourceResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCdmaRoamingPreferenceResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCdmaRoamingPreferenceResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        CdmaRoamingType /*type*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setTTYModeResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getTTYModeResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, TtyMode /*mode*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setPreferredVoicePrivacyResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getPreferredVoicePrivacyResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendCDMAFeatureCodeResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendBurstDtmfResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendCdmaSmsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::acknowledgeLastIncomingCdmaSmsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getGsmBroadcastConfigResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setGsmBroadcastConfigResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setGsmBroadcastActivationResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCdmaBroadcastConfigResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCdmaBroadcastConfigResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCdmaBroadcastActivationResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCDMASubscriptionResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*mdn*/,
        const ::android::hardware::hidl_string& /*hSid*/,
        const ::android::hardware::hidl_string& /*hNid*/,
        const ::android::hardware::hidl_string& /*min*/,
        const ::android::hardware::hidl_string& /*prl*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::writeSmsToRuimResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, uint32_t /*index*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::deleteSmsOnRuimResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getDeviceIdentityResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*imei*/,
        const ::android::hardware::hidl_string& /*imeisv*/,
        const ::android::hardware::hidl_string& /*esn*/,
        const ::android::hardware::hidl_string& /*meid*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::exitEmergencyCallbackModeResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getSmscAddressResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*smsc*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setSmscAddressResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::reportSmsMemoryStatusResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::reportStkServiceIsRunningResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCdmaSubscriptionSourceResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        CdmaSubscriptionSource /*source*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::requestIsimAuthenticationResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*response*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::acknowledgeIncomingGsmSmsWithPduResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendEnvelopeWithStatusResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getVoiceRadioTechnologyResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        ::android::hardware::radio::V1_0::RadioTechnology /*rat*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCellInfoListResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_0::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCellInfoListRateResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setInitialAttachApnResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getImsRegistrationStateResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, bool /*isRegistered*/,
        RadioTechnologyFamily /*ratFamily*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendImsSmsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::iccTransmitApduBasicChannelResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::iccOpenLogicalChannelResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, int32_t /*channelId*/,
        const ::android::hardware::hidl_vec<int8_t>& /*selectResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::iccCloseLogicalChannelResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::iccTransmitApduLogicalChannelResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::nvReadItemResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::nvWriteItemResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::nvWriteCdmaPrlResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::nvResetConfigResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setUiccSubscriptionResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setDataAllowedResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getHardwareConfigResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<HardwareConfig>& /*config*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::requestIccSimAuthenticationResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setDataProfileResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::requestShutdownResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getRadioCapabilityResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_0::RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setRadioCapabilityResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_0::RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::startLceServiceResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::stopLceServiceResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::pullLceDataResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const LceDataInfo& /*lceInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getModemActivityInfoResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ActivityStatsInfo& /*activityInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setAllowedCarriersResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        int32_t /*numAllowed*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getAllowedCarriersResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, bool /*allAllowed*/,
        const CarrierRestrictions& /*carriers*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendDeviceStateResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setIndicationFilterResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setSimCardPowerResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::acknowledgeRequest(int32_t /*serial*/) {
    return Void();
}

/* 1.1 Apis */
Return<void> RadioResponse_v1_6::setCarrierInfoForImsiEncryptionResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& info) {
    rspInfo_v1_0 = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setSimCardPowerResponse_1_1(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::startNetworkScanResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::stopNetworkScanResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::startKeepaliveResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const KeepaliveStatus& /*status*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::stopKeepaliveResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

/* 1.2 Apis */
Return<void> RadioResponse_v1_6::setSignalStrengthReportingCriteriaResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setLinkCapacityReportingCriteriaResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getIccCardStatusResponse_1_2(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_2::CardStatus& /*card_status*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCurrentCallsResponse_1_2(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::Call>& /*calls*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getSignalStrengthResponse_1_2(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_2::SignalStrength& /*sig_strength*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCellInfoListResponse_1_2(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_2::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getVoiceRegistrationStateResponse_1_2(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
        const ::android::hardware::radio::V1_2::VoiceRegStateResult& voiceRegResponse) {
    rspInfo_v1_0 = info;
    voiceRegResp = voiceRegResponse;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getDataRegistrationStateResponse_1_2(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_2::DataRegStateResult& /*dataRegResponse*/) {
    return Void();
}

/* 1.3 Apis */
Return<void> RadioResponse_v1_6::setSystemSelectionChannelsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::enableModemResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getModemStackStatusResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const bool /*enabled*/) {
    return Void();
}

/* 1.4 Apis */
Return<void> RadioResponse_v1_6::emergencyDialResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& info) {
    rspInfo_v1_0 = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::startNetworkScanResponse_1_4(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getDataRegistrationStateResponse_1_4(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_4::DataRegStateResult& /*dataRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getSignalStrengthResponse_1_4(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_4::SignalStrength& /*sig_strength*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCellInfoListResponse_1_4(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_4::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getIccCardStatusResponse_1_4(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_4::CardStatus& /*card_status*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getPreferredNetworkTypeBitmapResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_bitfield<
                ::android::hardware::radio::V1_4::RadioAccessFamily>
        /*networkTypeBitmap*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setPreferredNetworkTypeBitmapResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getDataCallListResponse_1_4(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_4::SetupDataCallResult>&
        /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setupDataCallResponse_1_4(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_4::SetupDataCallResult& /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setAllowedCarriersResponse_1_4(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getAllowedCarriersResponse_1_4(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const CarrierRestrictionsWithPriority& /*carriers*/,
        SimLockMultiSimPolicy /*multiSimPolicy*/) {
    return Void();
}

/* 1.5 Apis */
Return<void> RadioResponse_v1_6::setSignalStrengthReportingCriteriaResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setLinkCapacityReportingCriteriaResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::enableUiccApplicationsResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::areUiccApplicationsEnabledResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, bool /*enabled*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::canToggleUiccApplicationsEnablementResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/, bool /*canToggle*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setSystemSelectionChannelsResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::startNetworkScanResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setupDataCallResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_5::SetupDataCallResult& /* dcResponse */) {
    return Void();
}

Return<void> RadioResponse_v1_6::getDataCallListResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const hidl_vec<::android::hardware::radio::V1_5::SetupDataCallResult>& /* dcResponse */) {
    return Void();
}

Return<void> RadioResponse_v1_6::setInitialAttachApnResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setDataProfileResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setRadioPowerResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setIndicationFilterResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getBarringInfoResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_5::CellIdentity& /*cellIdentity*/,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_5::BarringInfo>&
        /*barringInfos*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getVoiceRegistrationStateResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_5::RegStateResult& /*regResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getDataRegistrationStateResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_5::RegStateResult& /*regResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCellInfoListResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_5::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setNetworkSelectionModeManualResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendCdmaSmsExpectMoreResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplySimDepersonalizationResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /*info*/,
        ::android::hardware::radio::V1_5::PersoSubstate /*persoType*/,
        int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getIccCardStatusResponse_1_5(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
        const ::android::hardware::radio::V1_5::CardStatus& card_status) {
    rspInfo_v1_0 = info;
    cardStatus = card_status;
    parent_v1_6.notify(info.serial);
    return Void();
}

/* 1.6 Apis */
Return<void> RadioResponse_v1_6::setRadioPowerResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setupDataCallResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const android::hardware::radio::V1_6::SetupDataCallResult& dcResponse) {
    rspInfo = info;
    setupDataCallResult = dcResponse;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setNrDualConnectivityStateResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getDataCallListResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const hidl_vec<::android::hardware::radio::V1_6::SetupDataCallResult>& /* dcResponse */) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::sendSmsResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::sendSmsExpectMoreResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::sendCdmaSmsResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setSimCardPowerResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::sendCdmaSmsExpectMoreResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::isNrDualConnectivityEnabledResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info, bool isEnabled) {
    rspInfo = info;
    isNRDualConnectivityEnabled = isEnabled;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::allocatePduSessionIdResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info, int32_t id) {
    rspInfo = info;
    allocatedPduSessionId = id;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::releasePduSessionIdResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::startHandoverResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::cancelHandoverResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setAllowedNetworkTypesBitmapResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getAllowedNetworkTypesBitmapResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const ::android::hardware::hidl_bitfield<
                ::android::hardware::radio::V1_4::RadioAccessFamily>
                networkTypeBitmap) {
    rspInfo = info;
    networkTypeBitmapResponse = networkTypeBitmap;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setDataThrottlingResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getSignalStrengthResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_6::SignalStrength& /*sig_strength*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCellInfoListResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_6::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getSystemSelectionChannelsResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const hidl_vec<::android::hardware::radio::V1_5::RadioAccessSpecifier>& /*specifier*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getVoiceRegistrationStateResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const ::android::hardware::radio::V1_6::RegStateResult& regResponse) {
    rspInfo = info;
    voiceRegResp.regState = regResponse.regState;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getDataRegistrationStateResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const ::android::hardware::radio::V1_6::RegStateResult& /*regResponse*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getCurrentCallsResponse_1_6(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_6::Call>& calls) {
    rspInfo = info;
    currentCalls = calls;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getSlicingConfigResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const ::android::hardware::radio::V1_6::SlicingConfig& /*slicingConfig*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getSimPhonebookRecordsResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getSimPhonebookCapacityResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        const ::android::hardware::radio::V1_6::PhonebookCapacity& capacity) {
    rspInfo = info;
    this->capacity = capacity;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::updateSimPhonebookRecordsResponse(
        const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
        int32_t updatedRecordIndex) {
    rspInfo = info;
    this->updatedRecordIndex = updatedRecordIndex;
    parent_v1_6.notify(info.serial);
    return Void();
}
