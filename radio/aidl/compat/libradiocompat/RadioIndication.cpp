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

Return<void> RadioIndication::networkStateChanged(V1_0::RadioIndicationType type) {
    return {};
}

Return<void> RadioIndication::nitzTimeReceived(V1_0::RadioIndicationType type,
                                               const hidl_string& nitzTime, uint64_t receivedTime) {
    return {};
}

Return<void> RadioIndication::currentSignalStrength(V1_0::RadioIndicationType type,
                                                    const V1_0::SignalStrength& signalStrength) {
    return {};
}

Return<void> RadioIndication::dataCallListChanged(
        V1_0::RadioIndicationType type, const hidl_vec<V1_0::SetupDataCallResult>& dcList) {
    return {};
}

Return<void> RadioIndication::suppSvcNotify(V1_0::RadioIndicationType type,
                                            const V1_0::SuppSvcNotification& suppSvc) {
    return {};
}

Return<void> RadioIndication::stkCallSetup(V1_0::RadioIndicationType type, int64_t timeout) {
    return {};
}

Return<void> RadioIndication::callRing(V1_0::RadioIndicationType type, bool isGsm,
                                       const V1_0::CdmaSignalInfoRecord& record) {
    return {};
}

Return<void> RadioIndication::restrictedStateChanged(V1_0::RadioIndicationType type,
                                                     V1_0::PhoneRestrictedState state) {
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

Return<void> RadioIndication::cdmaPrlChanged(V1_0::RadioIndicationType type, int32_t version) {
    return {};
}

Return<void> RadioIndication::exitEmergencyCallbackMode(V1_0::RadioIndicationType type) {
    return {};
}

Return<void> RadioIndication::rilConnected(V1_0::RadioIndicationType type) {
    return {};
}

Return<void> RadioIndication::voiceRadioTechChanged(V1_0::RadioIndicationType type,
                                                    V1_0::RadioTechnology rat) {
    return {};
}

Return<void> RadioIndication::cellInfoList(V1_0::RadioIndicationType type,
                                           const hidl_vec<V1_0::CellInfo>& records) {
    return {};
}

Return<void> RadioIndication::imsNetworkStateChanged(V1_0::RadioIndicationType type) {
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

Return<void> RadioIndication::lceData(V1_0::RadioIndicationType type,
                                      const V1_0::LceDataInfo& lce) {
    return {};
}

Return<void> RadioIndication::pcoData(V1_0::RadioIndicationType type,
                                      const V1_0::PcoDataInfo& pco) {
    return {};
}

Return<void> RadioIndication::modemReset(V1_0::RadioIndicationType type,
                                         const hidl_string& reason) {
    return {};
}

Return<void> RadioIndication::networkScanResult(V1_0::RadioIndicationType type,
                                                const V1_1::NetworkScanResult& result) {
    return {};
}

Return<void> RadioIndication::keepaliveStatus(V1_0::RadioIndicationType type,
                                              const V1_1::KeepaliveStatus& status) {
    return {};
}

Return<void> RadioIndication::networkScanResult_1_2(V1_0::RadioIndicationType type,
                                                    const V1_2::NetworkScanResult& result) {
    return {};
}

Return<void> RadioIndication::cellInfoList_1_2(V1_0::RadioIndicationType type,
                                               const hidl_vec<V1_2::CellInfo>& records) {
    return {};
}

Return<void> RadioIndication::currentLinkCapacityEstimate(V1_0::RadioIndicationType type,
                                                          const V1_2::LinkCapacityEstimate& lce) {
    return {};
}

Return<void> RadioIndication::currentPhysicalChannelConfigs(
        V1_0::RadioIndicationType type, const hidl_vec<V1_2::PhysicalChannelConfig>& configs) {
    return {};
}

Return<void> RadioIndication::currentSignalStrength_1_2(
        V1_0::RadioIndicationType type, const V1_2::SignalStrength& signalStrength) {
    return {};
}

Return<void> RadioIndication::currentEmergencyNumberList(
        V1_0::RadioIndicationType type, const hidl_vec<V1_4::EmergencyNumber>& emergencyNumbers) {
    return {};
}

Return<void> RadioIndication::cellInfoList_1_4(V1_0::RadioIndicationType type,
                                               const hidl_vec<V1_4::CellInfo>& records) {
    return {};
}

Return<void> RadioIndication::networkScanResult_1_4(V1_0::RadioIndicationType type,
                                                    const V1_4::NetworkScanResult& result) {
    return {};
}

Return<void> RadioIndication::currentPhysicalChannelConfigs_1_4(
        V1_0::RadioIndicationType type, const hidl_vec<V1_4::PhysicalChannelConfig>& configs) {
    return {};
}

Return<void> RadioIndication::dataCallListChanged_1_4(
        V1_0::RadioIndicationType type, const hidl_vec<V1_4::SetupDataCallResult>& dcList) {
    return {};
}

Return<void> RadioIndication::currentSignalStrength_1_4(
        V1_0::RadioIndicationType type, const V1_4::SignalStrength& signalStrength) {
    return {};
}

Return<void> RadioIndication::registrationFailed(  //
        V1_0::RadioIndicationType type, const V1_5::CellIdentity& cellIdentity,
        const hidl_string& chosenPlmn, hidl_bitfield<V1_5::Domain> domain, int32_t causeCode,
        int32_t additionalCauseCode) {
    return {};
}

Return<void> RadioIndication::barringInfoChanged(V1_0::RadioIndicationType type,
                                                 const V1_5::CellIdentity& cellIdentity,
                                                 const hidl_vec<V1_5::BarringInfo>& barringInfos) {
    return {};
}

Return<void> RadioIndication::cellInfoList_1_5(V1_0::RadioIndicationType type,
                                               const hidl_vec<V1_5::CellInfo>& records) {
    return {};
}

Return<void> RadioIndication::networkScanResult_1_5(V1_0::RadioIndicationType type,
                                                    const V1_5::NetworkScanResult& result) {
    return {};
}

Return<void> RadioIndication::dataCallListChanged_1_5(
        V1_0::RadioIndicationType type, const hidl_vec<V1_5::SetupDataCallResult>& dcList) {
    return {};
}

Return<void> RadioIndication::dataCallListChanged_1_6(
        V1_0::RadioIndicationType type, const hidl_vec<V1_6::SetupDataCallResult>& dcList) {
    return {};
}

Return<void> RadioIndication::unthrottleApn(V1_0::RadioIndicationType type,
                                            const hidl_string& apn) {
    return {};
}

Return<void> RadioIndication::currentLinkCapacityEstimate_1_6(
        V1_0::RadioIndicationType type, const V1_6::LinkCapacityEstimate& lce) {
    return {};
}

Return<void> RadioIndication::currentSignalStrength_1_6(
        V1_0::RadioIndicationType type, const V1_6::SignalStrength& signalStrength) {
    return {};
}

Return<void> RadioIndication::cellInfoList_1_6(V1_0::RadioIndicationType type,
                                               const hidl_vec<V1_6::CellInfo>& records) {
    return {};
}

Return<void> RadioIndication::networkScanResult_1_6(V1_0::RadioIndicationType type,
                                                    const V1_6::NetworkScanResult& result) {
    return {};
}

Return<void> RadioIndication::currentPhysicalChannelConfigs_1_6(
        V1_0::RadioIndicationType type, const hidl_vec<V1_6::PhysicalChannelConfig>& configs) {
    return {};
}

}  // namespace android::hardware::radio::compat
