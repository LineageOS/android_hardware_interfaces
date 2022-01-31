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

#include "radio_voice_utils.h"

RadioVoiceResponse::RadioVoiceResponse(RadioServiceTest& parent) : parent_voice(parent) {}

ndk::ScopedAStatus RadioVoiceResponse::acceptCallResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::acknowledgeRequest(int32_t /*serial*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::cancelPendingUssdResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::conferenceResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::dialResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::emergencyDialResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::exitEmergencyCallbackModeResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::explicitCallTransferResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::getCallForwardStatusResponse(
        const RadioResponseInfo& info, const std::vector<CallForwardInfo>& /*callForwardInfos*/) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::getCallWaitingResponse(const RadioResponseInfo& info,
                                                              bool /*enable*/,
                                                              int32_t /*serviceClass*/) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::getClipResponse(const RadioResponseInfo& info,
                                                       ClipStatus /*status*/) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::getClirResponse(const RadioResponseInfo& info, int32_t /*n*/,
                                                       int32_t /*m*/) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::getCurrentCallsResponse(const RadioResponseInfo& info,
                                                               const std::vector<Call>& calls) {
    rspInfo = info;
    currentCalls = calls;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::getLastCallFailCauseResponse(
        const RadioResponseInfo& info, const LastCallFailCauseInfo& /*failCauseInfo*/) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::getMuteResponse(const RadioResponseInfo& info,
                                                       bool /*enable*/) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::getPreferredVoicePrivacyResponse(
        const RadioResponseInfo& info, bool /*enable*/) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::getTtyModeResponse(const RadioResponseInfo& info,
                                                          TtyMode /*mode*/) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::handleStkCallSetupRequestFromSimResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::hangupConnectionResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::hangupForegroundResumeBackgroundResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::hangupWaitingOrBackgroundResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::isVoNrEnabledResponse(const RadioResponseInfo& info,
                                                             bool /*enabled*/) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::rejectCallResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::sendBurstDtmfResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::sendCdmaFeatureCodeResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::sendDtmfResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::sendUssdResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::separateConnectionResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::setCallForwardResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::setCallWaitingResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::setClirResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::setMuteResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::setPreferredVoicePrivacyResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::setTtyModeResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::setVoNrEnabledResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::startDtmfResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::stopDtmfResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceResponse::switchWaitingOrHoldingAndActiveResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_voice.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}
