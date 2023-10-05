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
#pragma once

#include "DriverContext.h"
#include "GuaranteedCallback.h"

#include <aidl/android/hardware/radio/data/IRadioDataResponse.h>
#include <aidl/android/hardware/radio/ims/IRadioImsResponse.h>
#include <aidl/android/hardware/radio/ims/media/IImsMediaListener.h>
#include <aidl/android/hardware/radio/messaging/IRadioMessagingResponse.h>
#include <aidl/android/hardware/radio/modem/IRadioModemResponse.h>
#include <aidl/android/hardware/radio/network/IRadioNetworkResponse.h>
#include <aidl/android/hardware/radio/sim/IRadioSimResponse.h>
#include <aidl/android/hardware/radio/voice/IRadioVoiceResponse.h>
#include <android/hardware/radio/1.6/IRadioResponse.h>

namespace android::hardware::radio::compat {

class RadioResponse : public V1_6::IRadioResponse {
    std::shared_ptr<DriverContext> mContext;

    GuaranteedCallback<::aidl::android::hardware::radio::data::IRadioDataResponse,
                       ::aidl::android::hardware::radio::data::IRadioDataResponseDefault>
            mDataCb;
    GuaranteedCallback<::aidl::android::hardware::radio::messaging::IRadioMessagingResponse,
                       ::aidl::android::hardware::radio::messaging::IRadioMessagingResponseDefault>
            mMessagingCb;
    GuaranteedCallback<::aidl::android::hardware::radio::modem::IRadioModemResponse,
                       ::aidl::android::hardware::radio::modem::IRadioModemResponseDefault>
            mModemCb;
    GuaranteedCallback<::aidl::android::hardware::radio::network::IRadioNetworkResponse,
                       ::aidl::android::hardware::radio::network::IRadioNetworkResponseDefault>
            mNetworkCb;
    GuaranteedCallback<::aidl::android::hardware::radio::sim::IRadioSimResponse,
                       ::aidl::android::hardware::radio::sim::IRadioSimResponseDefault>
            mSimCb;
    GuaranteedCallback<::aidl::android::hardware::radio::voice::IRadioVoiceResponse,
                       ::aidl::android::hardware::radio::voice::IRadioVoiceResponseDefault>
            mVoiceCb;
    GuaranteedCallback<::aidl::android::hardware::radio::ims::IRadioImsResponse,
                       ::aidl::android::hardware::radio::ims::IRadioImsResponseDefault>
            mImsCb;

    // IRadioResponse @ 1.0
    Return<void> getIccCardStatusResponse(const V1_0::RadioResponseInfo& info,
                                          const V1_0::CardStatus& cardStatus) override;
    Return<void> supplyIccPinForAppResponse(const V1_0::RadioResponseInfo& info,
                                            int32_t remainingRetries) override;
    Return<void> supplyIccPukForAppResponse(const V1_0::RadioResponseInfo& info,
                                            int32_t remainingRetries) override;
    Return<void> supplyIccPin2ForAppResponse(const V1_0::RadioResponseInfo& info,
                                             int32_t remainingRetries) override;
    Return<void> supplyIccPuk2ForAppResponse(const V1_0::RadioResponseInfo& info,
                                             int32_t remainingRetries) override;
    Return<void> changeIccPinForAppResponse(const V1_0::RadioResponseInfo& info,
                                            int32_t remainingRetries) override;
    Return<void> changeIccPin2ForAppResponse(const V1_0::RadioResponseInfo& info,
                                             int32_t remainingRetries) override;
    Return<void> supplyNetworkDepersonalizationResponse(const V1_0::RadioResponseInfo& info,
                                                        int32_t remainingRetries) override;
    Return<void> getCurrentCallsResponse(const V1_0::RadioResponseInfo& info,
                                         const hidl_vec<V1_0::Call>& calls) override;
    Return<void> dialResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getIMSIForAppResponse(const V1_0::RadioResponseInfo& info,
                                       const hidl_string& imsi) override;
    Return<void> hangupConnectionResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> hangupWaitingOrBackgroundResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> hangupForegroundResumeBackgroundResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> switchWaitingOrHoldingAndActiveResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> conferenceResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> rejectCallResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getLastCallFailCauseResponse(
            const V1_0::RadioResponseInfo& info,
            const V1_0::LastCallFailCauseInfo& failCauseinfo) override;
    Return<void> getSignalStrengthResponse(const V1_0::RadioResponseInfo& info,
                                           const V1_0::SignalStrength& sigStrength) override;
    Return<void> getVoiceRegistrationStateResponse(
            const V1_0::RadioResponseInfo& info,
            const V1_0::VoiceRegStateResult& voiceRegResponse) override;
    Return<void> getDataRegistrationStateResponse(
            const V1_0::RadioResponseInfo& info,
            const V1_0::DataRegStateResult& dataRegResponse) override;
    Return<void> getOperatorResponse(const V1_0::RadioResponseInfo& info,
                                     const hidl_string& longName, const hidl_string& shortName,
                                     const hidl_string& numeric) override;
    Return<void> setRadioPowerResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> sendDtmfResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> sendSmsResponse(const V1_0::RadioResponseInfo& info,
                                 const V1_0::SendSmsResult& sms) override;
    Return<void> sendSMSExpectMoreResponse(const V1_0::RadioResponseInfo& info,
                                           const V1_0::SendSmsResult& sms) override;
    Return<void> setupDataCallResponse(const V1_0::RadioResponseInfo& info,
                                       const V1_0::SetupDataCallResult& dcResponse) override;
    Return<void> iccIOForAppResponse(const V1_0::RadioResponseInfo& info,
                                     const V1_0::IccIoResult& iccIo) override;
    Return<void> sendUssdResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> cancelPendingUssdResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getClirResponse(const V1_0::RadioResponseInfo& info, int32_t n,
                                 int32_t m) override;
    Return<void> setClirResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getCallForwardStatusResponse(
            const V1_0::RadioResponseInfo& info,
            const hidl_vec<V1_0::CallForwardInfo>& callForwardInfos) override;
    Return<void> setCallForwardResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getCallWaitingResponse(const V1_0::RadioResponseInfo& info, bool enable,
                                        int32_t serviceClass) override;
    Return<void> setCallWaitingResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> acknowledgeLastIncomingGsmSmsResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> acceptCallResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> deactivateDataCallResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getFacilityLockForAppResponse(const V1_0::RadioResponseInfo& info,
                                               int32_t response) override;
    Return<void> setFacilityLockForAppResponse(const V1_0::RadioResponseInfo& info,
                                               int32_t retry) override;
    Return<void> setBarringPasswordResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getNetworkSelectionModeResponse(const V1_0::RadioResponseInfo& info,
                                                 bool manual) override;
    Return<void> setNetworkSelectionModeAutomaticResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> setNetworkSelectionModeManualResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> getAvailableNetworksResponse(
            const V1_0::RadioResponseInfo& info,
            const hidl_vec<V1_0::OperatorInfo>& networkInfos) override;
    Return<void> startDtmfResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> stopDtmfResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getBasebandVersionResponse(const V1_0::RadioResponseInfo& info,
                                            const hidl_string& version) override;
    Return<void> separateConnectionResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setMuteResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getMuteResponse(const V1_0::RadioResponseInfo& info, bool enable) override;
    Return<void> getClipResponse(const V1_0::RadioResponseInfo& info,
                                 V1_0::ClipStatus status) override;
    Return<void> getDataCallListResponse(
            const V1_0::RadioResponseInfo& info,
            const hidl_vec<V1_0::SetupDataCallResult>& dcResponse) override;
    Return<void> setSuppServiceNotificationsResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> writeSmsToSimResponse(const V1_0::RadioResponseInfo& info, int32_t index) override;
    Return<void> deleteSmsOnSimResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setBandModeResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getAvailableBandModesResponse(
            const V1_0::RadioResponseInfo& info,
            const hidl_vec<V1_0::RadioBandMode>& bandModes) override;
    Return<void> sendEnvelopeResponse(const V1_0::RadioResponseInfo& info,
                                      const hidl_string& commandResponse) override;
    Return<void> sendTerminalResponseToSimResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> handleStkCallSetupRequestFromSimResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> explicitCallTransferResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setPreferredNetworkTypeResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getPreferredNetworkTypeResponse(const V1_0::RadioResponseInfo& info,
                                                 V1_0::PreferredNetworkType nwType) override;
    Return<void> getNeighboringCidsResponse(const V1_0::RadioResponseInfo& info,
                                            const hidl_vec<V1_0::NeighboringCell>& cells) override;
    Return<void> setLocationUpdatesResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setCdmaSubscriptionSourceResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setCdmaRoamingPreferenceResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getCdmaRoamingPreferenceResponse(const V1_0::RadioResponseInfo& info,
                                                  V1_0::CdmaRoamingType type) override;
    Return<void> setTTYModeResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getTTYModeResponse(const V1_0::RadioResponseInfo& info,
                                    V1_0::TtyMode mode) override;
    Return<void> setPreferredVoicePrivacyResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getPreferredVoicePrivacyResponse(const V1_0::RadioResponseInfo& info,
                                                  bool enable) override;
    Return<void> sendCDMAFeatureCodeResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> sendBurstDtmfResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> sendCdmaSmsResponse(const V1_0::RadioResponseInfo& info,
                                     const V1_0::SendSmsResult& sms) override;
    Return<void> acknowledgeLastIncomingCdmaSmsResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> getGsmBroadcastConfigResponse(
            const V1_0::RadioResponseInfo& info,
            const hidl_vec<V1_0::GsmBroadcastSmsConfigInfo>& configs) override;
    Return<void> setGsmBroadcastConfigResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setGsmBroadcastActivationResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getCdmaBroadcastConfigResponse(
            const V1_0::RadioResponseInfo& info,
            const hidl_vec<V1_0::CdmaBroadcastSmsConfigInfo>& configs) override;
    Return<void> setCdmaBroadcastConfigResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setCdmaBroadcastActivationResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getCDMASubscriptionResponse(  //
            const V1_0::RadioResponseInfo& info, const hidl_string& mdn, const hidl_string& hSid,
            const hidl_string& hNid, const hidl_string& min, const hidl_string& prl) override;
    Return<void> writeSmsToRuimResponse(const V1_0::RadioResponseInfo& info,
                                        uint32_t index) override;
    Return<void> deleteSmsOnRuimResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getDeviceIdentityResponse(  //
            const V1_0::RadioResponseInfo& info, const hidl_string& imei, const hidl_string& imeisv,
            const hidl_string& esn, const hidl_string& meid) override;
    Return<void> exitEmergencyCallbackModeResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getSmscAddressResponse(const V1_0::RadioResponseInfo& info,
                                        const hidl_string& smsc) override;
    Return<void> setSmscAddressResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> reportSmsMemoryStatusResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> reportStkServiceIsRunningResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getCdmaSubscriptionSourceResponse(const V1_0::RadioResponseInfo& info,
                                                   V1_0::CdmaSubscriptionSource source) override;
    Return<void> requestIsimAuthenticationResponse(const V1_0::RadioResponseInfo& info,
                                                   const hidl_string& response) override;
    Return<void> acknowledgeIncomingGsmSmsWithPduResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> sendEnvelopeWithStatusResponse(const V1_0::RadioResponseInfo& info,
                                                const V1_0::IccIoResult& iccIo) override;
    Return<void> getVoiceRadioTechnologyResponse(const V1_0::RadioResponseInfo& info,
                                                 V1_0::RadioTechnology rat) override;
    Return<void> getCellInfoListResponse(const V1_0::RadioResponseInfo& info,
                                         const hidl_vec<V1_0::CellInfo>& cellInfo) override;
    Return<void> setCellInfoListRateResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setInitialAttachApnResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getImsRegistrationStateResponse(const V1_0::RadioResponseInfo& info,
                                                 bool isRegistered,
                                                 V1_0::RadioTechnologyFamily ratFamily) override;
    Return<void> sendImsSmsResponse(const V1_0::RadioResponseInfo& info,
                                    const V1_0::SendSmsResult& sms) override;
    Return<void> iccTransmitApduBasicChannelResponse(const V1_0::RadioResponseInfo& info,
                                                     const V1_0::IccIoResult& result) override;
    Return<void> iccOpenLogicalChannelResponse(const V1_0::RadioResponseInfo& info,
                                               int32_t channelId,
                                               const hidl_vec<int8_t>& selectResponse) override;
    Return<void> iccCloseLogicalChannelResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> iccTransmitApduLogicalChannelResponse(const V1_0::RadioResponseInfo& info,
                                                       const V1_0::IccIoResult& result) override;
    Return<void> nvReadItemResponse(const V1_0::RadioResponseInfo& info,
                                    const hidl_string& result) override;
    Return<void> nvWriteItemResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> nvWriteCdmaPrlResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> nvResetConfigResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setUiccSubscriptionResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setDataAllowedResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getHardwareConfigResponse(const V1_0::RadioResponseInfo& info,
                                           const hidl_vec<V1_0::HardwareConfig>& config) override;
    Return<void> requestIccSimAuthenticationResponse(const V1_0::RadioResponseInfo& info,
                                                     const V1_0::IccIoResult& result) override;
    Return<void> setDataProfileResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> requestShutdownResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getRadioCapabilityResponse(const V1_0::RadioResponseInfo& info,
                                            const V1_0::RadioCapability& rc) override;
    Return<void> setRadioCapabilityResponse(const V1_0::RadioResponseInfo& info,
                                            const V1_0::RadioCapability& rc) override;
    Return<void> startLceServiceResponse(const V1_0::RadioResponseInfo& info,
                                         const V1_0::LceStatusInfo& statusInfo) override;
    Return<void> stopLceServiceResponse(const V1_0::RadioResponseInfo& info,
                                        const V1_0::LceStatusInfo& statusInfo) override;
    Return<void> pullLceDataResponse(const V1_0::RadioResponseInfo& info,
                                     const V1_0::LceDataInfo& lceInfo) override;
    Return<void> getModemActivityInfoResponse(const V1_0::RadioResponseInfo& info,
                                              const V1_0::ActivityStatsInfo& activityInfo) override;
    Return<void> setAllowedCarriersResponse(const V1_0::RadioResponseInfo& info,
                                            int32_t numAllowed) override;
    Return<void> getAllowedCarriersResponse(const V1_0::RadioResponseInfo& info, bool allAllowed,
                                            const V1_0::CarrierRestrictions& carriers) override;
    Return<void> sendDeviceStateResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setIndicationFilterResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> setSimCardPowerResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> acknowledgeRequest(int32_t serial) override;

    // IRadioResponse @ 1.1
    Return<void> setCarrierInfoForImsiEncryptionResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> setSimCardPowerResponse_1_1(const V1_0::RadioResponseInfo& info) override;
    Return<void> startNetworkScanResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> stopNetworkScanResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> startKeepaliveResponse(const V1_0::RadioResponseInfo& info,
                                        const V1_1::KeepaliveStatus& status) override;
    Return<void> stopKeepaliveResponse(const V1_0::RadioResponseInfo& info) override;

    // IRadioResponse @ 1.2
    Return<void> getCellInfoListResponse_1_2(const V1_0::RadioResponseInfo& info,
                                             const hidl_vec<V1_2::CellInfo>& cellInfo) override;
    Return<void> getIccCardStatusResponse_1_2(const V1_0::RadioResponseInfo& info,
                                              const V1_2::CardStatus& cardStatus) override;
    Return<void> setSignalStrengthReportingCriteriaResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> setLinkCapacityReportingCriteriaResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> getCurrentCallsResponse_1_2(const V1_0::RadioResponseInfo& info,
                                             const hidl_vec<V1_2::Call>& calls) override;
    Return<void> getSignalStrengthResponse_1_2(const V1_0::RadioResponseInfo& info,
                                               const V1_2::SignalStrength& signalStrength) override;
    Return<void> getVoiceRegistrationStateResponse_1_2(
            const V1_0::RadioResponseInfo& info,
            const V1_2::VoiceRegStateResult& voiceRegResponse) override;
    Return<void> getDataRegistrationStateResponse_1_2(
            const V1_0::RadioResponseInfo& info,
            const V1_2::DataRegStateResult& dataRegResponse) override;

    // IRadioResponse @ 1.3
    Return<void> setSystemSelectionChannelsResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> enableModemResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> getModemStackStatusResponse(const V1_0::RadioResponseInfo& info,
                                             bool isEnabled) override;

    // IRadioResponse @ 1.4
    Return<void> emergencyDialResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> startNetworkScanResponse_1_4(const V1_0::RadioResponseInfo& info) override;
    Return<void> getCellInfoListResponse_1_4(const V1_0::RadioResponseInfo& info,
                                             const hidl_vec<V1_4::CellInfo>& cellInfo) override;
    Return<void> getDataRegistrationStateResponse_1_4(
            const V1_0::RadioResponseInfo& info,
            const V1_4::DataRegStateResult& dataRegResponse) override;
    Return<void> getIccCardStatusResponse_1_4(const V1_0::RadioResponseInfo& info,
                                              const V1_4::CardStatus& cardStatus) override;
    Return<void> getPreferredNetworkTypeBitmapResponse(
            const V1_0::RadioResponseInfo& info,
            hidl_bitfield<V1_4::RadioAccessFamily> networkTypeBitmap) override;
    Return<void> setPreferredNetworkTypeBitmapResponse(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> getDataCallListResponse_1_4(
            const V1_0::RadioResponseInfo& info,
            const hidl_vec<V1_4::SetupDataCallResult>& dcResponse) override;
    Return<void> setupDataCallResponse_1_4(const V1_0::RadioResponseInfo& info,
                                           const V1_4::SetupDataCallResult& dcResponse) override;
    Return<void> setAllowedCarriersResponse_1_4(const V1_0::RadioResponseInfo& info) override;
    Return<void> getAllowedCarriersResponse_1_4(
            const V1_0::RadioResponseInfo& info,
            const V1_4::CarrierRestrictionsWithPriority& carriers,
            V1_4::SimLockMultiSimPolicy multiSimPolicy) override;
    Return<void> getSignalStrengthResponse_1_4(const V1_0::RadioResponseInfo& info,
                                               const V1_4::SignalStrength& signalStrength) override;

    // IRadioResponse @ 1.5
    Return<void> setSignalStrengthReportingCriteriaResponse_1_5(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> setLinkCapacityReportingCriteriaResponse_1_5(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> enableUiccApplicationsResponse(const V1_0::RadioResponseInfo& info) override;
    Return<void> areUiccApplicationsEnabledResponse(const V1_0::RadioResponseInfo& info,
                                                    bool enabled) override;
    Return<void> setSystemSelectionChannelsResponse_1_5(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> startNetworkScanResponse_1_5(const V1_0::RadioResponseInfo& info) override;
    Return<void> setupDataCallResponse_1_5(const V1_0::RadioResponseInfo& info,
                                           const V1_5::SetupDataCallResult& dcResponse) override;
    Return<void> getDataCallListResponse_1_5(
            const V1_0::RadioResponseInfo& info,
            const hidl_vec<V1_5::SetupDataCallResult>& dcResponse) override;
    Return<void> setInitialAttachApnResponse_1_5(const V1_0::RadioResponseInfo& info) override;
    Return<void> setDataProfileResponse_1_5(const V1_0::RadioResponseInfo& info) override;
    Return<void> setRadioPowerResponse_1_5(const V1_0::RadioResponseInfo& info) override;
    Return<void> setIndicationFilterResponse_1_5(const V1_0::RadioResponseInfo& info) override;
    Return<void> getBarringInfoResponse(const V1_0::RadioResponseInfo& info,
                                        const V1_5::CellIdentity& cellIdentity,
                                        const hidl_vec<V1_5::BarringInfo>& barringInfos) override;
    Return<void> getVoiceRegistrationStateResponse_1_5(
            const V1_0::RadioResponseInfo& info,
            const V1_5::RegStateResult& voiceRegResponse) override;
    Return<void> getDataRegistrationStateResponse_1_5(
            const V1_0::RadioResponseInfo& info,
            const V1_5::RegStateResult& dataRegResponse) override;
    Return<void> getCellInfoListResponse_1_5(const V1_0::RadioResponseInfo& info,
                                             const hidl_vec<V1_5::CellInfo>& cellInfo) override;
    Return<void> setNetworkSelectionModeManualResponse_1_5(
            const V1_0::RadioResponseInfo& info) override;
    Return<void> sendCdmaSmsExpectMoreResponse(const V1_0::RadioResponseInfo& info,
                                               const V1_0::SendSmsResult& sms) override;
    Return<void> supplySimDepersonalizationResponse(const V1_0::RadioResponseInfo& info,
                                                    V1_5::PersoSubstate persoType,
                                                    int32_t remainingRetries) override;
    Return<void> getIccCardStatusResponse_1_5(const V1_0::RadioResponseInfo& info,
                                              const V1_5::CardStatus& cardStatus) override;

    // IRadioResponse @ 1.6
    Return<void> setRadioPowerResponse_1_6(const V1_6::RadioResponseInfo& info) override;
    Return<void> setupDataCallResponse_1_6(const V1_6::RadioResponseInfo& info,
                                           const V1_6::SetupDataCallResult& dcResponse) override;
    Return<void> getDataCallListResponse_1_6(
            const V1_6::RadioResponseInfo& info,
            const hidl_vec<V1_6::SetupDataCallResult>& dcResponse) override;
    Return<void> sendSmsResponse_1_6(const V1_6::RadioResponseInfo& info,
                                     const V1_0::SendSmsResult& sms) override;
    Return<void> sendSmsExpectMoreResponse_1_6(const V1_6::RadioResponseInfo& info,
                                               const V1_0::SendSmsResult& sms) override;
    Return<void> sendCdmaSmsResponse_1_6(const V1_6::RadioResponseInfo& info,
                                         const V1_0::SendSmsResult& sms) override;
    Return<void> sendCdmaSmsExpectMoreResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                   const V1_0::SendSmsResult& sms) override;
    Return<void> setSimCardPowerResponse_1_6(const V1_6::RadioResponseInfo& info) override;
    Return<void> setNrDualConnectivityStateResponse(const V1_6::RadioResponseInfo& info) override;
    Return<void> isNrDualConnectivityEnabledResponse(const V1_6::RadioResponseInfo& info,
                                                     bool isEnabled) override;
    Return<void> allocatePduSessionIdResponse(const V1_6::RadioResponseInfo& info,
                                              int32_t id) override;
    Return<void> releasePduSessionIdResponse(const V1_6::RadioResponseInfo& info) override;
    Return<void> startHandoverResponse(const V1_6::RadioResponseInfo& info) override;
    Return<void> cancelHandoverResponse(const V1_6::RadioResponseInfo& info) override;
    Return<void> setAllowedNetworkTypesBitmapResponse(const V1_6::RadioResponseInfo& info) override;
    Return<void> getAllowedNetworkTypesBitmapResponse(
            const V1_6::RadioResponseInfo& info,
            hidl_bitfield<V1_4::RadioAccessFamily> networkTypeBitmap) override;
    Return<void> setDataThrottlingResponse(const V1_6::RadioResponseInfo& info) override;
    Return<void> getSystemSelectionChannelsResponse(
            const V1_6::RadioResponseInfo& info,
            const hidl_vec<V1_5::RadioAccessSpecifier>& specifiers) override;
    Return<void> getCellInfoListResponse_1_6(const V1_6::RadioResponseInfo& info,
                                             const hidl_vec<V1_6::CellInfo>& cellInfo) override;
    Return<void> getSignalStrengthResponse_1_6(const V1_6::RadioResponseInfo& info,
                                               const V1_6::SignalStrength& signalStrength) override;
    Return<void> getVoiceRegistrationStateResponse_1_6(
            const V1_6::RadioResponseInfo& info,
            const V1_6::RegStateResult& voiceRegResponse) override;
    Return<void> getDataRegistrationStateResponse_1_6(
            const V1_6::RadioResponseInfo& info,
            const V1_6::RegStateResult& dataRegResponse) override;
    Return<void> getCurrentCallsResponse_1_6(const V1_6::RadioResponseInfo& info,
                                             const hidl_vec<V1_6::Call>& calls) override;
    Return<void> getSlicingConfigResponse(const V1_6::RadioResponseInfo& info,
                                          const V1_6::SlicingConfig& slicingConfig) override;
    Return<void> getSimPhonebookRecordsResponse(const V1_6::RadioResponseInfo& info) override;
    Return<void> getSimPhonebookCapacityResponse(const V1_6::RadioResponseInfo& info,
                                                 const V1_6::PhonebookCapacity& capacity) override;
    Return<void> updateSimPhonebookRecordsResponse(const V1_6::RadioResponseInfo& info,
                                                   int32_t updatedRecordIndex) override;

  public:
    RadioResponse(std::shared_ptr<DriverContext> context);

    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::data::IRadioDataResponse> dataCb);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::messaging::IRadioMessagingResponse>
                    radioMessagingResponse);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::modem::IRadioModemResponse> modemCb);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::network::IRadioNetworkResponse> nwCb);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::sim::IRadioSimResponse> simCb);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::voice::IRadioVoiceResponse> voiceCb);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::ims::IRadioImsResponse> imsCb);

    std::shared_ptr<::aidl::android::hardware::radio::data::IRadioDataResponse> dataCb();
    std::shared_ptr<::aidl::android::hardware::radio::messaging::IRadioMessagingResponse>
    messagingCb();
    std::shared_ptr<::aidl::android::hardware::radio::modem::IRadioModemResponse> modemCb();
    std::shared_ptr<::aidl::android::hardware::radio::network::IRadioNetworkResponse> networkCb();
    std::shared_ptr<::aidl::android::hardware::radio::sim::IRadioSimResponse> simCb();
    std::shared_ptr<::aidl::android::hardware::radio::voice::IRadioVoiceResponse> voiceCb();
    std::shared_ptr<::aidl::android::hardware::radio::ims::IRadioImsResponse> imsCb();
};

}  // namespace android::hardware::radio::compat
