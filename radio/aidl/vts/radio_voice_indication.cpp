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

RadioVoiceIndication::RadioVoiceIndication(RadioServiceTest& parent) : parent_voice(parent) {}

ndk::ScopedAStatus RadioVoiceIndication::callRing(RadioIndicationType /*type*/, bool /*isGsm*/,
                                                  const CdmaSignalInfoRecord& /*record*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::callStateChanged(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::cdmaCallWaiting(
        RadioIndicationType /*type*/, const CdmaCallWaiting& /*callWaitingRecord*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::cdmaInfoRec(
        RadioIndicationType /*type*/, const std::vector<CdmaInformationRecord>& /*records*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::cdmaOtaProvisionStatus(RadioIndicationType /*type*/,
                                                                CdmaOtaProvisionStatus /*status*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::currentEmergencyNumberList(
        RadioIndicationType /*type*/, const std::vector<EmergencyNumber>& /*emergencyNumberList*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::enterEmergencyCallbackMode(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::exitEmergencyCallbackMode(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::indicateRingbackTone(RadioIndicationType /*type*/,
                                                              bool /*start*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::onSupplementaryServiceIndication(
        RadioIndicationType /*type*/, const StkCcUnsolSsResult& /*ss*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::onUssd(RadioIndicationType /*type*/,
                                                UssdModeType /*modeType*/,
                                                const std::string& /*msg*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::resendIncallMute(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::srvccStateNotify(RadioIndicationType /*type*/,
                                                          SrvccState /*state*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::stkCallControlAlphaNotify(RadioIndicationType /*type*/,
                                                                   const std::string& /*alpha*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioVoiceIndication::stkCallSetup(RadioIndicationType /*type*/,
                                                      int64_t /*timeout*/) {
    return ndk::ScopedAStatus::ok();
}
