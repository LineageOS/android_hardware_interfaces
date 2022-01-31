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

#include <libradiocompat/RadioIndication.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "VoiceIndication"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::voice;

void RadioIndication::setResponseFunction(std::shared_ptr<aidl::IRadioVoiceIndication> voiceCb) {
    mVoiceCb = voiceCb;
}

std::shared_ptr<aidl::IRadioVoiceIndication> RadioIndication::voiceCb() {
    return mVoiceCb.get();
}

Return<void> RadioIndication::callRing(V1_0::RadioIndicationType type, bool isGsm,
                                       const V1_0::CdmaSignalInfoRecord& record) {
    LOG_CALL << type;
    voiceCb()->callRing(toAidl(type), isGsm, toAidl(record));
    return {};
}

Return<void> RadioIndication::callStateChanged(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    voiceCb()->callStateChanged(toAidl(type));
    return {};
}

Return<void> RadioIndication::cdmaCallWaiting(V1_0::RadioIndicationType type,
                                              const V1_0::CdmaCallWaiting& callWaitingRecord) {
    LOG_CALL << type;
    voiceCb()->cdmaCallWaiting(toAidl(type), toAidl(callWaitingRecord));
    return {};
}

Return<void> RadioIndication::cdmaInfoRec(V1_0::RadioIndicationType type,
                                          const V1_0::CdmaInformationRecords& records) {
    LOG_CALL << type;
    voiceCb()->cdmaInfoRec(toAidl(type), toAidl(records.infoRec));
    return {};
}

Return<void> RadioIndication::cdmaOtaProvisionStatus(V1_0::RadioIndicationType type,
                                                     V1_0::CdmaOtaProvisionStatus status) {
    LOG_CALL << type;
    voiceCb()->cdmaOtaProvisionStatus(toAidl(type), aidl::CdmaOtaProvisionStatus(status));
    return {};
}

Return<void> RadioIndication::currentEmergencyNumberList(
        V1_0::RadioIndicationType type, const hidl_vec<V1_4::EmergencyNumber>& emergencyNumbers) {
    LOG_CALL << type;
    voiceCb()->currentEmergencyNumberList(toAidl(type), toAidl(emergencyNumbers));
    return {};
}

Return<void> RadioIndication::enterEmergencyCallbackMode(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    voiceCb()->enterEmergencyCallbackMode(toAidl(type));
    return {};
}

Return<void> RadioIndication::exitEmergencyCallbackMode(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    voiceCb()->exitEmergencyCallbackMode(toAidl(type));
    return {};
}

Return<void> RadioIndication::indicateRingbackTone(V1_0::RadioIndicationType type, bool start) {
    LOG_CALL << type;
    voiceCb()->indicateRingbackTone(toAidl(type), start);
    return {};
}

Return<void> RadioIndication::onSupplementaryServiceIndication(V1_0::RadioIndicationType type,
                                                               const V1_0::StkCcUnsolSsResult& ss) {
    LOG_CALL << type;
    voiceCb()->onSupplementaryServiceIndication(toAidl(type), toAidl(ss));
    return {};
}

Return<void> RadioIndication::onUssd(V1_0::RadioIndicationType type, V1_0::UssdModeType modeType,
                                     const hidl_string& msg) {
    LOG_CALL << type;
    voiceCb()->onUssd(toAidl(type), aidl::UssdModeType(modeType), msg);
    return {};
}

Return<void> RadioIndication::resendIncallMute(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    voiceCb()->resendIncallMute(toAidl(type));
    return {};
}

Return<void> RadioIndication::srvccStateNotify(V1_0::RadioIndicationType type,
                                               V1_0::SrvccState state) {
    LOG_CALL << type;
    voiceCb()->srvccStateNotify(toAidl(type), aidl::SrvccState(state));
    return {};
}

Return<void> RadioIndication::stkCallControlAlphaNotify(V1_0::RadioIndicationType type,
                                                        const hidl_string& alpha) {
    LOG_CALL << type;
    voiceCb()->stkCallControlAlphaNotify(toAidl(type), alpha);
    return {};
}

Return<void> RadioIndication::stkCallSetup(V1_0::RadioIndicationType type, int64_t timeout) {
    LOG_CALL << type;
    voiceCb()->stkCallSetup(toAidl(type), timeout);
    return {};
}

}  // namespace android::hardware::radio::compat
