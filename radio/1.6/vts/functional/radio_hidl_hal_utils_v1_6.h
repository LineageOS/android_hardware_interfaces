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

#pragma once

#include <android-base/logging.h>

#include "radio_config_hidl_hal_utils.h"

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <android/hardware/radio/1.6/IRadio.h>
#include <android/hardware/radio/1.6/IRadioIndication.h>
#include <android/hardware/radio/1.6/IRadioResponse.h>
#include <android/hardware/radio/1.6/types.h>

#include "vts_test_util_v1_6.h"

using namespace ::android::hardware::radio::V1_6;
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

#define MODEM_EMERGENCY_CALL_ESTABLISH_TIME 3
#define MODEM_EMERGENCY_CALL_DISCONNECT_TIME 3
#define MODEM_SET_SIM_POWER_DELAY_IN_SECONDS 2

#define RADIO_SERVICE_SLOT1_NAME "slot1"  // HAL instance name for SIM slot 1 or single SIM device
#define RADIO_SERVICE_SLOT2_NAME "slot2"  // HAL instance name for SIM slot 2 on dual SIM device
#define RADIO_SERVICE_SLOT3_NAME "slot3"  // HAL instance name for SIM slot 3 on triple SIM device

class RadioHidlTest_v1_6;
extern ::android::hardware::radio::V1_5::CardStatus cardStatus;

/* Callback class for radio response v1_6 */
class RadioResponse_v1_6 : public ::android::hardware::radio::V1_6::IRadioResponse {
  protected:
    RadioResponseWaiter& parent_v1_6;

  public:
    hidl_vec<RadioBandMode> radioBandModes;
    hidl_vec<OperatorInfo> networkInfos;

    ::android::hardware::radio::V1_0::RadioResponseInfo rspInfo_v1_0;
    ::android::hardware::radio::V1_6::RadioResponseInfo rspInfo;

    // Call
    hidl_vec<::android::hardware::radio::V1_6::Call> currentCalls;
    ::android::hardware::radio::V1_2::VoiceRegStateResult voiceRegResp;

    // Sms
    SendSmsResult sendSmsResult;

    // Modem
    bool isModemEnabled;
    bool enableModemResponseToggle;
    bool isNRDualConnectivityEnabled;

    // Pdu Session Id and Handover
    int32_t allocatedPduSessionId;

    ::android::hardware::hidl_bitfield<::android::hardware::radio::V1_4::RadioAccessFamily>
            networkTypeBitmapResponse;

    // Data
    ::android::hardware::radio::V1_4::DataRegStateResult dataRegResp;
    ::android::hardware::radio::V1_6::SetupDataCallResult setupDataCallResult;

    // SimLock status
    ::android::hardware::radio::V1_4::CarrierRestrictionsWithPriority carrierRestrictionsResp;
    ::android::hardware::radio::V1_4::SimLockMultiSimPolicy multiSimPolicyResp;

    // Whether toggling uicc applications operation is supported.
    bool canToggleUiccApplicationsEnablement;

    // Whether Uicc applications are enabled or not.
    bool areUiccApplicationsEnabled;

    // Barring Info Response
    ::android::hardware::radio::V1_5::CellIdentity barringCellIdentity;
    ::android::hardware::hidl_vec<::android::hardware::radio::V1_5::BarringInfo> barringInfos;

    RadioResponse_v1_6(RadioResponseWaiter& parent_v1_6);

    // Phone Book
    ::android::hardware::radio::V1_6::PhonebookCapacity capacity;
    int32_t updatedRecordIndex;

    virtual ~RadioResponse_v1_6() = default;

    Return<void> getIccCardStatusResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_0::CardStatus& cardStatus);

    Return<void> supplyIccPinForAppResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            int32_t remainingRetries);

    Return<void> supplyIccPukForAppResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            int32_t remainingRetries);

    Return<void> supplyIccPin2ForAppResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            int32_t remainingRetries);

    Return<void> supplyIccPuk2ForAppResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            int32_t remainingRetries);

    Return<void> changeIccPinForAppResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            int32_t remainingRetries);

    Return<void> changeIccPin2ForAppResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            int32_t remainingRetries);

    Return<void> supplyNetworkDepersonalizationResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            int32_t remainingRetries);

    Return<void> getCurrentCallsResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::Call>& calls);

    Return<void> dialResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getIMSIForAppResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_string& imsi);

    Return<void> hangupConnectionResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> hangupWaitingOrBackgroundResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> hangupForegroundResumeBackgroundResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> switchWaitingOrHoldingAndActiveResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> conferenceResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> rejectCallResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getLastCallFailCauseResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const LastCallFailCauseInfo& failCauseInfo);

    Return<void> getSignalStrengthResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_0::SignalStrength& sigStrength);

    Return<void> getVoiceRegistrationStateResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_0::VoiceRegStateResult& voiceRegResponse);

    Return<void> getDataRegistrationStateResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_0::DataRegStateResult& dataRegResponse);

    Return<void> getOperatorResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_string& longName,
            const ::android::hardware::hidl_string& shortName,
            const ::android::hardware::hidl_string& numeric);

    Return<void> setRadioPowerResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> sendDtmfResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> sendSmsResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
                                 const SendSmsResult& sms);

    Return<void> sendSMSExpectMoreResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const SendSmsResult& sms);

    Return<void> setupDataCallResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const android::hardware::radio::V1_0::SetupDataCallResult& dcResponse);

    Return<void> iccIOForAppResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const IccIoResult& iccIo);

    Return<void> sendUssdResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> cancelPendingUssdResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getClirResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
                                 int32_t n, int32_t m);

    Return<void> setClirResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getCallForwardStatusResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<CallForwardInfo>& call_forwardInfos);

    Return<void> setCallForwardResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getCallWaitingResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, bool enable,
            int32_t serviceClass);

    Return<void> setCallWaitingResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> acknowledgeLastIncomingGsmSmsResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> acceptCallResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> deactivateDataCallResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getFacilityLockForAppResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, int32_t response);

    Return<void> setFacilityLockForAppResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, int32_t retry);

    Return<void> setBarringPasswordResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getNetworkSelectionModeResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, bool manual);

    Return<void> setNetworkSelectionModeAutomaticResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setNetworkSelectionModeManualResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getAvailableNetworksResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<OperatorInfo>& networkInfos);

    Return<void> startDtmfResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> stopDtmfResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getBasebandVersionResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_string& version);

    Return<void> separateConnectionResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setMuteResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getMuteResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
                                 bool enable);

    Return<void> getClipResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
                                 ClipStatus status);

    Return<void> getDataCallListResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<
                    android::hardware::radio::V1_0::SetupDataCallResult>& dcResponse);

    Return<void> sendOemRilRequestRawResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<uint8_t>& data);

    Return<void> sendOemRilRequestStringsResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& data);

    Return<void> setSuppServiceNotificationsResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> writeSmsToSimResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, int32_t index);

    Return<void> deleteSmsOnSimResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setBandModeResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getAvailableBandModesResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<RadioBandMode>& bandModes);

    Return<void> sendEnvelopeResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_string& commandResponse);

    Return<void> sendTerminalResponseToSimResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> handleStkCallSetupRequestFromSimResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> explicitCallTransferResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setPreferredNetworkTypeResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getPreferredNetworkTypeResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            PreferredNetworkType nwType);

    Return<void> getNeighboringCidsResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<NeighboringCell>& cells);

    Return<void> setLocationUpdatesResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setCdmaSubscriptionSourceResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setCdmaRoamingPreferenceResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getCdmaRoamingPreferenceResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, CdmaRoamingType type);

    Return<void> setTTYModeResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getTTYModeResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
                                    TtyMode mode);

    Return<void> setPreferredVoicePrivacyResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getPreferredVoicePrivacyResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, bool enable);

    Return<void> sendCDMAFeatureCodeResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> sendBurstDtmfResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> sendCdmaSmsResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const SendSmsResult& sms);

    Return<void> acknowledgeLastIncomingCdmaSmsResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getGsmBroadcastConfigResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<GsmBroadcastSmsConfigInfo>& configs);

    Return<void> setGsmBroadcastConfigResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setGsmBroadcastActivationResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getCdmaBroadcastConfigResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<CdmaBroadcastSmsConfigInfo>& configs);

    Return<void> setCdmaBroadcastConfigResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setCdmaBroadcastActivationResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getCDMASubscriptionResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_string& mdn,
            const ::android::hardware::hidl_string& hSid,
            const ::android::hardware::hidl_string& hNid,
            const ::android::hardware::hidl_string& min,
            const ::android::hardware::hidl_string& prl);

    Return<void> writeSmsToRuimResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, uint32_t index);

    Return<void> deleteSmsOnRuimResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getDeviceIdentityResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_string& imei,
            const ::android::hardware::hidl_string& imeisv,
            const ::android::hardware::hidl_string& esn,
            const ::android::hardware::hidl_string& meid);

    Return<void> exitEmergencyCallbackModeResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getSmscAddressResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_string& smsc);

    Return<void> setSmscAddressResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> reportSmsMemoryStatusResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> reportStkServiceIsRunningResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getCdmaSubscriptionSourceResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            CdmaSubscriptionSource source);

    Return<void> requestIsimAuthenticationResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_string& response);

    Return<void> acknowledgeIncomingGsmSmsWithPduResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> sendEnvelopeWithStatusResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const IccIoResult& iccIo);

    Return<void> getVoiceRadioTechnologyResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            ::android::hardware::radio::V1_0::RadioTechnology rat);

    Return<void> getCellInfoListResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::CellInfo>&
                    cellInfo);

    Return<void> setCellInfoListRateResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setInitialAttachApnResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getImsRegistrationStateResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, bool isRegistered,
            RadioTechnologyFamily ratFamily);

    Return<void> sendImsSmsResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
                                    const SendSmsResult& sms);

    Return<void> iccTransmitApduBasicChannelResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const IccIoResult& result);

    Return<void> iccOpenLogicalChannelResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, int32_t channelId,
            const ::android::hardware::hidl_vec<int8_t>& selectResponse);

    Return<void> iccCloseLogicalChannelResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> iccTransmitApduLogicalChannelResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const IccIoResult& result);

    Return<void> nvReadItemResponse(const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
                                    const ::android::hardware::hidl_string& result);

    Return<void> nvWriteItemResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> nvWriteCdmaPrlResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> nvResetConfigResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setUiccSubscriptionResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setDataAllowedResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getHardwareConfigResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<HardwareConfig>& config);

    Return<void> requestIccSimAuthenticationResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const IccIoResult& result);

    Return<void> setDataProfileResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> requestShutdownResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getRadioCapabilityResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const android::hardware::radio::V1_0::RadioCapability& rc);

    Return<void> setRadioCapabilityResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const android::hardware::radio::V1_0::RadioCapability& rc);

    Return<void> startLceServiceResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const LceStatusInfo& statusInfo);

    Return<void> stopLceServiceResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const LceStatusInfo& statusInfo);

    Return<void> pullLceDataResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const LceDataInfo& lceInfo);

    Return<void> getModemActivityInfoResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ActivityStatsInfo& activityInfo);

    Return<void> setAllowedCarriersResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, int32_t numAllowed);

    Return<void> getAllowedCarriersResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, bool allAllowed,
            const CarrierRestrictions& carriers);

    Return<void> sendDeviceStateResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setIndicationFilterResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setSimCardPowerResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> acknowledgeRequest(int32_t serial);

    /* 1.1 Api */
    Return<void> setCarrierInfoForImsiEncryptionResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setSimCardPowerResponse_1_1(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> startNetworkScanResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> stopNetworkScanResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> startKeepaliveResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const KeepaliveStatus& status);

    Return<void> stopKeepaliveResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    /* 1.2 Api */
    Return<void> setSignalStrengthReportingCriteriaResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setLinkCapacityReportingCriteriaResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getIccCardStatusResponse_1_2(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_2::CardStatus& card_status);

    Return<void> getCurrentCallsResponse_1_2(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::Call>& calls);

    Return<void> getSignalStrengthResponse_1_2(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_2::SignalStrength& sig_strength);

    Return<void> getSignalStrengthResponse_1_4(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_4::SignalStrength& sig_strength);

    Return<void> getCellInfoListResponse_1_2(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::CellInfo>&
                    cellInfo);

    Return<void> getVoiceRegistrationStateResponse_1_2(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_2::VoiceRegStateResult& voiceRegResponse);

    Return<void> getDataRegistrationStateResponse_1_2(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_2::DataRegStateResult& dataRegResponse);

    /* 1.3 Api */
    Return<void> setSystemSelectionChannelsResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> enableModemResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getModemStackStatusResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, const bool enabled);

    /* 1.4 Api */
    Return<void> emergencyDialResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> startNetworkScanResponse_1_4(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getCellInfoListResponse_1_4(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_4::CellInfo>&
                    cellInfo);

    Return<void> getDataRegistrationStateResponse_1_4(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_4::DataRegStateResult& dataRegResponse);

    Return<void> getIccCardStatusResponse_1_4(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_4::CardStatus& card_status);

    Return<void> getPreferredNetworkTypeBitmapResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_bitfield<
                    ::android::hardware::radio::V1_4::RadioAccessFamily>
                    networkTypeBitmap);

    Return<void> setPreferredNetworkTypeBitmapResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getDataCallListResponse_1_4(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<
                    ::android::hardware::radio::V1_4::SetupDataCallResult>& dcResponse);

    Return<void> setupDataCallResponse_1_4(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const android::hardware::radio::V1_4::SetupDataCallResult& dcResponse);

    Return<void> setAllowedCarriersResponse_1_4(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getAllowedCarriersResponse_1_4(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const CarrierRestrictionsWithPriority& carriers, SimLockMultiSimPolicy multiSimPolicy);

    /* 1.5 Api */
    Return<void> setSignalStrengthReportingCriteriaResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setLinkCapacityReportingCriteriaResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> enableUiccApplicationsResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> areUiccApplicationsEnabledResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, bool enabled);

    Return<void> canToggleUiccApplicationsEnablementResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info, bool canToggle);

    Return<void> setSystemSelectionChannelsResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> startNetworkScanResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setupDataCallResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const android::hardware::radio::V1_5::SetupDataCallResult& dcResponse);

    Return<void> getDataCallListResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const hidl_vec<::android::hardware::radio::V1_5::SetupDataCallResult>& dcResponse);

    Return<void> setInitialAttachApnResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setDataProfileResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setRadioPowerResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> setIndicationFilterResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> getBarringInfoResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_5::CellIdentity& cellIdentity,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_5::BarringInfo>&
                    barringInfos);

    Return<void> getVoiceRegistrationStateResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_5::RegStateResult& regResponse);

    Return<void> getDataRegistrationStateResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_5::RegStateResult& regResponse);

    Return<void> getCellInfoListResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_5::CellInfo>&
                    cellInfo);

    Return<void> setNetworkSelectionModeManualResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info);

    Return<void> sendCdmaSmsExpectMoreResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const SendSmsResult& sms);

    Return<void> supplySimDepersonalizationResponse(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            ::android::hardware::radio::V1_5::PersoSubstate persoType, int32_t remainingRetries);

    Return<void> getIccCardStatusResponse_1_5(
            const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_5::CardStatus& card_status);

    /* 1.6 Api */
    Return<void> setRadioPowerResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info);

    Return<void> setupDataCallResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const android::hardware::radio::V1_6::SetupDataCallResult& dcResponse);

    Return<void> getDataCallListResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const hidl_vec<::android::hardware::radio::V1_6::SetupDataCallResult>& dcResponse);

    Return<void> sendSmsResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const SendSmsResult& sms);

    Return<void> sendSmsExpectMoreResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const SendSmsResult& sms);

    Return<void> sendCdmaSmsResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const SendSmsResult& sms);

    Return<void> setSimCardPowerResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info);

    Return<void> sendCdmaSmsExpectMoreResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const SendSmsResult& sms);

    Return<void> setNrDualConnectivityStateResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info);
    Return<void> isNrDualConnectivityEnabledResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info, bool isEnabled);

    Return<void> allocatePduSessionIdResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info, int32_t id);

    Return<void> releasePduSessionIdResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info);

    Return<void> startHandoverResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info);

    Return<void> cancelHandoverResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info);

    Return<void> setAllowedNetworkTypesBitmapResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info);

    Return<void> getAllowedNetworkTypesBitmapResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const ::android::hardware::hidl_bitfield<
                    ::android::hardware::radio::V1_4::RadioAccessFamily>
                    networkTypeBitmap);

    Return<void> setDataThrottlingResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info);

    Return<void> getSystemSelectionChannelsResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const hidl_vec<::android::hardware::radio::V1_5::RadioAccessSpecifier>& specifier);

    Return<void> getSignalStrengthResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_6::SignalStrength& sig_strength);

    Return<void> getCellInfoListResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_6::CellInfo>&
                    cellInfo);

    Return<void> getVoiceRegistrationStateResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_6::RegStateResult& regResponse);

    Return<void> getDataRegistrationStateResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_6::RegStateResult& regResponse);

    Return<void> getCurrentCallsResponse_1_6(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_6::Call>& calls);

    Return<void> getSlicingConfigResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_6::SlicingConfig& slicingConfig);

    Return<void> getSimPhonebookRecordsResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info);

    Return<void> getSimPhonebookCapacityResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            const ::android::hardware::radio::V1_6::PhonebookCapacity& capacity);

    Return<void> updateSimPhonebookRecordsResponse(
            const ::android::hardware::radio::V1_6::RadioResponseInfo& info,
            int32_t updatedRecordIndex);
};

/* Callback class for radio indication */
class RadioIndication_v1_6 : public ::android::hardware::radio::V1_6::IRadioIndication {
  protected:
    RadioHidlTest_v1_6& parent_v1_6;

  public:
    RadioIndication_v1_6(RadioHidlTest_v1_6& parent_v1_6);
    virtual ~RadioIndication_v1_6() = default;

    /* 1.6 Api */
    Return<void> dataCallListChanged_1_6(
            RadioIndicationType type,
            const hidl_vec<::android::hardware::radio::V1_6::SetupDataCallResult>& dcList);

    Return<void> unthrottleApn(RadioIndicationType type,
                               const ::android::hardware::hidl_string& apn);

    Return<void> currentSignalStrength_1_6(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_6::SignalStrength& signalStrength);

    Return<void> networkScanResult_1_6(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_6::NetworkScanResult& result);

    Return<void> cellInfoList_1_6(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_6::CellInfo>&
                    records);

    Return<void> currentPhysicalChannelConfigs_1_6(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<
                    ::android::hardware::radio::V1_6::PhysicalChannelConfig>& configs);

    /* 1.5 Api */
    Return<void> uiccApplicationsEnablementChanged(RadioIndicationType type, bool enabled);

    Return<void> networkScanResult_1_5(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_5::NetworkScanResult& result);

    Return<void> cellInfoList_1_5(
            RadioIndicationType type,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_5::CellInfo>&
                    records);

    Return<void> dataCallListChanged_1_5(
            RadioIndicationType type,
            const hidl_vec<::android::hardware::radio::V1_5::SetupDataCallResult>& dcList);

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

    Return<void> currentLinkCapacityEstimate_1_6(
            RadioIndicationType type,
            const ::android::hardware::radio::V1_6::LinkCapacityEstimate& lce);

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

    Return<void> barringInfoChanged(
            RadioIndicationType /*type*/,
            const ::android::hardware::radio::V1_5::CellIdentity& /*cellIdentity*/,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_5::BarringInfo>&
            /*barringInfos*/);

    Return<void> simPhonebookChanged(RadioIndicationType type);

    Return<void> simPhonebookRecordsReceived(
            RadioIndicationType type,
            ::android::hardware::radio::V1_6::PbReceivedStatus status,
            const ::android::hardware::hidl_vec<::android::hardware::radio::V1_6::PhonebookRecordInfo>&
                    records);
};

// The main test class for Radio HIDL.
class RadioHidlTest_v1_6 : public ::testing::TestWithParam<std::string>,
                           public RadioResponseWaiter {
  protected:
    /* Clear Potential Established Calls */
    void clearPotentialEstablishedCalls();

    /* Update Sim Card Status */
    void updateSimCardStatus();

    /* Get current data call list */
    void getDataCallList();

  public:
    virtual void SetUp() override;

    bool getRadioHalCapabilities();

    /* radio service handle */
    sp<::android::hardware::radio::V1_6::IRadio> radio_v1_6;

    /* radio response handle */
    sp<RadioResponse_v1_6> radioRsp_v1_6;

    /* radio indication handle */
    sp<RadioIndication_v1_6> radioInd_v1_6;
};
