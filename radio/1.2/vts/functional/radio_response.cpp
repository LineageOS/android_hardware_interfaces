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

#include <radio_hidl_hal_utils_v1_2.h>

::android::hardware::radio::V1_2::CardStatus cardStatus;

RadioResponse_v1_2::RadioResponse_v1_2(RadioHidlTest_v1_2& parent) : parent_v1_2(parent) {}

/* 1.0 Apis */
Return<void> RadioResponse_v1_2::getIccCardStatusResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::radio::V1_0::CardStatus& /*card_status*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::supplyIccPinForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::supplyIccPukForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::supplyIccPin2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::supplyIccPuk2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::changeIccPinForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::changeIccPin2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::supplyNetworkDepersonalizationResponse(
    const RadioResponseInfo& /*info*/, int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getCurrentCallsResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::Call>& /*calls*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::dialResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getIMSIForAppResponse(
    const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*imsi*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::hangupConnectionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::hangupWaitingOrBackgroundResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::hangupForegroundResumeBackgroundResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::switchWaitingOrHoldingAndActiveResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::conferenceResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::rejectCallResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getLastCallFailCauseResponse(
    const RadioResponseInfo& /*info*/, const LastCallFailCauseInfo& /*failCauseInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getSignalStrengthResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::radio::V1_0::SignalStrength& /*sig_strength*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getVoiceRegistrationStateResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::radio::V1_0::VoiceRegStateResult& /*voiceRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getDataRegistrationStateResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::radio::V1_0::DataRegStateResult& /*dataRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getOperatorResponse(
    const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*longName*/,
    const ::android::hardware::hidl_string& /*shortName*/,
    const ::android::hardware::hidl_string& /*numeric*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setRadioPowerResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendSmsResponse(const RadioResponseInfo& /*info*/,
                                                 const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendSMSExpectMoreResponse(const RadioResponseInfo& /*info*/,
                                                           const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setupDataCallResponse(const RadioResponseInfo& info,
                                                       const SetupDataCallResult& /*dcResponse*/) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::iccIOForAppResponse(const RadioResponseInfo& /*info*/,
                                                     const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendUssdResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::cancelPendingUssdResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getClirResponse(const RadioResponseInfo& /*info*/, int32_t /*n*/,
                                                 int32_t /*m*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setClirResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getCallForwardStatusResponse(
    const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_vec<CallForwardInfo>&
    /*callForwardInfos*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setCallForwardResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getCallWaitingResponse(const RadioResponseInfo& /*info*/,
                                                        bool /*enable*/, int32_t /*serviceClass*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setCallWaitingResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::acknowledgeLastIncomingGsmSmsResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::acceptCallResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::deactivateDataCallResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::getFacilityLockForAppResponse(const RadioResponseInfo& /*info*/,
                                                               int32_t /*response*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setFacilityLockForAppResponse(const RadioResponseInfo& /*info*/,
                                                               int32_t /*retry*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setBarringPasswordResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getNetworkSelectionModeResponse(const RadioResponseInfo& /*info*/,
                                                                 bool /*manual*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setNetworkSelectionModeAutomaticResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setNetworkSelectionModeManualResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getAvailableNetworksResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec<OperatorInfo>& /*networkInfos*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::startDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::stopDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getBasebandVersionResponse(
    const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*version*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::separateConnectionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setMuteResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getMuteResponse(const RadioResponseInfo& /*info*/,
                                                 bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getClipResponse(const RadioResponseInfo& /*info*/,
                                                 ClipStatus /*status*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getDataCallListResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec<SetupDataCallResult>& /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendOemRilRequestRawResponse(
    const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_vec<uint8_t>& /*data*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendOemRilRequestStringsResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec< ::android::hardware::hidl_string>& /*data*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setSuppServiceNotificationsResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::writeSmsToSimResponse(const RadioResponseInfo& /*info*/,
                                                       int32_t /*index*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::deleteSmsOnSimResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setBandModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getAvailableBandModesResponse(
    const RadioResponseInfo& info, const ::android::hardware::hidl_vec<RadioBandMode>& bandModes) {
    rspInfo = info;
    radioBandModes = bandModes;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::sendEnvelopeResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_string& /*commandResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendTerminalResponseToSimResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::handleStkCallSetupRequestFromSimResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::explicitCallTransferResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setPreferredNetworkTypeResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getPreferredNetworkTypeResponse(const RadioResponseInfo& /*info*/,
                                                                 PreferredNetworkType /*nw_type*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getNeighboringCidsResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec<NeighboringCell>& /*cells*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setLocationUpdatesResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setCdmaSubscriptionSourceResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setCdmaRoamingPreferenceResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getCdmaRoamingPreferenceResponse(const RadioResponseInfo& /*info*/,
                                                                  CdmaRoamingType /*type*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setTTYModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getTTYModeResponse(const RadioResponseInfo& /*info*/,
                                                    TtyMode /*mode*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setPreferredVoicePrivacyResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getPreferredVoicePrivacyResponse(const RadioResponseInfo& /*info*/,
                                                                  bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendCDMAFeatureCodeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendBurstDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendCdmaSmsResponse(const RadioResponseInfo& /*info*/,
                                                     const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::acknowledgeLastIncomingCdmaSmsResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getGsmBroadcastConfigResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setGsmBroadcastConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setGsmBroadcastActivationResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getCdmaBroadcastConfigResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setCdmaBroadcastConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setCdmaBroadcastActivationResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getCDMASubscriptionResponse(
    const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*mdn*/,
    const ::android::hardware::hidl_string& /*hSid*/,
    const ::android::hardware::hidl_string& /*hNid*/,
    const ::android::hardware::hidl_string& /*min*/,
    const ::android::hardware::hidl_string& /*prl*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::writeSmsToRuimResponse(const RadioResponseInfo& /*info*/,
                                                        uint32_t /*index*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::deleteSmsOnRuimResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getDeviceIdentityResponse(
    const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*imei*/,
    const ::android::hardware::hidl_string& /*imeisv*/,
    const ::android::hardware::hidl_string& /*esn*/,
    const ::android::hardware::hidl_string& /*meid*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::exitEmergencyCallbackModeResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getSmscAddressResponse(
    const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*smsc*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setSmscAddressResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::reportSmsMemoryStatusResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::reportStkServiceIsRunningResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getCdmaSubscriptionSourceResponse(
    const RadioResponseInfo& /*info*/, CdmaSubscriptionSource /*source*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::requestIsimAuthenticationResponse(
    const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*response*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::acknowledgeIncomingGsmSmsWithPduResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendEnvelopeWithStatusResponse(const RadioResponseInfo& /*info*/,
                                                                const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getVoiceRadioTechnologyResponse(const RadioResponseInfo& /*info*/,
                                                                 RadioTechnology /*rat*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getCellInfoListResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setCellInfoListRateResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setInitialAttachApnResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getImsRegistrationStateResponse(
    const RadioResponseInfo& /*info*/, bool /*isRegistered*/, RadioTechnologyFamily /*ratFamily*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendImsSmsResponse(const RadioResponseInfo& /*info*/,
                                                    const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::iccTransmitApduBasicChannelResponse(
    const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::iccOpenLogicalChannelResponse(
    const RadioResponseInfo& /*info*/, int32_t /*channelId*/,
    const ::android::hardware::hidl_vec<int8_t>& /*selectResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::iccCloseLogicalChannelResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::iccTransmitApduLogicalChannelResponse(
    const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::nvReadItemResponse(
    const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::nvWriteItemResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::nvWriteCdmaPrlResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::nvResetConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setUiccSubscriptionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setDataAllowedResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getHardwareConfigResponse(
    const RadioResponseInfo& /*info*/,
    const ::android::hardware::hidl_vec<HardwareConfig>& /*config*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::requestIccSimAuthenticationResponse(
    const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setDataProfileResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::requestShutdownResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getRadioCapabilityResponse(const RadioResponseInfo& /*info*/,
                                                            const RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setRadioCapabilityResponse(const RadioResponseInfo& /*info*/,
                                                            const RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::startLceServiceResponse(const RadioResponseInfo& /*info*/,
                                                         const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::stopLceServiceResponse(const RadioResponseInfo& /*info*/,
                                                        const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::pullLceDataResponse(const RadioResponseInfo& /*info*/,
                                                     const LceDataInfo& /*lceInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getModemActivityInfoResponse(
    const RadioResponseInfo& /*info*/, const ActivityStatsInfo& /*activityInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setAllowedCarriersResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*numAllowed*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::getAllowedCarriersResponse(
    const RadioResponseInfo& /*info*/, bool /*allAllowed*/,
    const CarrierRestrictions& /*carriers*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::sendDeviceStateResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setIndicationFilterResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::setSimCardPowerResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::acknowledgeRequest(int32_t /*serial*/) {
    return Void();
}

/* 1.1 Apis */
Return<void> RadioResponse_v1_2::setCarrierInfoForImsiEncryptionResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::setSimCardPowerResponse_1_1(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::startNetworkScanResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::stopNetworkScanResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::startKeepaliveResponse(const RadioResponseInfo& /*info*/,
                                                        const KeepaliveStatus& /*status*/) {
    return Void();
}

Return<void> RadioResponse_v1_2::stopKeepaliveResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

/* 1.2 Apis */
Return<void> RadioResponse_v1_2::setSignalStrengthReportingCriteriaResponse(
    const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::setLinkCapacityReportingCriteriaResponse(
    const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::getIccCardStatusResponse_1_2(
    const RadioResponseInfo& info,
    const ::android::hardware::radio::V1_2::CardStatus& card_status) {
    rspInfo = info;
    cardStatus = card_status;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::getCurrentCallsResponse_1_2(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::Call>& /*calls*/) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::getSignalStrengthResponse_1_2(
    const RadioResponseInfo& info,
    const ::android::hardware::radio::V1_2::SignalStrength& /*sig_strength*/) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::getCellInfoListResponse_1_2(
    const RadioResponseInfo& info,
    const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::CellInfo>& /*cellInfo*/) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::getVoiceRegistrationStateResponse_1_2(
    const RadioResponseInfo& info,
    const ::android::hardware::radio::V1_2::VoiceRegStateResult& /*voiceRegResponse*/) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_2::getDataRegistrationStateResponse_1_2(
    const RadioResponseInfo& info,
    const ::android::hardware::radio::V1_2::DataRegStateResult& /*dataRegResponse*/) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}
