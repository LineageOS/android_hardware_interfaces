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

#include <libradiocompat/RadioVoice.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "Voice"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::voice;
constexpr auto ok = &ScopedAStatus::ok;

ScopedAStatus RadioVoice::acceptCall(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->acceptCall(serial);
    return ok();
}

ScopedAStatus RadioVoice::conference(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->conference(serial);
    return ok();
}

ScopedAStatus RadioVoice::dial(int32_t serial, const aidl::Dial& dialInfo) {
    LOG_CALL << serial;
    mHal1_5->dial(serial, toHidl(dialInfo));
    return ok();
}

ScopedAStatus RadioVoice::emergencyDial(  //
        int32_t serial, const aidl::Dial& dialInfo, aidl::EmergencyServiceCategory categories,
        const std::vector<std::string>& urns, aidl::EmergencyCallRouting routing,
        bool hasKnownUserIntentEmerg, bool isTesting) {
    LOG_CALL << serial;
    mHal1_5->emergencyDial(serial, toHidl(dialInfo),
                           toHidlBitfield<V1_4::EmergencyServiceCategory>(categories), toHidl(urns),
                           V1_4::EmergencyCallRouting(routing), hasKnownUserIntentEmerg, isTesting);
    return ok();
}

ScopedAStatus RadioVoice::exitEmergencyCallbackMode(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->exitEmergencyCallbackMode(serial);
    return ok();
}

ScopedAStatus RadioVoice::explicitCallTransfer(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->explicitCallTransfer(serial);
    return ok();
}

ScopedAStatus RadioVoice::getCallForwardStatus(int32_t serial,
                                               const aidl::CallForwardInfo& callInfo) {
    LOG_CALL << serial;
    mHal1_5->getCallForwardStatus(serial, toHidl(callInfo));
    return ok();
}

ScopedAStatus RadioVoice::getCallWaiting(int32_t serial, int32_t serviceClass) {
    LOG_CALL << serial;
    mHal1_5->getCallWaiting(serial, serviceClass);
    return ok();
}

ScopedAStatus RadioVoice::getClip(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getClip(serial);
    return ok();
}

ScopedAStatus RadioVoice::getClir(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getClir(serial);
    return ok();
}

ScopedAStatus RadioVoice::getCurrentCalls(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getCurrentCalls(serial);
    return ok();
}

ScopedAStatus RadioVoice::getLastCallFailCause(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getLastCallFailCause(serial);
    return ok();
}

ScopedAStatus RadioVoice::getMute(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getMute(serial);
    return ok();
}

ScopedAStatus RadioVoice::getPreferredVoicePrivacy(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getPreferredVoicePrivacy(serial);
    return ok();
}

ScopedAStatus RadioVoice::getTtyMode(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getTTYMode(serial);
    return ok();
}

ScopedAStatus RadioVoice::handleStkCallSetupRequestFromSim(int32_t serial, bool accept) {
    LOG_CALL << serial;
    mHal1_5->handleStkCallSetupRequestFromSim(serial, accept);
    return ok();
}

ScopedAStatus RadioVoice::hangup(int32_t serial, int32_t gsmIndex) {
    LOG_CALL << serial;
    mHal1_5->hangup(serial, gsmIndex);
    return ok();
}

ScopedAStatus RadioVoice::hangupForegroundResumeBackground(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->hangupForegroundResumeBackground(serial);
    return ok();
}

ScopedAStatus RadioVoice::hangupWaitingOrBackground(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->hangupWaitingOrBackground(serial);
    return ok();
}

ScopedAStatus RadioVoice::isVoNrEnabled(int32_t serial) {
    LOG_CALL << serial;
    // TODO(b/203699028): can't call isVoNrEnabledResponse with 1.6 callback
    return ok();
}

ScopedAStatus RadioVoice::rejectCall(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->rejectCall(serial);
    return ok();
}

ScopedAStatus RadioVoice::responseAcknowledgement() {
    LOG_CALL;
    mHal1_5->responseAcknowledgement();
    return ok();
}

ScopedAStatus RadioVoice::sendBurstDtmf(int32_t serial, const std::string& dtmf, int32_t on,
                                        int32_t off) {
    LOG_CALL << serial;
    mHal1_5->sendBurstDtmf(serial, dtmf, on, off);
    return ok();
}

ScopedAStatus RadioVoice::sendCdmaFeatureCode(int32_t serial, const std::string& featureCode) {
    LOG_CALL << serial;
    mHal1_5->sendCDMAFeatureCode(serial, featureCode);
    return ok();
}

ScopedAStatus RadioVoice::sendDtmf(int32_t serial, const std::string& s) {
    LOG_CALL << serial;
    mHal1_5->sendDtmf(serial, s);
    return ok();
}

ScopedAStatus RadioVoice::separateConnection(int32_t serial, int32_t gsmIndex) {
    LOG_CALL << serial;
    mHal1_5->separateConnection(serial, gsmIndex);
    return ok();
}

ScopedAStatus RadioVoice::setCallForward(int32_t serial, const aidl::CallForwardInfo& callInfo) {
    LOG_CALL << serial;
    mHal1_5->setCallForward(serial, toHidl(callInfo));
    return ok();
}

ScopedAStatus RadioVoice::setCallWaiting(int32_t serial, bool enable, int32_t serviceClass) {
    LOG_CALL << serial;
    mHal1_5->setCallWaiting(serial, enable, serviceClass);
    return ok();
}

ScopedAStatus RadioVoice::setClir(int32_t serial, int32_t status) {
    LOG_CALL << serial;
    mHal1_5->setClir(serial, status);
    return ok();
}

ScopedAStatus RadioVoice::setMute(int32_t serial, bool enable) {
    LOG_CALL << serial;
    mHal1_5->setMute(serial, enable);
    return ok();
}

ScopedAStatus RadioVoice::setPreferredVoicePrivacy(int32_t serial, bool enable) {
    LOG_CALL << serial;
    mHal1_5->setPreferredVoicePrivacy(serial, enable);
    return ok();
}

ScopedAStatus RadioVoice::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioVoiceResponse>& voiceResponse,
        const std::shared_ptr<aidl::IRadioVoiceIndication>& voiceIndication) {
    LOG_CALL << voiceResponse << ' ' << voiceIndication;

    CHECK(voiceResponse);
    CHECK(voiceIndication);

    mRadioResponse->setResponseFunction(voiceResponse);
    mRadioIndication->setResponseFunction(voiceIndication);

    return ok();
}

ScopedAStatus RadioVoice::setTtyMode(int32_t serial, aidl::TtyMode mode) {
    LOG_CALL << serial;
    mHal1_5->setTTYMode(serial, V1_0::TtyMode(mode));
    return ok();
}

ndk::ScopedAStatus RadioVoice::setVoNrEnabled(int32_t serial, [[maybe_unused]] bool enable) {
    LOG_CALL << serial;
    // TODO(b/203699028): should set `persist.radio.is_vonr_enabled_` property instead
    return ok();
}

ScopedAStatus RadioVoice::startDtmf(int32_t serial, const std::string& s) {
    LOG_CALL << serial;
    mHal1_5->startDtmf(serial, s);
    return ok();
}

ScopedAStatus RadioVoice::stopDtmf(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->stopDtmf(serial);
    return ok();
}

ScopedAStatus RadioVoice::switchWaitingOrHoldingAndActive(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->switchWaitingOrHoldingAndActive(serial);
    return ok();
}

}  // namespace android::hardware::radio::compat
