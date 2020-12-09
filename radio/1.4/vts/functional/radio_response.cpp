/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_4.h>

::android::hardware::radio::V1_4::CardStatus cardStatus;

RadioResponse_v1_4::RadioResponse_v1_4(RadioHidlTest_v1_4& parent) : parent_v1_4(parent) {}

/* 1.0 Apis */
Return<void> RadioResponse_v1_4::getIccCardStatusResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::CardStatus& /*card_status*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::supplyIccPinForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::supplyIccPukForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::supplyIccPin2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::supplyIccPuk2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::changeIccPinForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::changeIccPin2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::supplyNetworkDepersonalizationResponse(
        const RadioResponseInfo& /*info*/, int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getCurrentCallsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::Call>& /*calls*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::dialResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getIMSIForAppResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*imsi*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::hangupConnectionResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::hangupWaitingOrBackgroundResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::hangupForegroundResumeBackgroundResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::switchWaitingOrHoldingAndActiveResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::conferenceResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::rejectCallResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getLastCallFailCauseResponse(
        const RadioResponseInfo& /*info*/, const LastCallFailCauseInfo& /*failCauseInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getSignalStrengthResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::SignalStrength& /*sig_strength*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getVoiceRegistrationStateResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::VoiceRegStateResult& /*voiceRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getDataRegistrationStateResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::DataRegStateResult& /*dataRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getOperatorResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*longName*/,
        const ::android::hardware::hidl_string& /*shortName*/,
        const ::android::hardware::hidl_string& /*numeric*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setRadioPowerResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendSmsResponse(const RadioResponseInfo& /*info*/,
                                                 const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendSMSExpectMoreResponse(const RadioResponseInfo& /*info*/,
                                                           const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setupDataCallResponse(
        const RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_0::SetupDataCallResult& /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::iccIOForAppResponse(const RadioResponseInfo& /*info*/,
                                                     const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendUssdResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::cancelPendingUssdResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getClirResponse(const RadioResponseInfo& /*info*/, int32_t /*n*/,
                                                 int32_t /*m*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setClirResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getCallForwardStatusResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_vec<CallForwardInfo>&
        /*callForwardInfos*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setCallForwardResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getCallWaitingResponse(const RadioResponseInfo& /*info*/,
                                                        bool /*enable*/, int32_t /*serviceClass*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setCallWaitingResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::acknowledgeLastIncomingGsmSmsResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::acceptCallResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::deactivateDataCallResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getFacilityLockForAppResponse(const RadioResponseInfo& /*info*/,
                                                               int32_t /*response*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setFacilityLockForAppResponse(const RadioResponseInfo& /*info*/,
                                                               int32_t /*retry*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setBarringPasswordResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getNetworkSelectionModeResponse(const RadioResponseInfo& /*info*/,
                                                                 bool /*manual*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setNetworkSelectionModeAutomaticResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setNetworkSelectionModeManualResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getAvailableNetworksResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<OperatorInfo>& /*networkInfos*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::startDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::stopDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getBasebandVersionResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*version*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::separateConnectionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setMuteResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getMuteResponse(const RadioResponseInfo& /*info*/,
                                                 bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getClipResponse(const RadioResponseInfo& /*info*/,
                                                 ClipStatus /*status*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getDataCallListResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<android::hardware::radio::V1_0::SetupDataCallResult>&
        /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendOemRilRequestRawResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_vec<uint8_t>& /*data*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendOemRilRequestStringsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& /*data*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setSuppServiceNotificationsResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::writeSmsToSimResponse(const RadioResponseInfo& /*info*/,
                                                       int32_t /*index*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::deleteSmsOnSimResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setBandModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getAvailableBandModesResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<RadioBandMode>& bandModes) {
    rspInfo = info;
    radioBandModes = bandModes;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::sendEnvelopeResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*commandResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendTerminalResponseToSimResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::handleStkCallSetupRequestFromSimResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::explicitCallTransferResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setPreferredNetworkTypeResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getPreferredNetworkTypeResponse(const RadioResponseInfo& /*info*/,
                                                                 PreferredNetworkType /*nw_type*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getNeighboringCidsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<NeighboringCell>& /*cells*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setLocationUpdatesResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setCdmaSubscriptionSourceResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setCdmaRoamingPreferenceResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getCdmaRoamingPreferenceResponse(const RadioResponseInfo& /*info*/,
                                                                  CdmaRoamingType /*type*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setTTYModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getTTYModeResponse(const RadioResponseInfo& /*info*/,
                                                    TtyMode /*mode*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setPreferredVoicePrivacyResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getPreferredVoicePrivacyResponse(const RadioResponseInfo& /*info*/,
                                                                  bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendCDMAFeatureCodeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendBurstDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendCdmaSmsResponse(const RadioResponseInfo& /*info*/,
                                                     const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::acknowledgeLastIncomingCdmaSmsResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getGsmBroadcastConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setGsmBroadcastConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setGsmBroadcastActivationResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getCdmaBroadcastConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setCdmaBroadcastConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setCdmaBroadcastActivationResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getCDMASubscriptionResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*mdn*/,
        const ::android::hardware::hidl_string& /*hSid*/,
        const ::android::hardware::hidl_string& /*hNid*/,
        const ::android::hardware::hidl_string& /*min*/,
        const ::android::hardware::hidl_string& /*prl*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::writeSmsToRuimResponse(const RadioResponseInfo& /*info*/,
                                                        uint32_t /*index*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::deleteSmsOnRuimResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getDeviceIdentityResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*imei*/,
        const ::android::hardware::hidl_string& /*imeisv*/,
        const ::android::hardware::hidl_string& /*esn*/,
        const ::android::hardware::hidl_string& /*meid*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::exitEmergencyCallbackModeResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getSmscAddressResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*smsc*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setSmscAddressResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::reportSmsMemoryStatusResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::reportStkServiceIsRunningResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getCdmaSubscriptionSourceResponse(
        const RadioResponseInfo& /*info*/, CdmaSubscriptionSource /*source*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::requestIsimAuthenticationResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*response*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::acknowledgeIncomingGsmSmsWithPduResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendEnvelopeWithStatusResponse(const RadioResponseInfo& /*info*/,
                                                                const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getVoiceRadioTechnologyResponse(
        const RadioResponseInfo& /*info*/,
        ::android::hardware::radio::V1_0::RadioTechnology /*rat*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getCellInfoListResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_0::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setCellInfoListRateResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setInitialAttachApnResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getImsRegistrationStateResponse(
        const RadioResponseInfo& /*info*/, bool /*isRegistered*/,
        RadioTechnologyFamily /*ratFamily*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendImsSmsResponse(const RadioResponseInfo& /*info*/,
                                                    const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::iccTransmitApduBasicChannelResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::iccOpenLogicalChannelResponse(
        const RadioResponseInfo& /*info*/, int32_t /*channelId*/,
        const ::android::hardware::hidl_vec<int8_t>& /*selectResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::iccCloseLogicalChannelResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::iccTransmitApduLogicalChannelResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::nvReadItemResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::nvWriteItemResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::nvWriteCdmaPrlResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::nvResetConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setUiccSubscriptionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setDataAllowedResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getHardwareConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<HardwareConfig>& /*config*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::requestIccSimAuthenticationResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setDataProfileResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::requestShutdownResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getRadioCapabilityResponse(
        const RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_0::RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setRadioCapabilityResponse(
        const RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_0::RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::startLceServiceResponse(const RadioResponseInfo& /*info*/,
                                                         const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::stopLceServiceResponse(const RadioResponseInfo& /*info*/,
                                                        const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::pullLceDataResponse(const RadioResponseInfo& /*info*/,
                                                     const LceDataInfo& /*lceInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getModemActivityInfoResponse(
        const RadioResponseInfo& /*info*/, const ActivityStatsInfo& /*activityInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setAllowedCarriersResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*numAllowed*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getAllowedCarriersResponse(
        const RadioResponseInfo& /*info*/, bool /*allAllowed*/,
        const CarrierRestrictions& /*carriers*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::sendDeviceStateResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setIndicationFilterResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::setSimCardPowerResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::acknowledgeRequest(int32_t /*serial*/) {
    return Void();
}

/* 1.1 Apis */
Return<void> RadioResponse_v1_4::setCarrierInfoForImsiEncryptionResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::setSimCardPowerResponse_1_1(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::startNetworkScanResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::stopNetworkScanResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::startKeepaliveResponse(const RadioResponseInfo& /*info*/,
                                                        const KeepaliveStatus& /*status*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::stopKeepaliveResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

/* 1.2 Apis */
Return<void> RadioResponse_v1_4::setSignalStrengthReportingCriteriaResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::setLinkCapacityReportingCriteriaResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getIccCardStatusResponse_1_2(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_2::CardStatus& /*card_status*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getCurrentCallsResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::Call>& calls) {
    rspInfo = info;
    currentCalls = calls;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getSignalStrengthResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_2::SignalStrength& /*sig_strength*/) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getSignalStrengthResponse_1_4(const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_4::SignalStrength& /*sig_strength*/) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getCellInfoListResponse_1_2(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_2::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_4::getVoiceRegistrationStateResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_2::VoiceRegStateResult& voiceRegResponse) {
    rspInfo = info;
    voiceRegResp = voiceRegResponse;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getDataRegistrationStateResponse_1_2(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_2::DataRegStateResult& /*dataRegResponse*/) {
    return Void();
}

/* 1.3 Apis */
Return<void> RadioResponse_v1_4::setSystemSelectionChannelsResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::enableModemResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getModemStackStatusResponse(const RadioResponseInfo& info,
                                                             const bool enabled) {
    rspInfo = info;
    isModemEnabled = enabled;
    parent_v1_4.notify(info.serial);
    return Void();
}

/* 1.4 Apis */
Return<void> RadioResponse_v1_4::emergencyDialResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::startNetworkScanResponse_1_4(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getDataRegistrationStateResponse_1_4(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_4::DataRegStateResult& dataRegResponse) {
    rspInfo = info;
    dataRegResp = dataRegResponse;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getCellInfoListResponse_1_4(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_4::CellInfo>& /*cellInfo*/) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getIccCardStatusResponse_1_4(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_4::CardStatus& card_status) {
    rspInfo = info;
    cardStatus = card_status;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getPreferredNetworkTypeBitmapResponse(
        const RadioResponseInfo& info, const ::android::hardware::hidl_bitfield<
                                               ::android::hardware::radio::V1_4::RadioAccessFamily>
                                               networkTypeBitmap) {
    rspInfo = info;
    networkTypeBitmapResponse = networkTypeBitmap;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::setPreferredNetworkTypeBitmapResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getDataCallListResponse_1_4(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_4::SetupDataCallResult>&
        /*dcResponse*/) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::setupDataCallResponse_1_4(
        const RadioResponseInfo& info,
        const android::hardware::radio::V1_4::SetupDataCallResult& /*dcResponse*/) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::setAllowedCarriersResponse_1_4(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_4.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_4::getAllowedCarriersResponse_1_4(
        const RadioResponseInfo& info, const CarrierRestrictionsWithPriority& carriers,
        SimLockMultiSimPolicy multiSimPolicy) {
    rspInfo = info;
    carrierRestrictionsResp = carriers;
    multiSimPolicyResp = multiSimPolicy;
    parent_v1_4.notify(info.serial);
    return Void();
}
