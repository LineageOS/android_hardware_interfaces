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

#include <aidl/android/hardware/radio/voice/BnRadioVoiceIndication.h>
#include <aidl/android/hardware/radio/voice/BnRadioVoiceResponse.h>
#include <aidl/android/hardware/radio/voice/IRadioVoice.h>

#include "radio_aidl_hal_utils.h"
#include "radio_network_utils.h"

using namespace aidl::android::hardware::radio::voice;

class RadioVoiceTest;

/* Callback class for radio voice response */
class RadioVoiceResponse : public BnRadioVoiceResponse {
  protected:
    RadioServiceTest& parent_voice;

  public:
    RadioVoiceResponse(RadioServiceTest& parent_voice);
    virtual ~RadioVoiceResponse() = default;

    RadioResponseInfo rspInfo;
    std::vector<Call> currentCalls;

    virtual ndk::ScopedAStatus acceptCallResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus acknowledgeRequest(int32_t serial) override;

    virtual ndk::ScopedAStatus cancelPendingUssdResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus conferenceResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus dialResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus emergencyDialResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus exitEmergencyCallbackModeResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus explicitCallTransferResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus getCallForwardStatusResponse(
            const RadioResponseInfo& info,
            const std::vector<CallForwardInfo>& call_forwardInfos) override;

    virtual ndk::ScopedAStatus getCallWaitingResponse(const RadioResponseInfo& info, bool enable,
                                                      int32_t serviceClass) override;

    virtual ndk::ScopedAStatus getClipResponse(const RadioResponseInfo& info,
                                               ClipStatus status) override;

    virtual ndk::ScopedAStatus getClirResponse(const RadioResponseInfo& info, int32_t n,
                                               int32_t m) override;

    virtual ndk::ScopedAStatus getCurrentCallsResponse(const RadioResponseInfo& info,
                                                       const std::vector<Call>& calls) override;

    virtual ndk::ScopedAStatus getLastCallFailCauseResponse(
            const RadioResponseInfo& info, const LastCallFailCauseInfo& failCauseInfo) override;

    virtual ndk::ScopedAStatus getMuteResponse(const RadioResponseInfo& info, bool enable) override;

    virtual ndk::ScopedAStatus getPreferredVoicePrivacyResponse(const RadioResponseInfo& info,
                                                                bool enable) override;

    virtual ndk::ScopedAStatus getTtyModeResponse(const RadioResponseInfo& info,
                                                  TtyMode mode) override;

    virtual ndk::ScopedAStatus handleStkCallSetupRequestFromSimResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus hangupConnectionResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus hangupForegroundResumeBackgroundResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus hangupWaitingOrBackgroundResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus isVoNrEnabledResponse(const RadioResponseInfo& info,
                                                     bool enable) override;

    virtual ndk::ScopedAStatus rejectCallResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus sendBurstDtmfResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus sendCdmaFeatureCodeResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus sendDtmfResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus sendUssdResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus separateConnectionResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setCallForwardResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setCallWaitingResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setClirResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setMuteResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setPreferredVoicePrivacyResponse(
            const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setTtyModeResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus setVoNrEnabledResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus startDtmfResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus stopDtmfResponse(const RadioResponseInfo& info) override;

    virtual ndk::ScopedAStatus switchWaitingOrHoldingAndActiveResponse(
            const RadioResponseInfo& info) override;
};

/* Callback class for radio voice indication */
class RadioVoiceIndication : public BnRadioVoiceIndication {
  protected:
    RadioServiceTest& parent_voice;

  public:
    RadioVoiceIndication(RadioServiceTest& parent_voice);
    virtual ~RadioVoiceIndication() = default;

    virtual ndk::ScopedAStatus callRing(RadioIndicationType type, bool isGsm,
                                        const CdmaSignalInfoRecord& record) override;

    virtual ndk::ScopedAStatus callStateChanged(RadioIndicationType type) override;

    virtual ndk::ScopedAStatus cdmaCallWaiting(RadioIndicationType type,
                                               const CdmaCallWaiting& callWaitingRecord) override;

    virtual ndk::ScopedAStatus cdmaInfoRec(
            RadioIndicationType type, const std::vector<CdmaInformationRecord>& records) override;

    virtual ndk::ScopedAStatus cdmaOtaProvisionStatus(RadioIndicationType type,
                                                      CdmaOtaProvisionStatus status) override;

    virtual ndk::ScopedAStatus currentEmergencyNumberList(
            RadioIndicationType type,
            const std::vector<EmergencyNumber>& emergencyNumberList) override;

    virtual ndk::ScopedAStatus enterEmergencyCallbackMode(RadioIndicationType type) override;

    virtual ndk::ScopedAStatus exitEmergencyCallbackMode(RadioIndicationType type) override;

    virtual ndk::ScopedAStatus indicateRingbackTone(RadioIndicationType type, bool start) override;

    virtual ndk::ScopedAStatus onSupplementaryServiceIndication(
            RadioIndicationType type, const StkCcUnsolSsResult& ss) override;

    virtual ndk::ScopedAStatus onUssd(RadioIndicationType type, UssdModeType modeType,
                                      const std::string& msg) override;

    virtual ndk::ScopedAStatus resendIncallMute(RadioIndicationType type) override;

    virtual ndk::ScopedAStatus srvccStateNotify(RadioIndicationType type,
                                                SrvccState state) override;

    virtual ndk::ScopedAStatus stkCallControlAlphaNotify(RadioIndicationType type,
                                                         const std::string& alpha) override;

    virtual ndk::ScopedAStatus stkCallSetup(RadioIndicationType type, int64_t timeout) override;
};

// The main test class for Radio AIDL Voice.
class RadioVoiceTest : public RadioServiceTest {
  protected:
    /* Clear Potential Established Calls */
    virtual ndk::ScopedAStatus clearPotentialEstablishedCalls();
    std::shared_ptr<network::IRadioNetwork> radio_network;
    std::shared_ptr<RadioNetworkResponse> radioRsp_network;
    std::shared_ptr<RadioNetworkIndication> radioInd_network;

  public:
    void SetUp() override;

    /* radio voice service handle */
    std::shared_ptr<IRadioVoice> radio_voice;
    /* radio voice response handle */
    std::shared_ptr<RadioVoiceResponse> radioRsp_voice;
    /* radio voice indication handle */
    std::shared_ptr<RadioVoiceIndication> radioInd_voice;
};
