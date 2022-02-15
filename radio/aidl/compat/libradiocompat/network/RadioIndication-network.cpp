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

#define RADIO_MODULE "NetworkIndication"

namespace android::hardware::radio::compat {

using ::aidl::android::hardware::radio::RadioTechnology;
namespace aidl = ::aidl::android::hardware::radio::network;

void RadioIndication::setResponseFunction(std::shared_ptr<aidl::IRadioNetworkIndication> netCb) {
    mNetworkCb = netCb;
}

std::shared_ptr<aidl::IRadioNetworkIndication> RadioIndication::networkCb() {
    return mNetworkCb.get();
}

Return<void> RadioIndication::barringInfoChanged(V1_0::RadioIndicationType type,
                                                 const V1_5::CellIdentity& cellIdentity,
                                                 const hidl_vec<V1_5::BarringInfo>& barringInfos) {
    LOG_CALL << type;
    networkCb()->barringInfoChanged(toAidl(type), toAidl(cellIdentity), toAidl(barringInfos));
    return {};
}

Return<void> RadioIndication::cdmaPrlChanged(V1_0::RadioIndicationType type, int32_t version) {
    LOG_CALL << type;
    networkCb()->cdmaPrlChanged(toAidl(type), version);
    return {};
}

Return<void> RadioIndication::cellInfoList(V1_0::RadioIndicationType type,
                                           const hidl_vec<V1_0::CellInfo>&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioIndication::cellInfoList_1_2(V1_0::RadioIndicationType type,
                                               const hidl_vec<V1_2::CellInfo>&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.2 not supported";
    return {};
}

Return<void> RadioIndication::cellInfoList_1_4(V1_0::RadioIndicationType type,
                                               const hidl_vec<V1_4::CellInfo>&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.4 not supported";
    return {};
}

Return<void> RadioIndication::cellInfoList_1_5(V1_0::RadioIndicationType type,
                                               const hidl_vec<V1_5::CellInfo>& records) {
    LOG_CALL << type;
    networkCb()->cellInfoList(toAidl(type), toAidl(records));
    return {};
}

Return<void> RadioIndication::cellInfoList_1_6(V1_0::RadioIndicationType type,
                                               const hidl_vec<V1_6::CellInfo>& records) {
    LOG_CALL << type;
    networkCb()->cellInfoList(toAidl(type), toAidl(records));
    return {};
}

Return<void> RadioIndication::currentLinkCapacityEstimate(V1_0::RadioIndicationType type,
                                                          const V1_2::LinkCapacityEstimate& lce) {
    LOG_CALL << type;
    networkCb()->currentLinkCapacityEstimate(toAidl(type), toAidl(lce));
    return {};
}

Return<void> RadioIndication::currentLinkCapacityEstimate_1_6(
        V1_0::RadioIndicationType type, const V1_6::LinkCapacityEstimate& lce) {
    LOG_CALL << type;
    networkCb()->currentLinkCapacityEstimate(toAidl(type), toAidl(lce));
    return {};
}

Return<void> RadioIndication::currentPhysicalChannelConfigs(
        V1_0::RadioIndicationType type, const hidl_vec<V1_2::PhysicalChannelConfig>&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioIndication::currentPhysicalChannelConfigs_1_4(
        V1_0::RadioIndicationType type, const hidl_vec<V1_4::PhysicalChannelConfig>& configs) {
    LOG_CALL << type;
    networkCb()->currentPhysicalChannelConfigs(toAidl(type), toAidl(configs));
    return {};
}

Return<void> RadioIndication::currentPhysicalChannelConfigs_1_6(
        V1_0::RadioIndicationType type, const hidl_vec<V1_6::PhysicalChannelConfig>& configs) {
    LOG_CALL << type;
    networkCb()->currentPhysicalChannelConfigs(toAidl(type), toAidl(configs));
    return {};
}

Return<void> RadioIndication::currentSignalStrength(V1_0::RadioIndicationType type,
                                                    const V1_0::SignalStrength&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioIndication::currentSignalStrength_1_2(V1_0::RadioIndicationType type,
                                                        const V1_2::SignalStrength&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.2 not supported";
    return {};
}

Return<void> RadioIndication::currentSignalStrength_1_4(
        V1_0::RadioIndicationType type, const V1_4::SignalStrength& signalStrength) {
    LOG_CALL << type;
    networkCb()->currentSignalStrength(toAidl(type), toAidl(signalStrength));
    return {};
}

Return<void> RadioIndication::currentSignalStrength_1_6(
        V1_0::RadioIndicationType type, const V1_6::SignalStrength& signalStrength) {
    LOG_CALL << type;
    networkCb()->currentSignalStrength(toAidl(type), toAidl(signalStrength));
    return {};
}

Return<void> RadioIndication::imsNetworkStateChanged(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    networkCb()->imsNetworkStateChanged(toAidl(type));
    return {};
}

Return<void> RadioIndication::networkScanResult(V1_0::RadioIndicationType type,
                                                const V1_1::NetworkScanResult&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioIndication::networkScanResult_1_2(V1_0::RadioIndicationType type,
                                                    const V1_2::NetworkScanResult&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.2 not supported";
    return {};
}

Return<void> RadioIndication::networkScanResult_1_4(V1_0::RadioIndicationType type,
                                                    const V1_4::NetworkScanResult&) {
    LOG_CALL << type;
    LOG(ERROR) << "IRadio HAL 1.4 not supported";
    return {};
}

Return<void> RadioIndication::networkScanResult_1_5(V1_0::RadioIndicationType type,
                                                    const V1_5::NetworkScanResult& result) {
    LOG_CALL << type;
    networkCb()->networkScanResult(toAidl(type), toAidl(result));
    return {};
}

Return<void> RadioIndication::networkScanResult_1_6(V1_0::RadioIndicationType type,
                                                    const V1_6::NetworkScanResult& result) {
    LOG_CALL << type;
    networkCb()->networkScanResult(toAidl(type), toAidl(result));
    return {};
}

Return<void> RadioIndication::networkStateChanged(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    networkCb()->networkStateChanged(toAidl(type));
    return {};
}

Return<void> RadioIndication::nitzTimeReceived(V1_0::RadioIndicationType type,
                                               const hidl_string& nitzTime, uint64_t receivedTime) {
    LOG_CALL << type;
    networkCb()->nitzTimeReceived(toAidl(type), nitzTime, receivedTime, 0);
    return {};
}

Return<void> RadioIndication::registrationFailed(  //
        V1_0::RadioIndicationType type, const V1_5::CellIdentity& cellIdentity,
        const hidl_string& chosenPlmn, hidl_bitfield<V1_5::Domain> domain, int32_t causeCode,
        int32_t additionalCauseCode) {
    LOG_CALL << type;
    networkCb()->registrationFailed(toAidl(type), toAidl(cellIdentity), chosenPlmn, domain,
                                    causeCode, additionalCauseCode);
    return {};
}

Return<void> RadioIndication::restrictedStateChanged(V1_0::RadioIndicationType type,
                                                     V1_0::PhoneRestrictedState state) {
    LOG_CALL << type;
    networkCb()->restrictedStateChanged(toAidl(type), aidl::PhoneRestrictedState(state));
    return {};
}

Return<void> RadioIndication::suppSvcNotify(V1_0::RadioIndicationType type,
                                            const V1_0::SuppSvcNotification& suppSvc) {
    LOG_CALL << type;
    networkCb()->suppSvcNotify(toAidl(type), toAidl(suppSvc));
    return {};
}

Return<void> RadioIndication::voiceRadioTechChanged(V1_0::RadioIndicationType type,
                                                    V1_0::RadioTechnology rat) {
    LOG_CALL << type;
    networkCb()->voiceRadioTechChanged(toAidl(type), RadioTechnology(rat));
    return {};
}

Return<void> RadioIndication::lceData(V1_0::RadioIndicationType type, const V1_0::LceDataInfo&) {
    LOG_CALL << type;
    LOG(WARNING) << "lceData indication is deprecated";
    return {};
}

}  // namespace android::hardware::radio::compat
