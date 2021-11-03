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

// TODO(b/203699028): remove when fully implemented
#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace android::hardware::radio::compat {

Return<void> RadioIndication::radioStateChanged(V1_0::RadioIndicationType type,
                                                V1_0::RadioState radioState) {
    return {};
}

Return<void> RadioIndication::callStateChanged(V1_0::RadioIndicationType type) {
    return {};
}

Return<void> RadioIndication::stkCallSetup(V1_0::RadioIndicationType type, int64_t timeout) {
    return {};
}

Return<void> RadioIndication::callRing(V1_0::RadioIndicationType type, bool isGsm,
                                       const V1_0::CdmaSignalInfoRecord& record) {
    return {};
}

Return<void> RadioIndication::enterEmergencyCallbackMode(V1_0::RadioIndicationType type) {
    return {};
}

Return<void> RadioIndication::cdmaCallWaiting(V1_0::RadioIndicationType type,
                                              const V1_0::CdmaCallWaiting& callWaitingRecord) {
    return {};
}

Return<void> RadioIndication::cdmaOtaProvisionStatus(V1_0::RadioIndicationType type,
                                                     V1_0::CdmaOtaProvisionStatus status) {
    return {};
}

Return<void> RadioIndication::cdmaInfoRec(V1_0::RadioIndicationType type,
                                          const V1_0::CdmaInformationRecords& records) {
    return {};
}

Return<void> RadioIndication::indicateRingbackTone(V1_0::RadioIndicationType type, bool start) {
    return {};
}

Return<void> RadioIndication::resendIncallMute(V1_0::RadioIndicationType type) {
    return {};
}

Return<void> RadioIndication::exitEmergencyCallbackMode(V1_0::RadioIndicationType type) {
    return {};
}

Return<void> RadioIndication::rilConnected(V1_0::RadioIndicationType type) {
    return {};
}

Return<void> RadioIndication::srvccStateNotify(V1_0::RadioIndicationType type,
                                               V1_0::SrvccState state) {
    return {};
}

Return<void> RadioIndication::hardwareConfigChanged(V1_0::RadioIndicationType type,
                                                    const hidl_vec<V1_0::HardwareConfig>& configs) {
    return {};
}

Return<void> RadioIndication::radioCapabilityIndication(V1_0::RadioIndicationType type,
                                                        const V1_0::RadioCapability& rc) {
    return {};
}

Return<void> RadioIndication::onSupplementaryServiceIndication(V1_0::RadioIndicationType type,
                                                               const V1_0::StkCcUnsolSsResult& ss) {
    return {};
}

Return<void> RadioIndication::stkCallControlAlphaNotify(V1_0::RadioIndicationType type,
                                                        const hidl_string& alpha) {
    return {};
}

Return<void> RadioIndication::modemReset(V1_0::RadioIndicationType type,
                                         const hidl_string& reason) {
    return {};
}

Return<void> RadioIndication::currentEmergencyNumberList(
        V1_0::RadioIndicationType type, const hidl_vec<V1_4::EmergencyNumber>& emergencyNumbers) {
    return {};
}

}  // namespace android::hardware::radio::compat
