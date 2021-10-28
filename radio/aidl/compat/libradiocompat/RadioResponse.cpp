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

#include <libradiocompat/RadioResponse.h>

#include "debug.h"

// TODO(b/203699028): remove when fully implemented
#pragma GCC diagnostic ignored "-Wunused-parameter"

#define RADIO_MODULE "Common"

namespace android::hardware::radio::compat {

Return<void> RadioResponse::acknowledgeRequest(int32_t serial) {
    LOG_CALL << serial;
    // TODO(b/203699028): send to correct requestor or confirm if spam is not a problem
    if (mMessagingCb) mMessagingCb->acknowledgeRequest(serial);
    if (mSimCb) mSimCb->acknowledgeRequest(serial);
    return {};
}

Return<void> RadioResponse::supplyNetworkDepersonalizationResponse(
        const V1_0::RadioResponseInfo& info, int32_t remainingRetries) {
    return {};
}

Return<void> RadioResponse::getCurrentCallsResponse(const V1_0::RadioResponseInfo& info,
                                                    const hidl_vec<V1_0::Call>& calls) {
    return {};
}

Return<void> RadioResponse::dialResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::hangupConnectionResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::hangupWaitingOrBackgroundResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::hangupForegroundResumeBackgroundResponse(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::switchWaitingOrHoldingAndActiveResponse(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::conferenceResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::rejectCallResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getLastCallFailCauseResponse(
        const V1_0::RadioResponseInfo& info, const V1_0::LastCallFailCauseInfo& failCauseinfo) {
    return {};
}

Return<void> RadioResponse::getSignalStrengthResponse(const V1_0::RadioResponseInfo& info,
                                                      const V1_0::SignalStrength& sigStrength) {
    return {};
}

Return<void> RadioResponse::getVoiceRegistrationStateResponse(
        const V1_0::RadioResponseInfo& info, const V1_0::VoiceRegStateResult& voiceRegResponse) {
    return {};
}

Return<void> RadioResponse::getDataRegistrationStateResponse(
        const V1_0::RadioResponseInfo& info, const V1_0::DataRegStateResult& dataRegResponse) {
    return {};
}

Return<void> RadioResponse::getOperatorResponse(  //
        const V1_0::RadioResponseInfo& info, const hidl_string& longName,
        const hidl_string& shortName, const hidl_string& numeric) {
    return {};
}

Return<void> RadioResponse::setRadioPowerResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::sendDtmfResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setupDataCallResponse(const V1_0::RadioResponseInfo& info,
                                                  const V1_0::SetupDataCallResult& dcResponse) {
    return {};
}

Return<void> RadioResponse::getClirResponse(const V1_0::RadioResponseInfo& info, int32_t n,
                                            int32_t m) {
    return {};
}

Return<void> RadioResponse::setClirResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getCallForwardStatusResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::CallForwardInfo>& callFwdInfos) {
    return {};
}

Return<void> RadioResponse::setCallForwardResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getCallWaitingResponse(const V1_0::RadioResponseInfo& info, bool enable,
                                                   int32_t serviceClass) {
    return {};
}

Return<void> RadioResponse::setCallWaitingResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::acceptCallResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::deactivateDataCallResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setBarringPasswordResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getNetworkSelectionModeResponse(const V1_0::RadioResponseInfo& info,
                                                            bool manual) {
    return {};
}

Return<void> RadioResponse::setNetworkSelectionModeAutomaticResponse(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setNetworkSelectionModeManualResponse(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getAvailableNetworksResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::OperatorInfo>& networkInfos) {
    return {};
}

Return<void> RadioResponse::startDtmfResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::stopDtmfResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getBasebandVersionResponse(const V1_0::RadioResponseInfo& info,
                                                       const hidl_string& version) {
    return {};
}

Return<void> RadioResponse::separateConnectionResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setMuteResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getMuteResponse(const V1_0::RadioResponseInfo& info, bool enable) {
    return {};
}

Return<void> RadioResponse::getClipResponse(const V1_0::RadioResponseInfo& info,
                                            V1_0::ClipStatus status) {
    return {};
}

Return<void> RadioResponse::getDataCallListResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::SetupDataCallResult>& dcResp) {
    return {};
}

Return<void> RadioResponse::setSuppServiceNotificationsResponse(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setBandModeResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getAvailableBandModesResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::RadioBandMode>& bandModes) {
    return {};
}

Return<void> RadioResponse::handleStkCallSetupRequestFromSimResponse(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::explicitCallTransferResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setPreferredNetworkTypeResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getPreferredNetworkTypeResponse(const V1_0::RadioResponseInfo& info,
                                                            V1_0::PreferredNetworkType nwType) {
    return {};
}

Return<void> RadioResponse::getNeighboringCidsResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::NeighboringCell>& cells) {
    return {};
}

Return<void> RadioResponse::setLocationUpdatesResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setCdmaRoamingPreferenceResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getCdmaRoamingPreferenceResponse(const V1_0::RadioResponseInfo& info,
                                                             V1_0::CdmaRoamingType type) {
    return {};
}

Return<void> RadioResponse::setTTYModeResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getTTYModeResponse(const V1_0::RadioResponseInfo& info,
                                               V1_0::TtyMode mode) {
    return {};
}

Return<void> RadioResponse::setPreferredVoicePrivacyResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getPreferredVoicePrivacyResponse(const V1_0::RadioResponseInfo& info,
                                                             bool enable) {
    return {};
}

Return<void> RadioResponse::sendCDMAFeatureCodeResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::sendBurstDtmfResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getDeviceIdentityResponse(  //
        const V1_0::RadioResponseInfo& info, const hidl_string& imei, const hidl_string& imeisv,
        const hidl_string& esn, const hidl_string& meid) {
    return {};
}

Return<void> RadioResponse::exitEmergencyCallbackModeResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getVoiceRadioTechnologyResponse(const V1_0::RadioResponseInfo& info,
                                                            V1_0::RadioTechnology rat) {
    return {};
}

Return<void> RadioResponse::getCellInfoListResponse(const V1_0::RadioResponseInfo& info,
                                                    const hidl_vec<V1_0::CellInfo>& cellInfo) {
    return {};
}

Return<void> RadioResponse::setCellInfoListRateResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setInitialAttachApnResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getImsRegistrationStateResponse(  //
        const V1_0::RadioResponseInfo& info, bool isRegd, V1_0::RadioTechnologyFamily ratFamily) {
    return {};
}

Return<void> RadioResponse::nvReadItemResponse(const V1_0::RadioResponseInfo& info,
                                               const hidl_string& result) {
    return {};
}

Return<void> RadioResponse::nvWriteItemResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::nvWriteCdmaPrlResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::nvResetConfigResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setDataAllowedResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getHardwareConfigResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::HardwareConfig>& config) {
    return {};
}

Return<void> RadioResponse::setDataProfileResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::requestShutdownResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getRadioCapabilityResponse(const V1_0::RadioResponseInfo& info,
                                                       const V1_0::RadioCapability& rc) {
    return {};
}

Return<void> RadioResponse::setRadioCapabilityResponse(const V1_0::RadioResponseInfo& info,
                                                       const V1_0::RadioCapability& rc) {
    return {};
}

Return<void> RadioResponse::startLceServiceResponse(const V1_0::RadioResponseInfo& info,
                                                    const V1_0::LceStatusInfo& statusInfo) {
    return {};
}

Return<void> RadioResponse::stopLceServiceResponse(const V1_0::RadioResponseInfo& info,
                                                   const V1_0::LceStatusInfo& statusInfo) {
    return {};
}

Return<void> RadioResponse::pullLceDataResponse(const V1_0::RadioResponseInfo& info,
                                                const V1_0::LceDataInfo& lceInfo) {
    return {};
}

Return<void> RadioResponse::getModemActivityInfoResponse(
        const V1_0::RadioResponseInfo& info, const V1_0::ActivityStatsInfo& activityInfo) {
    return {};
}

Return<void> RadioResponse::sendDeviceStateResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setIndicationFilterResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::startNetworkScanResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::stopNetworkScanResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::startKeepaliveResponse(const V1_0::RadioResponseInfo& info,
                                                   const V1_1::KeepaliveStatus& status) {
    return {};
}

Return<void> RadioResponse::stopKeepaliveResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getCellInfoListResponse_1_2(const V1_0::RadioResponseInfo& info,
                                                        const hidl_vec<V1_2::CellInfo>& cellInfo) {
    return {};
}

Return<void> RadioResponse::setSignalStrengthReportingCriteriaResponse(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setLinkCapacityReportingCriteriaResponse(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getCurrentCallsResponse_1_2(const V1_0::RadioResponseInfo& info,
                                                        const hidl_vec<V1_2::Call>& calls) {
    return {};
}

Return<void> RadioResponse::getSignalStrengthResponse_1_2(
        const V1_0::RadioResponseInfo& info, const V1_2::SignalStrength& signalStrength) {
    return {};
}

Return<void> RadioResponse::getVoiceRegistrationStateResponse_1_2(
        const V1_0::RadioResponseInfo& info, const V1_2::VoiceRegStateResult& voiceRegResponse) {
    return {};
}

Return<void> RadioResponse::getDataRegistrationStateResponse_1_2(
        const V1_0::RadioResponseInfo& info, const V1_2::DataRegStateResult& dataRegResponse) {
    return {};
}

Return<void> RadioResponse::setSystemSelectionChannelsResponse(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::enableModemResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getModemStackStatusResponse(const V1_0::RadioResponseInfo& info,
                                                        bool isEnabled) {
    return {};
}

Return<void> RadioResponse::emergencyDialResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::startNetworkScanResponse_1_4(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getCellInfoListResponse_1_4(const V1_0::RadioResponseInfo& info,
                                                        const hidl_vec<V1_4::CellInfo>& cellInfo) {
    return {};
}

Return<void> RadioResponse::getDataRegistrationStateResponse_1_4(
        const V1_0::RadioResponseInfo& info, const V1_4::DataRegStateResult& dataRegResponse) {
    return {};
}

Return<void> RadioResponse::getPreferredNetworkTypeBitmapResponse(
        const V1_0::RadioResponseInfo& info,
        hidl_bitfield<V1_4::RadioAccessFamily> networkTypeBitmap) {
    return {};
}

Return<void> RadioResponse::setPreferredNetworkTypeBitmapResponse(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getDataCallListResponse_1_4(
        const V1_0::RadioResponseInfo& info,
        const hidl_vec<V1_4::SetupDataCallResult>& dcResponse) {
    return {};
}

Return<void> RadioResponse::setupDataCallResponse_1_4(const V1_0::RadioResponseInfo& info,
                                                      const V1_4::SetupDataCallResult& dcResponse) {
    return {};
}

Return<void> RadioResponse::getSignalStrengthResponse_1_4(
        const V1_0::RadioResponseInfo& info, const V1_4::SignalStrength& signalStrength) {
    return {};
}

Return<void> RadioResponse::setSignalStrengthReportingCriteriaResponse_1_5(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setLinkCapacityReportingCriteriaResponse_1_5(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setSystemSelectionChannelsResponse_1_5(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::startNetworkScanResponse_1_5(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setupDataCallResponse_1_5(const V1_0::RadioResponseInfo& info,
                                                      const V1_5::SetupDataCallResult& dcResponse) {
    return {};
}

Return<void> RadioResponse::getDataCallListResponse_1_5(
        const V1_0::RadioResponseInfo& info,
        const hidl_vec<V1_5::SetupDataCallResult>& dcResponse) {
    return {};
}

Return<void> RadioResponse::setInitialAttachApnResponse_1_5(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setDataProfileResponse_1_5(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setRadioPowerResponse_1_5(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setIndicationFilterResponse_1_5(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getBarringInfoResponse(
        const V1_0::RadioResponseInfo& info, const V1_5::CellIdentity& cellIdentity,
        const hidl_vec<V1_5::BarringInfo>& barringInfos) {
    return {};
}

Return<void> RadioResponse::getVoiceRegistrationStateResponse_1_5(
        const V1_0::RadioResponseInfo& info, const V1_5::RegStateResult& voiceRegResponse) {
    return {};
}

Return<void> RadioResponse::getDataRegistrationStateResponse_1_5(
        const V1_0::RadioResponseInfo& info, const V1_5::RegStateResult& dataRegResponse) {
    return {};
}

Return<void> RadioResponse::getCellInfoListResponse_1_5(const V1_0::RadioResponseInfo& info,
                                                        const hidl_vec<V1_5::CellInfo>& cellInfo) {
    return {};
}

Return<void> RadioResponse::setNetworkSelectionModeManualResponse_1_5(
        const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setRadioPowerResponse_1_6(const V1_6::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setupDataCallResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                      const V1_6::SetupDataCallResult& dcResponse) {
    return {};
}

Return<void> RadioResponse::getDataCallListResponse_1_6(
        const V1_6::RadioResponseInfo& info,
        const hidl_vec<V1_6::SetupDataCallResult>& dcResponse) {
    return {};
}

Return<void> RadioResponse::setNrDualConnectivityStateResponse(
        const V1_6::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::isNrDualConnectivityEnabledResponse(const V1_6::RadioResponseInfo& info,
                                                                bool isEnabled) {
    return {};
}

Return<void> RadioResponse::allocatePduSessionIdResponse(const V1_6::RadioResponseInfo& info,
                                                         int32_t id) {
    return {};
}

Return<void> RadioResponse::releasePduSessionIdResponse(const V1_6::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::startHandoverResponse(const V1_6::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::cancelHandoverResponse(const V1_6::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setAllowedNetworkTypesBitmapResponse(
        const V1_6::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getAllowedNetworkTypesBitmapResponse(
        const V1_6::RadioResponseInfo& info,
        hidl_bitfield<V1_4::RadioAccessFamily> networkTypeBitmap) {
    return {};
}

Return<void> RadioResponse::setDataThrottlingResponse(const V1_6::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getSystemSelectionChannelsResponse(
        const V1_6::RadioResponseInfo& info,
        const hidl_vec<V1_5::RadioAccessSpecifier>& specifiers) {
    return {};
}

Return<void> RadioResponse::getCellInfoListResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                        const hidl_vec<V1_6::CellInfo>& cellInfo) {
    return {};
}

Return<void> RadioResponse::getSignalStrengthResponse_1_6(
        const V1_6::RadioResponseInfo& info, const V1_6::SignalStrength& signalStrength) {
    return {};
}

Return<void> RadioResponse::getVoiceRegistrationStateResponse_1_6(
        const V1_6::RadioResponseInfo& info, const V1_6::RegStateResult& voiceRegResponse) {
    return {};
}

Return<void> RadioResponse::getDataRegistrationStateResponse_1_6(
        const V1_6::RadioResponseInfo& info, const V1_6::RegStateResult& dataRegResponse) {
    return {};
}

Return<void> RadioResponse::getCurrentCallsResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                        const hidl_vec<V1_6::Call>& calls) {
    return {};
}

Return<void> RadioResponse::getSlicingConfigResponse(const V1_6::RadioResponseInfo& info,
                                                     const V1_6::SlicingConfig& slicingConfig) {
    return {};
}

}  // namespace android::hardware::radio::compat
