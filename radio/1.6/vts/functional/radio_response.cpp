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

RadioResponse_v1_6::RadioResponse_v1_6(RadioHidlTest_v1_6& parent) : parent_v1_6(parent) {}

/* 1.0 Apis */
Return<void> RadioResponse_v1_6::getIccCardStatusResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::CardStatus& /*card_status*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplyIccPinForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplyIccPukForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplyIccPin2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplyIccPuk2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::changeIccPinForAppResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::changeIccPin2ForAppResponse(const RadioResponseInfo& /*info*/,
                                                             int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::supplyNetworkDepersonalizationResponse(
        const RadioResponseInfo& /*info*/, int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCurrentCallsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::Call>& /*calls*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::dialResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getIMSIForAppResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*imsi*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::hangupConnectionResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::hangupWaitingOrBackgroundResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::hangupForegroundResumeBackgroundResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::switchWaitingOrHoldingAndActiveResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::conferenceResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::rejectCallResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getLastCallFailCauseResponse(
        const RadioResponseInfo& /*info*/, const LastCallFailCauseInfo& /*failCauseInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getSignalStrengthResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::SignalStrength& /*sig_strength*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getVoiceRegistrationStateResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::VoiceRegStateResult& /*voiceRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getDataRegistrationStateResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_0::DataRegStateResult& /*dataRegResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getOperatorResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*longName*/,
        const ::android::hardware::hidl_string& /*shortName*/,
        const ::android::hardware::hidl_string& /*numeric*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setRadioPowerResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendSmsResponse(const RadioResponseInfo& /*info*/,
                                                 const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendSMSExpectMoreResponse(const RadioResponseInfo& /*info*/,
                                                           const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setupDataCallResponse(
        const RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_0::SetupDataCallResult& /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::iccIOForAppResponse(const RadioResponseInfo& /*info*/,
                                                     const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendUssdResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::cancelPendingUssdResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getClirResponse(const RadioResponseInfo& /*info*/, int32_t /*n*/,
                                                 int32_t /*m*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setClirResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCallForwardStatusResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_vec<CallForwardInfo>&
        /*callForwardInfos*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCallForwardResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCallWaitingResponse(const RadioResponseInfo& /*info*/,
                                                        bool /*enable*/, int32_t /*serviceClass*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCallWaitingResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::acknowledgeLastIncomingGsmSmsResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::acceptCallResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::deactivateDataCallResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getFacilityLockForAppResponse(const RadioResponseInfo& /*info*/,
                                                               int32_t /*response*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setFacilityLockForAppResponse(const RadioResponseInfo& /*info*/,
                                                               int32_t /*retry*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setBarringPasswordResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getNetworkSelectionModeResponse(const RadioResponseInfo& /*info*/,
                                                                 bool /*manual*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setNetworkSelectionModeAutomaticResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setNetworkSelectionModeManualResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getAvailableNetworksResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<OperatorInfo>& /*networkInfos*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::startDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::stopDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getBasebandVersionResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*version*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::separateConnectionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setMuteResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getMuteResponse(const RadioResponseInfo& /*info*/,
                                                 bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getClipResponse(const RadioResponseInfo& /*info*/,
                                                 ClipStatus /*status*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getDataCallListResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<android::hardware::radio::V1_0::SetupDataCallResult>&
        /*dcResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendOemRilRequestRawResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_vec<uint8_t>& /*data*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendOemRilRequestStringsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& /*data*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setSuppServiceNotificationsResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::writeSmsToSimResponse(const RadioResponseInfo& /*info*/,
                                                       int32_t /*index*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::deleteSmsOnSimResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setBandModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getAvailableBandModesResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<RadioBandMode>& bandModes) {
    rspInfo = info;
    radioBandModes = bandModes;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::sendEnvelopeResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_string& /*commandResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendTerminalResponseToSimResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::handleStkCallSetupRequestFromSimResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::explicitCallTransferResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setPreferredNetworkTypeResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getPreferredNetworkTypeResponse(const RadioResponseInfo& /*info*/,
                                                                 PreferredNetworkType /*nw_type*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getNeighboringCidsResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<NeighboringCell>& /*cells*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setLocationUpdatesResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCdmaSubscriptionSourceResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCdmaRoamingPreferenceResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCdmaRoamingPreferenceResponse(const RadioResponseInfo& /*info*/,
                                                                  CdmaRoamingType /*type*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setTTYModeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getTTYModeResponse(const RadioResponseInfo& /*info*/,
                                                    TtyMode /*mode*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setPreferredVoicePrivacyResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getPreferredVoicePrivacyResponse(const RadioResponseInfo& /*info*/,
                                                                  bool /*enable*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendCDMAFeatureCodeResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendBurstDtmfResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendCdmaSmsResponse(const RadioResponseInfo& /*info*/,
                                                     const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::acknowledgeLastIncomingCdmaSmsResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getGsmBroadcastConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setGsmBroadcastConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setGsmBroadcastActivationResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCdmaBroadcastConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo>& /*configs*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCdmaBroadcastConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCdmaBroadcastActivationResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCDMASubscriptionResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*mdn*/,
        const ::android::hardware::hidl_string& /*hSid*/,
        const ::android::hardware::hidl_string& /*hNid*/,
        const ::android::hardware::hidl_string& /*min*/,
        const ::android::hardware::hidl_string& /*prl*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::writeSmsToRuimResponse(const RadioResponseInfo& /*info*/,
                                                        uint32_t /*index*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::deleteSmsOnRuimResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getDeviceIdentityResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*imei*/,
        const ::android::hardware::hidl_string& /*imeisv*/,
        const ::android::hardware::hidl_string& /*esn*/,
        const ::android::hardware::hidl_string& /*meid*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::exitEmergencyCallbackModeResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getSmscAddressResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*smsc*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setSmscAddressResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::reportSmsMemoryStatusResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::reportStkServiceIsRunningResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCdmaSubscriptionSourceResponse(
        const RadioResponseInfo& /*info*/, CdmaSubscriptionSource /*source*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::requestIsimAuthenticationResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*response*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::acknowledgeIncomingGsmSmsWithPduResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendEnvelopeWithStatusResponse(const RadioResponseInfo& /*info*/,
                                                                const IccIoResult& /*iccIo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getVoiceRadioTechnologyResponse(
        const RadioResponseInfo& /*info*/,
        ::android::hardware::radio::V1_0::RadioTechnology /*rat*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCellInfoListResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_0::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setCellInfoListRateResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setInitialAttachApnResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getImsRegistrationStateResponse(
        const RadioResponseInfo& /*info*/, bool /*isRegistered*/,
        RadioTechnologyFamily /*ratFamily*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendImsSmsResponse(const RadioResponseInfo& /*info*/,
                                                    const SendSmsResult& /*sms*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::iccTransmitApduBasicChannelResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::iccOpenLogicalChannelResponse(
        const RadioResponseInfo& /*info*/, int32_t /*channelId*/,
        const ::android::hardware::hidl_vec<int8_t>& /*selectResponse*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::iccCloseLogicalChannelResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::iccTransmitApduLogicalChannelResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::nvReadItemResponse(
        const RadioResponseInfo& /*info*/, const ::android::hardware::hidl_string& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::nvWriteItemResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::nvWriteCdmaPrlResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::nvResetConfigResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setUiccSubscriptionResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setDataAllowedResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getHardwareConfigResponse(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<HardwareConfig>& /*config*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::requestIccSimAuthenticationResponse(
        const RadioResponseInfo& /*info*/, const IccIoResult& /*result*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setDataProfileResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::requestShutdownResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getRadioCapabilityResponse(
        const RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_0::RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setRadioCapabilityResponse(
        const RadioResponseInfo& /*info*/,
        const android::hardware::radio::V1_0::RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::startLceServiceResponse(const RadioResponseInfo& /*info*/,
                                                         const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::stopLceServiceResponse(const RadioResponseInfo& /*info*/,
                                                        const LceStatusInfo& /*statusInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::pullLceDataResponse(const RadioResponseInfo& /*info*/,
                                                     const LceDataInfo& /*lceInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getModemActivityInfoResponse(
        const RadioResponseInfo& /*info*/, const ActivityStatsInfo& /*activityInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setAllowedCarriersResponse(const RadioResponseInfo& /*info*/,
                                                            int32_t /*numAllowed*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getAllowedCarriersResponse(
        const RadioResponseInfo& /*info*/, bool /*allAllowed*/,
        const CarrierRestrictions& /*carriers*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::sendDeviceStateResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setIndicationFilterResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setSimCardPowerResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::acknowledgeRequest(int32_t /*serial*/) {
    return Void();
}

/* 1.1 Apis */
Return<void> RadioResponse_v1_6::setCarrierInfoForImsiEncryptionResponse(
        const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setSimCardPowerResponse_1_1(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::startNetworkScanResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::stopNetworkScanResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::startKeepaliveResponse(const RadioResponseInfo& /*info*/,
                                                        const KeepaliveStatus& /*status*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::stopKeepaliveResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

/* 1.2 Apis */
Return<void> RadioResponse_v1_6::setSignalStrengthReportingCriteriaResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setLinkCapacityReportingCriteriaResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getIccCardStatusResponse_1_2(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_2::CardStatus& /*card_status*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getCurrentCallsResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::Call>& calls) {
    rspInfo = info;
    currentCalls = calls;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getSignalStrengthResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_2::SignalStrength& /*sig_strength*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getSignalStrengthResponse_1_4(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_4::SignalStrength& /*sig_strength*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getCellInfoListResponse_1_2(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_2::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getVoiceRegistrationStateResponse_1_2(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_2::VoiceRegStateResult& /*voiceRegResponse*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getDataRegistrationStateResponse_1_2(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::radio::V1_2::DataRegStateResult& /*dataRegResponse*/) {
    return Void();
}

/* 1.3 Apis */
Return<void> RadioResponse_v1_6::setSystemSelectionChannelsResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::enableModemResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getModemStackStatusResponse(const RadioResponseInfo& info,
                                                             const bool enabled) {
    rspInfo = info;
    isModemEnabled = enabled;
    parent_v1_6.notify(info.serial);
    return Void();
}

/* 1.4 Apis */
Return<void> RadioResponse_v1_6::emergencyDialResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::startNetworkScanResponse_1_4(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getDataRegistrationStateResponse_1_4(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_4::DataRegStateResult& dataRegResponse) {
    rspInfo = info;
    dataRegResp = dataRegResponse;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getCellInfoListResponse_1_4(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_4::CellInfo>& /*cellInfo*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getIccCardStatusResponse_1_4(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_4::CardStatus& /*card_status*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getPreferredNetworkTypeBitmapResponse(
        const RadioResponseInfo& info, const ::android::hardware::hidl_bitfield<
                                               ::android::hardware::radio::V1_4::RadioAccessFamily>
                                               networkTypeBitmap) {
    rspInfo = info;
    networkTypeBitmapResponse = networkTypeBitmap;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setPreferredNetworkTypeBitmapResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getDataCallListResponse_1_4(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_4::SetupDataCallResult>&
        /*dcResponse*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setupDataCallResponse_1_4(
        const RadioResponseInfo& info,
        const android::hardware::radio::V1_4::SetupDataCallResult& /*dcResponse*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setAllowedCarriersResponse_1_4(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getAllowedCarriersResponse_1_4(
        const RadioResponseInfo& info, const CarrierRestrictionsWithPriority& carriers,
        SimLockMultiSimPolicy multiSimPolicy) {
    rspInfo = info;
    carrierRestrictionsResp = carriers;
    multiSimPolicyResp = multiSimPolicy;
    parent_v1_6.notify(info.serial);
    return Void();
}

/* 1.5 Apis */
Return<void> RadioResponse_v1_6::setSignalStrengthReportingCriteriaResponse_1_5(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setLinkCapacityReportingCriteriaResponse_1_5(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::enableUiccApplicationsResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::areUiccApplicationsEnabledResponse(const RadioResponseInfo& info,
                                                                    bool enabled) {
    rspInfo = info;
    areUiccApplicationsEnabled = enabled;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::canToggleUiccApplicationsEnablementResponse(
        const RadioResponseInfo& info, bool canToggle) {
    rspInfo = info;
    canToggleUiccApplicationsEnablement = canToggle;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setSystemSelectionChannelsResponse_1_5(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::startNetworkScanResponse_1_5(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setupDataCallResponse_1_5(
        const RadioResponseInfo& info,
        const android::hardware::radio::V1_5::SetupDataCallResult& /* dcResponse */) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getDataCallListResponse_1_5(
        const RadioResponseInfo& info,
        const hidl_vec<::android::hardware::radio::V1_5::SetupDataCallResult>& /* dcResponse */) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setInitialAttachApnResponse_1_5(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setDataProfileResponse_1_5(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setRadioPowerResponse_1_5(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::setIndicationFilterResponse_1_5(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getBarringInfoResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_5::CellIdentity& cellIdentity,
        const ::android::hardware::hidl_vec<::android::hardware::radio::V1_5::BarringInfo>&
                barringInfos) {
    this->barringCellIdentity = cellIdentity;
    this->barringInfos = barringInfos;
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getVoiceRegistrationStateResponse_1_5(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_5::RegStateResult& /*regResponse*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getDataRegistrationStateResponse_1_5(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_5::RegStateResult& /*regResponse*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::getCellInfoListResponse_1_5(
        const RadioResponseInfo& /*info*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_5::CellInfo>& /*cellInfo*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::setNetworkSelectionModeManualResponse_1_5(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::sendCdmaSmsExpectMoreResponse(const RadioResponseInfo& info,
                                                               const SendSmsResult& /*sms*/) {
    rspInfo = info;
    parent_v1_6.notify(info.serial);
    return Void();
}

Return<void> RadioResponse_v1_6::supplySimDepersonalizationResponse(
        const RadioResponseInfo& /*info*/,
        ::android::hardware::radio::V1_5::PersoSubstate /*persoType*/,
        int32_t /*remainingRetries*/) {
    return Void();
}

Return<void> RadioResponse_v1_6::getIccCardStatusResponse_1_5(
        const RadioResponseInfo& info,
        const ::android::hardware::radio::V1_5::CardStatus& card_status) {
    rspInfo = info;
    cardStatus = card_status;
    parent_v1_6.notify(info.serial);
    return Void();
}

/* 1.6 Apis */

