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

#include <android-base/logging.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <android/hardware/radio/config/1.1/IRadioConfig.h>

#include <android/hardware/radio/1.5/IRadio.h>
#include <android/hardware/radio/1.5/IRadioIndication.h>
#include <android/hardware/radio/1.5/IRadioResponse.h>
#include <android/hardware/radio/1.5/types.h>

#include "vts_test_util.h"

using namespace ::android::hardware::radio::V1_5;
using namespace ::android::hardware::radio::V1_4;
using namespace ::android::hardware::radio::V1_3;
using namespace ::android::hardware::radio::V1_2;
using namespace ::android::hardware::radio::V1_1;
using namespace ::android::hardware::radio::V1_0;

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

#define TIMEOUT_PERIOD 75
#define MODEM_EMERGENCY_CALL_ESTABLISH_TIME 3
#define MODEM_EMERGENCY_CALL_DISCONNECT_TIME 3

#define RADIO_SERVICE_NAME "slot1"

class RadioHidlTest_v1_5;
extern ::android::hardware::radio::V1_4::CardStatus cardStatus;

/* Callback class for radio response v1_5 */
class RadioResponse_v1_5 : public ::android::hardware::radio::V1_5::IRadioResponse {
  protected:
    RadioHidlTest_v1_5& parent_v1_5;

  public:
    hidl_vec<RadioBandMode> radioBandModes;

    RadioResponseInfo rspInfo;

    // Call
    hidl_vec<::android::hardware::radio::V1_2::Call> currentCalls;

    // Modem
    bool isModemEnabled;
    bool enableModemResponseToggle;

    ::android::hardware::hidl_bitfield<::android::hardware::radio::V1_4::RadioAccessFamily>
            networkTypeBitmapResponse;

    // Data
    ::android::hardware::radio::V1_4::DataRegStateResult dataRegResp;

    // SimLock status
    ::android::hardware::radio::V1_4::CarrierRestrictionsWithPriority carrierRestrictionsResp;
    ::android::hardware::radio::V1_4::SimLockMultiSimPolicy multiSimPolicyResp;

    // Whether toggling uicc applications operation is supported.
    bool canToggleUiccApplicationsEnablement;

    // Whether Uicc applications are enabled or not.
    bool areUiccApplicationsEnabled;

    RadioResponse_v1_5(RadioHidlTest_v1_5& parent_v1_5);
    virtual ~RadioResponse_v1_5() = default;

    Return<void> getIccCardStatusResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_0::CardStatus& cardStatus);

    Return<void> supplyIccPinForAppResponse(const RadioResponseInfo& info,
                                            int32_t remainingRetries);

    Return<void> supplyIccPukForAppResponse(const RadioResponseInfo& info,
                                            int32_t remainingRetries);

    Return<void> supplyIccPin2ForAppResponse(const RadioResponseInfo& info,
                                             int32_t remainingRetries);

    Return<void> supplyIccPuk2ForAppResponse(const RadioResponseInfo& info,
                                             int32_t remainingRetries);

    Return<void> changeIccPinForAppResponse(const RadioResponseInfo& info,
                                            int32_t remainingRetries);

    Return<void> changeIccPin2ForAppResponse(const RadioResponseInfo& info,
                                             int32_t remainingRetries);

    Return<void> supplyNetworkDepersonalizationResponse(const RadioResponseInfo& info,
                                                        int32_t remainingRetries);

    Return<void> getCurrentCallsResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::Call>& calls);

    Return<void> dialResponse(const RadioResponseInfo& info);

    Return<void> getIMSIForAppResponse(const RadioResponseInfo& info,
                                       const ::android::hardware::hidl_string& imsi);

    Return<void> hangupConnectionResponse(const RadioResponseInfo& info);

    Return<void> hangupWaitingOrBackgroundResponse(const RadioResponseInfo& info);

    Return<void> hangupForegroundResumeBackgroundResponse(const RadioResponseInfo& info);

    Return<void> switchWaitingOrHoldingAndActiveResponse(const RadioResponseInfo& info);

    Return<void> conferenceResponse(const RadioResponseInfo& info);

    Return<void> rejectCallResponse(const RadioResponseInfo& info);

    Return<void> getLastCallFailCauseResponse(const RadioResponseInfo& info,
                                              const LastCallFailCauseInfo& failCauseInfo);

    Return<void> getSignalStrengthResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_0::SignalStrength& sigStrength);

    Return<void> getVoiceRegistrationStateResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_0::VoiceRegStateResult& voiceRegResponse);

    Return<void> getDataRegistrationStateResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_0::DataRegStateResult& dataRegResponse);

    Return<void> getOperatorResponse(const RadioResponseInfo& info,
                                     const ::android::hardware::hidl_string& longName,
                                     const ::android::hardware::hidl_string& shortName,
                                     const ::android::hardware::hidl_string& numeric);

    Return<void> setRadioPowerResponse(const RadioResponseInfo& info);

    Return<void> sendDtmfResponse(const RadioResponseInfo& info);

    Return<void> sendSmsResponse(const RadioResponseInfo& info, const SendSmsResult& sms);

    Return<void> sendSMSExpectMoreResponse(const RadioResponseInfo& info, const SendSmsResult& sms);

    Return<void> setupDataCallResponse(
            const RadioResponseInfo& info,
            const android::hardware::radio::V1_0::SetupDataCallResult& dcResponse);

    Return<void> iccIOForAppResponse(const RadioResponseInfo& info, const IccIoResult& iccIo);

    Return<void> sendUssdResponse(const RadioResponseInfo& info);

    Return<void> cancelPendingUssdResponse(const RadioResponseInfo& info);

    Return<void> getClirResponse(const RadioResponseInfo& info, int32_t n, int32_t m);

    Return<void> setClirResponse(const RadioResponseInfo& info);

    Return<void> getCallForwardStatusResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<CallForwardInfo>& call_forwardInfos);

    Return<void> setCallForwardResponse(const RadioResponseInfo& info);

    Return<void> getCallWaitingResponse(const RadioResponseInfo& info, bool enable,
                                        int32_t serviceClass);

    Return<void> setCallWaitingResponse(const RadioResponseInfo& info);

    Return<void> acknowledgeLastIncomingGsmSmsResponse(const RadioResponseInfo& info);

    Return<void> acceptCallResponse(const RadioResponseInfo& info);

    Return<void> deactivateDataCallResponse(const RadioResponseInfo& info);

    Return<void> getFacilityLockForAppResponse(const RadioResponseInfo& info, int32_t response);

    Return<void> setFacilityLockForAppResponse(const RadioResponseInfo& info, int32_t retry);

    Return<void> setBarringPasswordResponse(const RadioResponseInfo& info);

    Return<void> getNetworkSelectionModeResponse(const RadioResponseInfo& info, bool manual);

    Return<void> setNetworkSelectionModeAutomaticResponse(const RadioResponseInfo& info);

    Return<void> setNetworkSelectionModeManualResponse(const RadioResponseInfo& info);

    Return<void> getAvailableNetworksResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<OperatorInfo>& networkInfos);

    Return<void> startDtmfResponse(const RadioResponseInfo& info);

    Return<void> stopDtmfResponse(const RadioResponseInfo& info);

    Return<void> getBasebandVersionResponse(const RadioResponseInfo& info,
                                            const ::android::hardware::hidl_string& version);

    Return<void> separateConnectionResponse(const RadioResponseInfo& info);

    Return<void> setMuteResponse(const RadioResponseInfo& info);

    Return<void> getMuteResponse(const RadioResponseInfo& info, bool enable);

    Return<void> getClipResponse(const RadioResponseInfo& info, ClipStatus status);

    Return<void> getDataCallListResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<
                    android::hardware::radio::V1_0::SetupDataCallResult>& dcResponse);

    Return<void> sendOemRilRequestRawResponse(const RadioResponseInfo& info,
                                              const ::android::hardware::hidl_vec<uint8_t>& data);

    Return<void> sendOemRilRequestStringsResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& data);

    Return<void> setSuppServiceNotificationsResponse(const RadioResponseInfo& info);

    Return<void> writeSmsToSimResponse(const RadioResponseInfo& info, int32_t index);

    Return<void> deleteSmsOnSimResponse(const RadioResponseInfo& info);

    Return<void> setBandModeResponse(const RadioResponseInfo& info);

    Return<void> getAvailableBandModesResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<RadioBandMode>& bandModes);

    Return<void> sendEnvelopeResponse(const RadioResponseInfo& info,
                                      const ::android::hardware::hidl_string& commandResponse);

    Return<void> sendTerminalResponseToSimResponse(const RadioResponseInfo& info);

    Return<void> handleStkCallSetupRequestFromSimResponse(const RadioResponseInfo& info);

    Return<void> explicitCallTransferResponse(const RadioResponseInfo& info);

    Return<void> setPreferredNetworkTypeResponse(const RadioResponseInfo& info);

    Return<void> getPreferredNetworkTypeResponse(const RadioResponseInfo& info,
                                                 PreferredNetworkType nwType);

    Return<void> getNeighboringCidsResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<NeighboringCell>& cells);

    Return<void> setLocationUpdatesResponse(const RadioResponseInfo& info);

    Return<void> setCdmaSubscriptionSourceResponse(const RadioResponseInfo& info);

    Return<void> setCdmaRoamingPreferenceResponse(const RadioResponseInfo& info);

    Return<void> getCdmaRoamingPreferenceResponse(const RadioResponseInfo& info,
                                                  CdmaRoamingType type);

    Return<void> setTTYModeResponse(const RadioResponseInfo& info);

    Return<void> getTTYModeResponse(const RadioResponseInfo& info, TtyMode mode);

    Return<void> setPreferredVoicePrivacyResponse(const RadioResponseInfo& info);

    Return<void> getPreferredVoicePrivacyResponse(const RadioResponseInfo& info, bool enable);

    Return<void> sendCDMAFeatureCodeResponse(const RadioResponseInfo& info);

    Return<void> sendBurstDtmfResponse(const RadioResponseInfo& info);

    Return<void> sendCdmaSmsResponse(const RadioResponseInfo& info, const SendSmsResult& sms);

    Return<void> acknowledgeLastIncomingCdmaSmsResponse(const RadioResponseInfo& info);

    Return<void> getGsmBroadcastConfigResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo>& configs);

    Return<void> setGsmBroadcastConfigResponse(const RadioResponseInfo& info);

    Return<void> setGsmBroadcastActivationResponse(const RadioResponseInfo& info);

    Return<void> getCdmaBroadcastConfigResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo>& configs);

    Return<void> setCdmaBroadcastConfigResponse(const RadioResponseInfo& info);

    Return<void> setCdmaBroadcastActivationResponse(const RadioResponseInfo& info);

    Return<void> getCDMASubscriptionResponse(const RadioResponseInfo& info,
                                             const ::android::hardware::hidl_string& mdn,
                                             const ::android::hardware::hidl_string& hSid,
                                             const ::android::hardware::hidl_string& hNid,
                                             const ::android::hardware::hidl_string& min,
                                             const ::android::hardware::hidl_string& prl);

    Return<void> writeSmsToRuimResponse(const RadioResponseInfo& info, uint32_t index);

    Return<void> deleteSmsOnRuimResponse(const RadioResponseInfo& info);

    Return<void> getDeviceIdentityResponse(const RadioResponseInfo& info,
                                           const ::android::hardware::hidl_string& imei,
                                           const ::android::hardware::hidl_string& imeisv,
                                           const ::android::hardware::hidl_string& esn,
                                           const ::android::hardware::hidl_string& meid);

    Return<void> exitEmergencyCallbackModeResponse(const RadioResponseInfo& info);

    Return<void> getSmscAddressResponse(const RadioResponseInfo& info,
                                        const ::android::hardware::hidl_string& smsc);

    Return<void> setSmscAddressResponse(const RadioResponseInfo& info);

    Return<void> reportSmsMemoryStatusResponse(const RadioResponseInfo& info);

    Return<void> reportStkServiceIsRunningResponse(const RadioResponseInfo& info);

    Return<void> getCdmaSubscriptionSourceResponse(const RadioResponseInfo& info,
                                                   CdmaSubscriptionSource source);

    Return<void> requestIsimAuthenticationResponse(
            const RadioResponseInfo& info, const ::android::hardware::hidl_string& response);

    Return<void> acknowledgeIncomingGsmSmsWithPduResponse(const RadioResponseInfo& info);

    Return<void> sendEnvelopeWithStatusResponse(const RadioResponseInfo& info,
                                                const IccIoResult& iccIo);

    Return<void> getVoiceRadioTechnologyResponse(
            const RadioResponseInfo& info, ::android::hardware::radio::V1_0::RadioTechnology rat);

    Return<void> getCellInfoListResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::CellInfo>&
                    cellInfo);

    Return<void> setCellInfoListRateResponse(const RadioResponseInfo& info);

    Return<void> setInitialAttachApnResponse(const RadioResponseInfo& info);

    Return<void> getImsRegistrationStateResponse(const RadioResponseInfo& info, bool isRegistered,
                                                 RadioTechnologyFamily ratFamily);

    Return<void> sendImsSmsResponse(const RadioResponseInfo& info, const SendSmsResult& sms);

    Return<void> iccTransmitApduBasicChannelResponse(const RadioResponseInfo& info,
                                                     const IccIoResult& result);

    Return<void> iccOpenLogicalChannelResponse(
            const RadioResponseInfo& info, int32_t channelId,
            const ::android::hardware::hidl_vec<int8_t>& selectResponse);

    Return<void> iccCloseLogicalChannelResponse(const RadioResponseInfo& info);

    Return<void> iccTransmitApduLogicalChannelResponse(const RadioResponseInfo& info,
                                                       const IccIoResult& result);

    Return<void> nvReadItemResponse(const RadioResponseInfo& info,
                                    const ::android::hardware::hidl_string& result);

    Return<void> nvWriteItemResponse(const RadioResponseInfo& info);

    Return<void> nvWriteCdmaPrlResponse(const RadioResponseInfo& info);

    Return<void> nvResetConfigResponse(const RadioResponseInfo& info);

    Return<void> setUiccSubscriptionResponse(const RadioResponseInfo& info);

    Return<void> setDataAllowedResponse(const RadioResponseInfo& info);

    Return<void> getHardwareConfigResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<HardwareConfig>& config);

    Return<void> requestIccSimAuthenticationResponse(const RadioResponseInfo& info,
                                                     const IccIoResult& result);

    Return<void> setDataProfileResponse(const RadioResponseInfo& info);

    Return<void> requestShutdownResponse(const RadioResponseInfo& info);

    Return<void> getRadioCapabilityResponse(
            const RadioResponseInfo& info,
            const android::hardware::radio::V1_0::RadioCapability& rc);

    Return<void> setRadioCapabilityResponse(
            const RadioResponseInfo& info,
            const android::hardware::radio::V1_0::RadioCapability& rc);

    Return<void> startLceServiceResponse(const RadioResponseInfo& info,
                                         const LceStatusInfo& statusInfo);

    Return<void> stopLceServiceResponse(const RadioResponseInfo& info,
                                        const LceStatusInfo& statusInfo);

    Return<void> pullLceDataResponse(const RadioResponseInfo& info, const LceDataInfo& lceInfo);

    Return<void> getModemActivityInfoResponse(const RadioResponseInfo& info,
                                              const ActivityStatsInfo& activityInfo);

    Return<void> setAllowedCarriersResponse(const RadioResponseInfo& info, int32_t numAllowed);

    Return<void> getAllowedCarriersResponse(const RadioResponseInfo& info, bool allAllowed,
                                            const CarrierRestrictions& carriers);

    Return<void> sendDeviceStateResponse(const RadioResponseInfo& info);

    Return<void> setIndicationFilterResponse(const RadioResponseInfo& info);

    Return<void> setSimCardPowerResponse(const RadioResponseInfo& info);

    Return<void> acknowledgeRequest(int32_t serial);

    /* 1.1 Api */
    Return<void> setCarrierInfoForImsiEncryptionResponse(const RadioResponseInfo& info);

    Return<void> setSimCardPowerResponse_1_1(const RadioResponseInfo& info);

    Return<void> startNetworkScanResponse(const RadioResponseInfo& info);

    Return<void> stopNetworkScanResponse(const RadioResponseInfo& info);

    Return<void> startKeepaliveResponse(const RadioResponseInfo& info,
                                        const KeepaliveStatus& status);

    Return<void> stopKeepaliveResponse(const RadioResponseInfo& info);

    /* 1.2 Api */
    Return<void> setSignalStrengthReportingCriteriaResponse(const RadioResponseInfo& info);

    Return<void> setLinkCapacityReportingCriteriaResponse(const RadioResponseInfo& info);

    Return<void> getIccCardStatusResponse_1_2(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_2::CardStatus& card_status);

    Return<void> getCurrentCallsResponse_1_2(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::Call>& calls);

    Return<void> getSignalStrengthResponse_1_2(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_2::SignalStrength& sig_strength);

    Return<void> getSignalStrengthResponse_1_4(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_4::SignalStrength& sig_strength);

    Return<void> getCellInfoListResponse_1_2(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::CellInfo>&
                    cellInfo);

    Return<void> getVoiceRegistrationStateResponse_1_2(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_2::VoiceRegStateResult& voiceRegResponse);

    Return<void> getDataRegistrationStateResponse_1_2(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_2::DataRegStateResult& dataRegResponse);

    /* 1.3 Api */
    Return<void> setSystemSelectionChannelsResponse(const RadioResponseInfo& info);

    Return<void> enableModemResponse(const RadioResponseInfo& info);

    Return<void> getModemStackStatusResponse(const RadioResponseInfo& info, const bool enabled);

    /* 1.4 Api */
    Return<void> emergencyDialResponse(const RadioResponseInfo& info);

    Return<void> startNetworkScanResponse_1_4(const RadioResponseInfo& info);

    Return<void> getCellInfoListResponse_1_4(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_4::CellInfo>&
                    cellInfo);

    Return<void> getDataRegistrationStateResponse_1_4(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_4::DataRegStateResult& dataRegResponse);

    Return<void> getIccCardStatusResponse_1_4(
            const RadioResponseInfo& info,
            const ::android::hardware::radio::V1_4::CardStatus& card_status);

    Return<void> getPreferredNetworkTypeBitmapResponse(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_bitfield<
                    ::android::hardware::radio::V1_4::RadioAccessFamily>
                    networkTypeBitmap);

    Return<void> setPreferredNetworkTypeBitmapResponse(const RadioResponseInfo& info);

    Return<void> getDataCallListResponse_1_4(
            const RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<
                    ::android::hardware::radio::V1_4::SetupDataCallResult>& dcResponse);

    Return<void> setupDataCallResponse_1_4(
            const RadioResponseInfo& info,
            const android::hardware::radio::V1_4::SetupDataCallResult& dcResponse);

    Return<void> setAllowedCarriersResponse_1_4(const RadioResponseInfo& info);

    Return<void> getAllowedCarriersResponse_1_4(const RadioResponseInfo& info,
                                                const CarrierRestrictionsWithPriority& carriers,
                                                SimLockMultiSimPolicy multiSimPolicy);

    /* 1.5 Api */
    Return<void> setSignalStrengthReportingCriteriaResponse_1_5(const RadioResponseInfo& info);

    Return<void> enableUiccApplicationsResponse(const RadioResponseInfo& info);

    Return<void> areUiccApplicationsEnabledResponse(const RadioResponseInfo& info, bool enabled);

    Return<void> canToggleUiccApplicationsEnablementResponse(const RadioResponseInfo& info,
                                                             bool canToggle);

    Return<void> setSystemSelectionChannelsResponse_1_5(const RadioResponseInfo& info);

    Return<void> startNetworkScanResponse_1_5(const RadioResponseInfo& info);

    Return<void> setupDataCallResponse_1_5(
            const RadioResponseInfo& info,
            const android::hardware::radio::V1_5::SetupDataCallResult& dcResponse);

    Return<void> setInitialAttachApnResponse_1_5(const RadioResponseInfo& info);

    Return<void> setDataProfileResponse_1_5(const RadioResponseInfo& info);

    Return<void> setRadioPowerResponse_1_5(const RadioResponseInfo& info);
};

/* Callback class for radio indication */
class RadioIndication_v1_5 : public ::android::hardware::radio::V1_5::IRadioIndication {
  protected:
    RadioHidlTest_v1_5& parent_v1_5;

  public:
    RadioIndication_v1_5(RadioHidlTest_v1_5& parent_v1_5);
    virtual ~RadioIndication_v1_5() = default;

    /* 1.5 Api */
    Return<void> uiccApplicationsEnablementChanged(RadioIndicationType type, bool enabled);

    /* 1.4 Api */
    Return<void> currentEmergencyNumberList(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<EmergencyNumber>& emergencyNumberList);

    Return<void> cellInfoList_1_4(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_4::CellInfo>&
                    records);

    Return<void> networkScanResult_1_4(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_4::NetworkScanResult& result);

    Return<void> currentPhysicalChannelConfigs_1_4(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<
                    ::android::hardware::radio::V1_4::PhysicalChannelConfig>& configs);

    Return<void> dataCallListChanged_1_4(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<
                    android::hardware::radio::V1_4::SetupDataCallResult>& dcList);

    /* 1.2 Api */
    Return<void> networkScanResult_1_2(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_2::NetworkScanResult& result);

    Return<void> cellInfoList_1_2(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::CellInfo>&
                    records);

    Return<void> currentLinkCapacityEstimate(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_2::LinkCapacityEstimate& lce);

    Return<void> currentPhysicalChannelConfigs(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<
                    ::android::hardware::radio::V1_2::PhysicalChannelConfig>& configs);

    Return<void> currentSignalStrength_1_2(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_2::SignalStrength& signalStrength);

    Return<void> currentSignalStrength_1_4(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_4::SignalStrength& signalStrength);

    /* 1.1 Api */
    Return<void> carrierInfoForImsiEncryption(RadioIndicationType info);

    Return<void> networkScanResult(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_1::NetworkScanResult& result);

    Return<void> keepaliveStatus(RadioIndicationType type, const KeepaliveStatus& status);

    /* 1.0 Api */
    Return<void> radioStateChanged(RadioIndicationType type, RadioState radioState);

    Return<void> callStateChanged(RadioIndicationType type);

    Return<void> networkStateChanged(RadioIndicationType type);

    Return<void> newSms(RadioIndicationType type,
                        const ::android::hardware::hidl_vec<uint8_t>& pdu);

    Return<void> newSmsStatusReport(RadioIndicationType type,
                                    const ::android::hardware::hidl_vec<uint8_t>& pdu);

    Return<void> newSmsOnSim(RadioIndicationType type, int32_t recordNumber);

    Return<void> onUssd(RadioIndicationType type, UssdModeType modeType,
                        const ::android::hardware::hidl_string& msg);

    Return<void> nitzTimeReceived(RadioIndicationType type,
                                  const ::android::hardware::hidl_string& nitzTime,
                                  uint64_t receivedTime);

    Return<void> currentSignalStrength(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_0::SignalStrength& signalStrength);

    Return<void> dataCallListChanged(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<
                    android::hardware::radio::V1_0::SetupDataCallResult>& dcList);

    Return<void> suppSvcNotify(RadioIndicationType type, const SuppSvcNotification& suppSvc);

    Return<void> stkSessionEnd(RadioIndicationType type);

    Return<void> stkProactiveCommand(RadioIndicationType type,
                                     const ::android::hardware::hidl_string& cmd);

    Return<void> stkEventNotify(RadioIndicationType type,
                                const ::android::hardware::hidl_string& cmd);

    Return<void> stkCallSetup(RadioIndicationType type, int64_t timeout);

    Return<void> simSmsStorageFull(RadioIndicationType type);

    Return<void> simRefresh(RadioIndicationType type, const SimRefreshResult& refreshResult);

    Return<void> callRing(RadioIndicationType type, bool isGsm, const CdmaSignalInfoRecord& record);

    Return<void> simStatusChanged(RadioIndicationType type);

    Return<void> cdmaNewSms(RadioIndicationType type, const CdmaSmsMessage& msg);

    Return<void> newBroadcastSms(RadioIndicationType type,
                                 const ::android::hardware::hidl_vec<uint8_t>& data);

    Return<void> cdmaRuimSmsStorageFull(RadioIndicationType type);

    Return<void> restrictedStateChanged(RadioIndicationType type, PhoneRestrictedState state);

    Return<void> enterEmergencyCallbackMode(RadioIndicationType type);

    Return<void> cdmaCallWaiting(RadioIndicationType type,
                                 const CdmaCallWaiting& callWaitingRecord);

    Return<void> cdmaOtaProvisionStatus(RadioIndicationType type, CdmaOtaProvisionStatus status);

    Return<void> cdmaInfoRec(RadioIndicationType type, const CdmaInformationRecords& records);

    Return<void> indicateRingbackTone(RadioIndicationType type, bool start);

    Return<void> resendIncallMute(RadioIndicationType type);

    Return<void> cdmaSubscriptionSourceChanged(RadioIndicationType type,
                                               CdmaSubscriptionSource cdmaSource);

    Return<void> cdmaPrlChanged(RadioIndicationType type, int32_t version);

    Return<void> exitEmergencyCallbackMode(RadioIndicationType type);

    Return<void> rilConnected(RadioIndicationType type);

    Return<void> voiceRadioTechChanged(RadioIndicationType type,
                                       ::android::hardware::radio::V1_0::RadioTechnology rat);

    Return<void> cellInfoList(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::CellInfo>&
                    records);

    Return<void> imsNetworkStateChanged(RadioIndicationType type);

    Return<void> subscriptionStatusChanged(RadioIndicationType type, bool activate);

    Return<void> srvccStateNotify(RadioIndicationType type, SrvccState state);

    Return<void> hardwareConfigChanged(
            RadioIndicationType type, const ::android::hardware::hidl_vec<HardwareConfig>& configs);

    Return<void> radioCapabilityIndication(
            RadioIndicationType type, const android::hardware::radio::V1_0::RadioCapability& rc);

    Return<void> onSupplementaryServiceIndication(RadioIndicationType type,
                                                  const StkCcUnsolSsResult& ss);

    Return<void> stkCallControlAlphaNotify(RadioIndicationType type,
                                           const ::android::hardware::hidl_string& alpha);

    Return<void> lceData(RadioIndicationType type, const LceDataInfo& lce);

    Return<void> pcoData(RadioIndicationType type, const PcoDataInfo& pco);

    Return<void> modemReset(RadioIndicationType type,
                            const ::android::hardware::hidl_string& reason);

    Return<void> registrationFailed(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_5::CellIdentity& cellIdentity,
            const ::android::hardware::hidl_string& chosenPlmn,
            ::android::hardware::hidl_bitfield<::android::hardware::radio::V1_5::Domain> domain,
            int32_t causeCode, int32_t additionalCauseCode);
};

// Test environment for Radio HIDL HAL.
class RadioHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
  public:
    // get the test environment singleton
    static RadioHidlEnvironment* Instance() {
        static RadioHidlEnvironment* instance = new RadioHidlEnvironment;
        return instance;
    }
    virtual void registerTestServices() override {
        registerTestService<::android::hardware::radio::V1_5::IRadio>();
    }

  private:
    RadioHidlEnvironment() {}
};

// The main test class for Radio HIDL.
class RadioHidlTest_v1_5 : public ::testing::VtsHalHidlTargetTestBase {
  protected:
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_;

    /* Serial number for radio request */
    int serial;

    /* Clear Potential Established Calls */
    void clearPotentialEstablishedCalls();

    /* Update Sim Card Status */
    void updateSimCardStatus();

  public:
    virtual void SetUp() override;

    /* Used as a mechanism to inform the test about data/event callback */
    void notify(int receivedSerial);

    /* Test code calls this function to wait for response */
    std::cv_status wait();

    /* radio service handle */
    sp<::android::hardware::radio::V1_5::IRadio> radio_v1_5;

    /* radio response handle */
    sp<RadioResponse_v1_5> radioRsp_v1_5;

    /* radio indication handle */
    sp<RadioIndication_v1_5> radioInd_v1_5;
};
