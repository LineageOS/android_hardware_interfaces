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

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "VoiceResponse"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::voice;

void RadioResponse::setResponseFunction(std::shared_ptr<aidl::IRadioVoiceResponse> voiceCb) {
    mVoiceCb = voiceCb;
}

std::shared_ptr<aidl::IRadioVoiceResponse> RadioResponse::voiceCb() {
    return mVoiceCb.get();
}

Return<void> RadioResponse::acceptCallResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->acceptCallResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::cancelPendingUssdResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->cancelPendingUssdResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::conferenceResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->conferenceResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::dialResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->dialResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::emergencyDialResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->emergencyDialResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::exitEmergencyCallbackModeResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->exitEmergencyCallbackModeResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::explicitCallTransferResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->explicitCallTransferResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::getCallForwardStatusResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::CallForwardInfo>& callFwdInfos) {
    LOG_CALL << info.serial;
    voiceCb()->getCallForwardStatusResponse(toAidl(info), toAidl(callFwdInfos));
    return {};
}

Return<void> RadioResponse::getCallWaitingResponse(const V1_0::RadioResponseInfo& info, bool enable,
                                                   int32_t serviceClass) {
    LOG_CALL << info.serial;
    voiceCb()->getCallWaitingResponse(toAidl(info), enable, serviceClass);
    return {};
}

Return<void> RadioResponse::getClipResponse(const V1_0::RadioResponseInfo& info,
                                            V1_0::ClipStatus status) {
    LOG_CALL << info.serial;
    voiceCb()->getClipResponse(toAidl(info), aidl::ClipStatus(status));
    return {};
}

Return<void> RadioResponse::getClirResponse(const V1_0::RadioResponseInfo& info, int32_t n,
                                            int32_t m) {
    LOG_CALL << info.serial;
    voiceCb()->getClirResponse(toAidl(info), n, m);
    return {};
}

Return<void> RadioResponse::getCurrentCallsResponse(const V1_0::RadioResponseInfo& info,
                                                    const hidl_vec<V1_0::Call>& calls) {
    LOG_CALL << info.serial;
    voiceCb()->getCurrentCallsResponse(toAidl(info), toAidl(calls));
    return {};
}

Return<void> RadioResponse::getCurrentCallsResponse_1_2(const V1_0::RadioResponseInfo& info,
                                                        const hidl_vec<V1_2::Call>& calls) {
    LOG_CALL << info.serial;
    voiceCb()->getCurrentCallsResponse(toAidl(info), toAidl(calls));
    return {};
}

Return<void> RadioResponse::getCurrentCallsResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                        const hidl_vec<V1_6::Call>& calls) {
    LOG_CALL << info.serial;
    voiceCb()->getCurrentCallsResponse(toAidl(info), toAidl(calls));
    return {};
}

Return<void> RadioResponse::getLastCallFailCauseResponse(
        const V1_0::RadioResponseInfo& info, const V1_0::LastCallFailCauseInfo& failCauseinfo) {
    LOG_CALL << info.serial;
    voiceCb()->getLastCallFailCauseResponse(toAidl(info), toAidl(failCauseinfo));
    return {};
}

Return<void> RadioResponse::getMuteResponse(const V1_0::RadioResponseInfo& info, bool enable) {
    LOG_CALL << info.serial;
    voiceCb()->getMuteResponse(toAidl(info), enable);
    return {};
}

Return<void> RadioResponse::getPreferredVoicePrivacyResponse(const V1_0::RadioResponseInfo& info,
                                                             bool enable) {
    LOG_CALL << info.serial;
    voiceCb()->getPreferredVoicePrivacyResponse(toAidl(info), enable);
    return {};
}

Return<void> RadioResponse::getTTYModeResponse(const V1_0::RadioResponseInfo& info,
                                               V1_0::TtyMode mode) {
    LOG_CALL << info.serial;
    voiceCb()->getTtyModeResponse(toAidl(info), aidl::TtyMode(mode));
    return {};
}

Return<void> RadioResponse::handleStkCallSetupRequestFromSimResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->handleStkCallSetupRequestFromSimResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::hangupConnectionResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->hangupConnectionResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::hangupForegroundResumeBackgroundResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->hangupForegroundResumeBackgroundResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::hangupWaitingOrBackgroundResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->hangupWaitingOrBackgroundResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::rejectCallResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->rejectCallResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::sendBurstDtmfResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->sendBurstDtmfResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::sendCDMAFeatureCodeResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->sendCdmaFeatureCodeResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::sendDtmfResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->sendDtmfResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::sendUssdResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->sendUssdResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::separateConnectionResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->separateConnectionResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setCallForwardResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->setCallForwardResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setCallWaitingResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->setCallWaitingResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setClirResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->setClirResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setMuteResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->setMuteResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setPreferredVoicePrivacyResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->setPreferredVoicePrivacyResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setTTYModeResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->setTtyModeResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::startDtmfResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->startDtmfResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::stopDtmfResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->stopDtmfResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::switchWaitingOrHoldingAndActiveResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    voiceCb()->switchWaitingOrHoldingAndActiveResponse(toAidl(info));
    return {};
}

}  // namespace android::hardware::radio::compat
