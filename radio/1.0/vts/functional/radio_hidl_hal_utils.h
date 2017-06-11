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

#include <android-base/logging.h>

#include <VtsHalHidlTargetTestBase.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <android/hardware/radio/1.0/IRadio.h>
#include <android/hardware/radio/1.0/IRadioIndication.h>
#include <android/hardware/radio/1.0/IRadioResponse.h>
#include <android/hardware/radio/1.0/types.h>

#include <vts_test_util.h>

using ::android::hardware::radio::V1_0::ActivityStatsInfo;
using ::android::hardware::radio::V1_0::AppType;
using ::android::hardware::radio::V1_0::CardStatus;
using ::android::hardware::radio::V1_0::CardState;
using ::android::hardware::radio::V1_0::Call;
using ::android::hardware::radio::V1_0::CallForwardInfo;
using ::android::hardware::radio::V1_0::CarrierMatchType;
using ::android::hardware::radio::V1_0::CarrierRestrictions;
using ::android::hardware::radio::V1_0::CdmaRoamingType;
using ::android::hardware::radio::V1_0::CdmaBroadcastSmsConfigInfo;
using ::android::hardware::radio::V1_0::CdmaSubscriptionSource;
using ::android::hardware::radio::V1_0::CellInfo;
using ::android::hardware::radio::V1_0::ClipStatus;
using ::android::hardware::radio::V1_0::DataRegStateResult;
using ::android::hardware::radio::V1_0::DeviceStateType;
using ::android::hardware::radio::V1_0::Dial;
using ::android::hardware::radio::V1_0::GsmBroadcastSmsConfigInfo;
using ::android::hardware::radio::V1_0::HardwareConfig;
using ::android::hardware::radio::V1_0::IccIo;
using ::android::hardware::radio::V1_0::IccIoResult;
using ::android::hardware::radio::V1_0::IRadio;
using ::android::hardware::radio::V1_0::IRadioResponse;
using ::android::hardware::radio::V1_0::IRadioIndication;
using ::android::hardware::radio::V1_0::RadioConst;
using ::android::hardware::radio::V1_0::RadioError;
using ::android::hardware::radio::V1_0::RadioResponseInfo;
using ::android::hardware::radio::V1_0::LastCallFailCauseInfo;
using ::android::hardware::radio::V1_0::LceDataInfo;
using ::android::hardware::radio::V1_0::LceStatusInfo;
using ::android::hardware::radio::V1_0::NeighboringCell;
using ::android::hardware::radio::V1_0::NvItem;
using ::android::hardware::radio::V1_0::NvWriteItem;
using ::android::hardware::radio::V1_0::OperatorInfo;
using ::android::hardware::radio::V1_0::PreferredNetworkType;
using ::android::hardware::radio::V1_0::RadioBandMode;
using ::android::hardware::radio::V1_0::RadioCapability;
using ::android::hardware::radio::V1_0::RadioResponseType;
using ::android::hardware::radio::V1_0::RadioTechnology;
using ::android::hardware::radio::V1_0::RadioTechnologyFamily;
using ::android::hardware::radio::V1_0::ResetNvType;
using ::android::hardware::radio::V1_0::SelectUiccSub;
using ::android::hardware::radio::V1_0::SendSmsResult;
using ::android::hardware::radio::V1_0::SetupDataCallResult;
using ::android::hardware::radio::V1_0::SignalStrength;
using ::android::hardware::radio::V1_0::SimApdu;
using ::android::hardware::radio::V1_0::TtyMode;
using ::android::hardware::radio::V1_0::VoiceRegStateResult;

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

#define TIMEOUT_PERIOD 75
#define RADIO_SERVICE_NAME "slot1"

class RadioHidlTest;
extern CardStatus cardStatus;

/* Callback class for radio response */
class RadioResponse : public IRadioResponse {
   private:
    RadioHidlTest& parent;

   public:
    RadioResponseInfo rspInfo;
    hidl_string imsi;
    IccIoResult iccIoResult;
    int channelId;

    // Sms
    SendSmsResult sendSmsResult;
    hidl_string smscAddress;
    uint32_t writeSmsToSimIndex;
    uint32_t writeSmsToRuimIndex;

    RadioResponse(RadioHidlTest& parent);

    virtual ~RadioResponse() = default;

    Return<void> getIccCardStatusResponse(const RadioResponseInfo& info,
                                          const CardStatus& cardStatus);

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

    Return<void> getCurrentCallsResponse(const RadioResponseInfo& info,
                                         const ::android::hardware::hidl_vec<Call>& calls);

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

    Return<void> getSignalStrengthResponse(const RadioResponseInfo& info,
                                           const SignalStrength& sigStrength);

    Return<void> getVoiceRegistrationStateResponse(const RadioResponseInfo& info,
                                                   const VoiceRegStateResult& voiceRegResponse);

    Return<void> getDataRegistrationStateResponse(const RadioResponseInfo& info,
                                                  const DataRegStateResult& dataRegResponse);

    Return<void> getOperatorResponse(const RadioResponseInfo& info,
                                     const ::android::hardware::hidl_string& longName,
                                     const ::android::hardware::hidl_string& shortName,
                                     const ::android::hardware::hidl_string& numeric);

    Return<void> setRadioPowerResponse(const RadioResponseInfo& info);

    Return<void> sendDtmfResponse(const RadioResponseInfo& info);

    Return<void> sendSmsResponse(const RadioResponseInfo& info, const SendSmsResult& sms);

    Return<void> sendSMSExpectMoreResponse(const RadioResponseInfo& info, const SendSmsResult& sms);

    Return<void> setupDataCallResponse(const RadioResponseInfo& info,
                                       const SetupDataCallResult& dcResponse);

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
        const ::android::hardware::hidl_vec<SetupDataCallResult>& dcResponse);

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
        const RadioResponseInfo& info, const ::android::hardware::hidl_vec<NeighboringCell>& cells);

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

    Return<void> getVoiceRadioTechnologyResponse(const RadioResponseInfo& info,
                                                 RadioTechnology rat);

    Return<void> getCellInfoListResponse(const RadioResponseInfo& info,
                                         const ::android::hardware::hidl_vec<CellInfo>& cellInfo);

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
        const RadioResponseInfo& info, const ::android::hardware::hidl_vec<HardwareConfig>& config);

    Return<void> requestIccSimAuthenticationResponse(const RadioResponseInfo& info,
                                                     const IccIoResult& result);

    Return<void> setDataProfileResponse(const RadioResponseInfo& info);

    Return<void> requestShutdownResponse(const RadioResponseInfo& info);

    Return<void> getRadioCapabilityResponse(const RadioResponseInfo& info,
                                            const RadioCapability& rc);

    Return<void> setRadioCapabilityResponse(const RadioResponseInfo& info,
                                            const RadioCapability& rc);

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
};

// The main test class for Radio HIDL.
class RadioHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;

   public:
    virtual void SetUp() override;

    virtual void TearDown() override;

    /* Used as a mechanism to inform the test about data/event callback */
    void notify();

    /* Test code calls this function to wait for response */
    std::cv_status wait();

    /* Used for checking General Errors */
    bool CheckGeneralError();

    /* Used for checking OEM Errors */
    bool CheckOEMError();

    sp<IRadio> radio;
    sp<RadioResponse> radioRsp;
    sp<IRadioIndication> radioInd;
};

// A class for test environment setup
class RadioHidlEnvironment : public ::testing::Environment {
   public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};