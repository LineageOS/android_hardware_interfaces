/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_3.h>

::android::hardware::radio::V1_2::CardStatus cardStatus;

RadioResponse_v1_3::RadioResponse_v1_3(RadioHidlTest_v1_3& parent) : parent_v1_3(parent) {}

/* 1.0 Apis */
Return<void> RadioResponse_v1_3::getIccCardStatusResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::CardStatus& /*card_status*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::supplyIccPinForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::supplyIccPukForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::supplyIccPin2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::supplyIccPuk2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::changeIccPinForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::changeIccPin2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::supplyNetworkDepersonalizationResponse(
        const RadioResponseInfo& /*info*/, int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getCurrentCallsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::Call>& /*calls*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::dialResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getIMSIForAppResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*imsi*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::hangupConnectionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::hangupWaitingOrBackgroundResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::hangupForegroundResumeBackgroundResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::switchWaitingOrHoldingAndActiveResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::conferenceResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::rejectCallResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getLastCallFailCauseResponse(
        const RadioResponseInfo& /*info*/, const LastCallFailCauseInfo& /*failCauseInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getSignalStrengthResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::SignalStrength& /*sig_strength*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getVoiceRegistrationStateResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::VoiceRegStateResult& /*voiceRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getDataRegistrationStateResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::DataRegStateResult& /*dataRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getOperatorResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*longName*/,
        const ::android::hardware::hidl_string& /*shortName*/,
        const ::android::hardware::hidl_string& /*numeric*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setRadioPowerResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendSmsResponse(const RadioResponseInfo& /*info*/,
                                                 const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendSMSExpectMoreResponse(const RadioResponseInfo& /*info*/,
                                                           const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setupDataCallResponse(const RadioResponseInfo& info,
                                                       const SetupDataCallResult& /*dcResponse*/) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::iccIOForAppResponse(const RadioResponseInfo& /*info*/,
                                                     const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendUssdResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::cancelPendingUssdResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getClirResponse(const RadioResponseInfo& /*info*/, int32_t /*n*/,
                                                 int32_t /*m*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setClirResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getCallForwardStatusResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_vec<CallForwardInfo>&
        /*callForwardInfos*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setCallForwardResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getCallWaitingResponse(const RadioResponseInfo& /*info*/,
                                                        bool /*enable*/, int32_t /*serviceClass*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setCallWaitingResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::acknowledgeLastIncomingGsmSmsResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::acceptCallResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::deactivateDataCallResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::getFacilityLockForAppResponse(const RadioResponseInfo& /*info*/,
                                                               int32_t /*response*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setFacilityLockForAppResponse(const RadioResponseInfo& /*info*/,
                                                               int32_t /*retry*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setBarringPasswordResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getNetworkSelectionModeResponse(const RadioResponseInfo& /*info*/,
                                                                 bool /*manual*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setNetworkSelectionModeAutomaticResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setNetworkSelectionModeManualResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getAvailableNetworksResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<OperatorInfo>& /*networkInfos*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::startDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::stopDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getBasebandVersionResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*version*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::separateConnectionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setMuteResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getMuteResponse(const RadioResponseInfo& /*info*/,
                                                 bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getClipResponse(const RadioResponseInfo& /*info*/,
                                                 ClipStatus /*status*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getDataCallListResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<SetupDataCallResult>& /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendOemRilRequestRawResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_vec<uint8_t>& /*data*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendOemRilRequestStringsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& /*data*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setSuppServiceNotificationsResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::writeSmsToSimResponse(const RadioResponseInfo& /*info*/,
                                                       int32_t /*index*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::deleteSmsOnSimResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setBandModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getAvailableBandModesResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<RadioBandMode>& /* bandModes */) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::sendEnvelopeResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*commandResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendTerminalResponseToSimResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::handleStkCallSetupRequestFromSimResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::explicitCallTransferResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setPreferredNetworkTypeResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getPreferredNetworkTypeResponse(const RadioResponseInfo& /*info*/,
                                                                 PreferredNetworkType /*nw_type*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getNeighboringCidsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<NeighboringCell>& /*cells*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setLocationUpdatesResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setCdmaSubscriptionSourceResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setCdmaRoamingPreferenceResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getCdmaRoamingPreferenceResponse(const RadioResponseInfo& /*info*/,
                                                                  CdmaRoamingType /*type*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setTTYModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getTTYModeResponse(const RadioResponseInfo& /*info*/,
                                                    TtyMode /*mode*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setPreferredVoicePrivacyResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getPreferredVoicePrivacyResponse(const RadioResponseInfo& /*info*/,
                                                                  bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendCDMAFeatureCodeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendBurstDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendCdmaSmsResponse(const RadioResponseInfo& /*info*/,
                                                     const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::acknowledgeLastIncomingCdmaSmsResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getGsmBroadcastConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setGsmBroadcastConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setGsmBroadcastActivationResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getCdmaBroadcastConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setCdmaBroadcastConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setCdmaBroadcastActivationResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getCDMASubscriptionResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*mdn*/,
        const ::android::hardware::hidl_string& /*hSid*/,
        const ::android::hardware::hidl_string& /*hNid*/,
        const ::android::hardware::hidl_string& /*min*/,
        const ::android::hardware::hidl_string& /*prl*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::writeSmsToRuimResponse(const RadioResponseInfo& /*info*/,
                                                        uint32_t /*index*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::deleteSmsOnRuimResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getDeviceIdentityResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*imei*/,
        const ::android::hardware::hidl_string& /*imeisv*/,
        const ::android::hardware::hidl_string& /*esn*/,
        const ::android::hardware::hidl_string& /*meid*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::exitEmergencyCallbackModeResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getSmscAddressResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*smsc*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setSmscAddressResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::reportSmsMemoryStatusResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::reportStkServiceIsRunningResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getCdmaSubscriptionSourceResponse(
        const RadioResponseInfo& /*info*/, CdmaSubscriptionSource /*source*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::requestIsimAuthenticationResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*response*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::acknowledgeIncomingGsmSmsWithPduResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendEnvelopeWithStatusResponse(const RadioResponseInfo& /*info*/,
                                                                const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getVoiceRadioTechnologyResponse(const RadioResponseInfo& /*info*/,
                                                                 RadioTechnology /*rat*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getCellInfoListResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_0::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setCellInfoListRateResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setInitialAttachApnResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getImsRegistrationStateResponse(
        const RadioResponseInfo& /*info*/, bool /*isRegistered*/,
        RadioTechnologyFamily /*ratFamily*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendImsSmsResponse(const RadioResponseInfo& /*info*/,
                                                    const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::iccTransmitApduBasicChannelResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::iccOpenLogicalChannelResponse(
        const RadioResponseInfo& /*info*/, int32_t /*channelId*/,
        const ::android::hardware::hidl_vec<int8_t>& /*selectResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::iccCloseLogicalChannelResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::iccTransmitApduLogicalChannelResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::nvReadItemResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::nvWriteItemResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::nvWriteCdmaPrlResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::nvResetConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setUiccSubscriptionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setDataAllowedResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getHardwareConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<HardwareConfig>& /*config*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::requestIccSimAuthenticationResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setDataProfileResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::requestShutdownResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getRadioCapabilityResponse(const RadioResponseInfo& /*info*/,
                                                            const RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setRadioCapabilityResponse(const RadioResponseInfo& /*info*/,
                                                            const RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::startLceServiceResponse(const RadioResponseInfo& /*info*/,
                                                         const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::stopLceServiceResponse(const RadioResponseInfo& /*info*/,
                                                        const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::pullLceDataResponse(const RadioResponseInfo& /*info*/,
                                                     const LceDataInfo& /*lceInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getModemActivityInfoResponse(
        const RadioResponseInfo& /*info*/, const ActivityStatsInfo& /*activityInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setAllowedCarriersResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*numAllowed*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::getAllowedCarriersResponse(
        const RadioResponseInfo& /*info*/, bool /*allAllowed*/,
        const CarrierRestrictions& /*carriers*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::sendDeviceStateResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setIndicationFilterResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::setSimCardPowerResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::acknowledgeRequest(int32_t /*serial*/) {
    return Void();
}

/* 1.1 Apis */
Return<void> RadioResponse_v1_3::setCarrierInfoForImsiEncryptionResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::setSimCardPowerResponse_1_1(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::startNetworkScanResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::stopNetworkScanResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::startKeepaliveResponse(const RadioResponseInfo& /*info*/,
                                                        const KeepaliveStatus& /*status*/) {
    return Void();
}

Return<void> RadioResponse_v1_3::stopKeepaliveResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

/* 1.2 Apis */
Return<void> RadioResponse_v1_3::setSignalStrengthReportingCriteriaResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::setLinkCapacityReportingCriteriaResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::getIccCardStatusResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_2::CardStatus& card_status) {
    rspInfo = info;
    cardStatus = card_status;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::getCurrentCallsResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::Call>& /*calls*/) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::getSignalStrengthResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_2::SignalStrength& /*sig_strength*/) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::getCellInfoListResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_2::CellInfo>& /*cellInfo*/) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::getVoiceRegistrationStateResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_2::VoiceRegStateResult& /*voiceRegResponse*/) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::getDataRegistrationStateResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_2::DataRegStateResult& dataRegResponse) {
    rspInfo = info;
    dataRegResp = dataRegResponse;
    parent_v1_3.notify(info.serial);
    return Void();
}

/* 1.3 Api */
Return<void> RadioResponse_v1_3::setSystemSelectionChannelsResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::enableModemResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    enableModemResponseToggle = !enableModemResponseToggle;
    parent_v1_3.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_3::getModemStackStatusResponse(const RadioResponseInfo& info,
                                                             const bool enabled) {
    rspInfo = info;
    isModemEnabled = enabled;
    parent_v1_3.notify(info.serial);
    return Void();
}
