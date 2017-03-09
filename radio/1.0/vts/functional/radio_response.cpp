/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include<radio_hidl_hal_utils.h>

CardStatus cardStatus;

RadioResponse::RadioResponse(RadioHidlTest& parent) : parent(parent) {
}

Return<void> RadioResponse::getIccCardStatusResponse(
        const RadioResponseInfo& info, const CardStatus& card_status) {
    rspInfo = info;
    cardStatus = card_status;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::supplyIccPinForAppResponse(
        const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::supplyIccPukForAppResponse(
        const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::supplyIccPin2ForAppResponse(
        const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
  rspInfo = info;
  parent.notify();
  return Void();
}

Return<void> RadioResponse::supplyIccPuk2ForAppResponse(
        const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::changeIccPinForAppResponse(
        const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::changeIccPin2ForAppResponse(
        const RadioResponseInfo& info, int32_t /*remainingRetries*/) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::supplyNetworkDepersonalizationResponse(
        const RadioResponseInfo& /*info*/, int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse::getCurrentCallsResponse(
        const RadioResponseInfo& info, const ::android::hardware::hidl_vec<Call>& /*calls*/) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::dialResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::getIMSIForAppResponse(
        const RadioResponseInfo& info, const ::android::hardware::hidl_string& imsi) {
    rspInfo = info;
    this->imsi = imsi;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::hangupConnectionResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::hangupWaitingOrBackgroundResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::hangupForegroundResumeBackgroundResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::switchWaitingOrHoldingAndActiveResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::conferenceResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::rejectCallResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::getLastCallFailCauseResponse(
        const RadioResponseInfo& info, const LastCallFailCauseInfo& /*failCauseInfo*/) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::getSignalStrengthResponse(
        const RadioResponseInfo& /*info*/, const SignalStrength& /*sig_strength*/) {
    return Void();
}

Return<void> RadioResponse::getVoiceRegistrationStateResponse(
        const RadioResponseInfo& /*info*/, const VoiceRegStateResult& /*voiceRegResponse*/) {
    return Void();
}

Return<void> RadioResponse::getDataRegistrationStateResponse(
        const RadioResponseInfo& /*info*/, const DataRegStateResult& /*dataRegResponse*/) {
    return Void();
}

Return<void> RadioResponse::getOperatorResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*longName*/,
        const ::android::hardware::hidl_string& /*shortName*/,
        const ::android::hardware::hidl_string& /*numeric*/) {
    return Void();
}

Return<void> RadioResponse::setRadioPowerResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::sendDtmfResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::sendSmsResponse(const RadioResponseInfo& info,
        const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::sendSMSExpectMoreResponse(
        const RadioResponseInfo& info, const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::setupDataCallResponse(
        const RadioResponseInfo& /*info*/, const SetupDataCallResult& /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse::iccIOForAppResponse(
        const RadioResponseInfo& info, const IccIoResult& iccIo) {
    rspInfo = info;
    this->iccIoResult = iccIo;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::sendUssdResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::cancelPendingUssdResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::getClirResponse(const RadioResponseInfo& /*info*/, int32_t /*n*/,
        int32_t /*m*/) {
    return Void();
}

Return<void> RadioResponse::setClirResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getCallForwardStatusResponse(
        const RadioResponseInfo& info, const ::android::hardware::hidl_vec<CallForwardInfo>&
        /*callForwardInfos*/) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::setCallForwardResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::getCallWaitingResponse(
        const RadioResponseInfo& info, bool /*enable*/, int32_t /*serviceClass*/) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::setCallWaitingResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::acknowledgeLastIncomingGsmSmsResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::acceptCallResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::deactivateDataCallResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getFacilityLockForAppResponse(const RadioResponseInfo& /*info*/,
        int32_t /*response*/) {
    return Void();
}

Return<void> RadioResponse::setFacilityLockForAppResponse(const RadioResponseInfo& /*info*/,
        int32_t /*retry*/) {
    return Void();
}

Return<void> RadioResponse::setBarringPasswordResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getNetworkSelectionModeResponse(
        const RadioResponseInfo& /*info*/, bool /*manual*/) {
    return Void();
}

Return<void> RadioResponse::setNetworkSelectionModeAutomaticResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::setNetworkSelectionModeManualResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getAvailableNetworksResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<OperatorInfo>& /*networkInfos*/) {
    return Void();
}

Return<void> RadioResponse::startDtmfResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::stopDtmfResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::getBasebandVersionResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*version*/) {
    return Void();
}

Return<void> RadioResponse::separateConnectionResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::setMuteResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::getMuteResponse(const RadioResponseInfo& info, bool /*enable*/) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::getClipResponse(const RadioResponseInfo& /*info*/,
        ClipStatus /*status*/) {
    return Void();
}

Return<void> RadioResponse::getDataCallListResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<SetupDataCallResult>& /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse::sendOemRilRequestRawResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<uint8_t>& /*data*/) {
    return Void();
}

Return<void> RadioResponse::sendOemRilRequestStringsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& /*data*/) {
    return Void();
}

Return<void> RadioResponse::setSuppServiceNotificationsResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::writeSmsToSimResponse(
        const RadioResponseInfo& info, int32_t index) {
    rspInfo = info;
    writeSmsToSimIndex = index;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::deleteSmsOnSimResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::setBandModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getAvailableBandModesResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<RadioBandMode>& /*bandModes*/) {
    return Void();
}

Return<void> RadioResponse::sendEnvelopeResponse(const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*commandResponse*/) {
    return Void();
}

Return<void> RadioResponse::sendTerminalResponseToSimResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::handleStkCallSetupRequestFromSimResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::explicitCallTransferResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::setPreferredNetworkTypeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getPreferredNetworkTypeResponse(
        const RadioResponseInfo& /*info*/, PreferredNetworkType /*nw_type*/) {
    return Void();
}

Return<void> RadioResponse::getNeighboringCidsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<NeighboringCell>& /*cells*/) {
    return Void();
}

Return<void> RadioResponse::setLocationUpdatesResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::setCdmaSubscriptionSourceResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::setCdmaRoamingPreferenceResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getCdmaRoamingPreferenceResponse(
        const RadioResponseInfo& /*info*/, CdmaRoamingType /*type*/) {
    return Void();
}

Return<void> RadioResponse::setTTYModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getTTYModeResponse(const RadioResponseInfo& /*info*/,
        TtyMode /*mode*/) {
    return Void();
}

Return<void> RadioResponse::setPreferredVoicePrivacyResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getPreferredVoicePrivacyResponse(
        const RadioResponseInfo& /*info*/, bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse::sendCDMAFeatureCodeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::sendBurstDtmfResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::sendCdmaSmsResponse(
        const RadioResponseInfo& info, const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::acknowledgeLastIncomingCdmaSmsResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::getGsmBroadcastConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse::setGsmBroadcastConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::setGsmBroadcastActivationResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getCdmaBroadcastConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse::setCdmaBroadcastConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::setCdmaBroadcastActivationResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getCDMASubscriptionResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*mdn*/,
        const ::android::hardware::hidl_string& /*hSid*/,
        const ::android::hardware::hidl_string& /*hNid*/,
        const ::android::hardware::hidl_string& /*min*/,
        const ::android::hardware::hidl_string& /*prl*/) {
    return Void();
}

Return<void> RadioResponse::writeSmsToRuimResponse(
        const RadioResponseInfo& info, uint32_t index) {
    rspInfo = info;
    writeSmsToRuimIndex = index;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::deleteSmsOnRuimResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::getDeviceIdentityResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*imei*/,
        const ::android::hardware::hidl_string& /*imeisv*/,
        const ::android::hardware::hidl_string& /*esn*/,
        const ::android::hardware::hidl_string& /*meid*/) {
    return Void();
}

Return<void> RadioResponse::exitEmergencyCallbackModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getSmscAddressResponse(
        const RadioResponseInfo& info, const ::android::hardware::hidl_string& smsc) {
    rspInfo = info;
    smscAddress = smsc;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::setSmscAddressResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::reportSmsMemoryStatusResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::reportStkServiceIsRunningResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getCdmaSubscriptionSourceResponse(
        const RadioResponseInfo& /*info*/, CdmaSubscriptionSource /*source*/) {
    return Void();
}

Return<void> RadioResponse::requestIsimAuthenticationResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*response*/) {
    return Void();
}

Return<void> RadioResponse::acknowledgeIncomingGsmSmsWithPduResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::sendEnvelopeWithStatusResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse::getVoiceRadioTechnologyResponse(
        const RadioResponseInfo& /*info*/, RadioTechnology /*rat*/) {
    return Void();
}

Return<void> RadioResponse::getCellInfoListResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse::setCellInfoListRateResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::setInitialAttachApnResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getImsRegistrationStateResponse(
        const RadioResponseInfo& /*info*/, bool /*isRegistered*/,
        RadioTechnologyFamily /*ratFamily*/) {
    return Void();
}

Return<void> RadioResponse::sendImsSmsResponse(
        const RadioResponseInfo& info, const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::iccTransmitApduBasicChannelResponse(
        const RadioResponseInfo& info, const IccIoResult& result) {
    rspInfo = info;
    this->iccIoResult = result;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::iccOpenLogicalChannelResponse(
        const RadioResponseInfo& info, int32_t channelId,
        const ::android::hardware::hidl_vec<int8_t>& /*selectResponse*/) {
    rspInfo = info;
    this->channelId = channelId;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::iccCloseLogicalChannelResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::iccTransmitApduLogicalChannelResponse(
        const RadioResponseInfo& info, const IccIoResult& result) {
    rspInfo = info;
    this->iccIoResult = result;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::nvReadItemResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*result*/) {
    return Void();
}

Return<void> RadioResponse::nvWriteItemResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::nvWriteCdmaPrlResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::nvResetConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::setUiccSubscriptionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::setDataAllowedResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getHardwareConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<HardwareConfig>& /*config*/) {
    return Void();
}

Return<void> RadioResponse::requestIccSimAuthenticationResponse(
        const RadioResponseInfo& info, const IccIoResult& result) {
    rspInfo = info;
    this->iccIoResult = result;
    parent.notify();
    return Void();
}

Return<void> RadioResponse::setDataProfileResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::requestShutdownResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::getRadioCapabilityResponse(
        const RadioResponseInfo& /*info*/, const RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse::setRadioCapabilityResponse(
        const RadioResponseInfo& /*info*/, const RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse::startLceServiceResponse(
        const RadioResponseInfo& /*info*/, const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse::stopLceServiceResponse(
        const RadioResponseInfo& /*info*/, const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse::pullLceDataResponse(
        const RadioResponseInfo& /*info*/, const LceDataInfo& /*lceInfo*/) {
    return Void();
}

Return<void> RadioResponse::getModemActivityInfoResponse(
        const RadioResponseInfo& /*info*/, const ActivityStatsInfo& /*activityInfo*/) {
    return Void();
}

Return<void> RadioResponse::setAllowedCarriersResponse(
        const RadioResponseInfo& /*info*/, int32_t /*numAllowed*/) {
    return Void();
}

Return<void> RadioResponse::getAllowedCarriersResponse(
        const RadioResponseInfo& /*info*/, bool /*allAllowed*/,
        const CarrierRestrictions& /*carriers*/) {
    return Void();
}

Return<void> RadioResponse::sendDeviceStateResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::setIndicationFilterResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::setSimCardPowerResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse::acknowledgeRequest(int32_t /*serial*/) {
    return Void();
}
