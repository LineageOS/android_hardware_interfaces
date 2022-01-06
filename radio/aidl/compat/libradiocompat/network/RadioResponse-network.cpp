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
#include "utils.h"

#include "collections.h"

#define RADIO_MODULE "NetworkResponse"

namespace android::hardware::radio::compat {

using ::aidl::android::hardware::radio::RadioTechnology;
using ::aidl::android::hardware::radio::RadioTechnologyFamily;
namespace aidl = ::aidl::android::hardware::radio::network;

void RadioResponse::setResponseFunction(std::shared_ptr<aidl::IRadioNetworkResponse> netCb) {
    mNetworkCb = netCb;
}

std::shared_ptr<aidl::IRadioNetworkResponse> RadioResponse::networkCb() {
    return mNetworkCb.get();
}

Return<void> RadioResponse::getAllowedNetworkTypesBitmapResponse(
        const V1_6::RadioResponseInfo& info,
        hidl_bitfield<V1_4::RadioAccessFamily> networkTypeBitmap) {
    LOG_CALL << info.serial;
    networkCb()->getAllowedNetworkTypesBitmapResponse(toAidl(info), networkTypeBitmap);
    return {};
}

Return<void> RadioResponse::getPreferredNetworkTypeResponse(const V1_0::RadioResponseInfo& info,
                                                            V1_0::PreferredNetworkType nwType) {
    LOG_CALL << info.serial;
    networkCb()->getAllowedNetworkTypesBitmapResponse(toAidl(info), getRafFromNetworkType(nwType));
    return {};
}

Return<void> RadioResponse::getPreferredNetworkTypeBitmapResponse(
        const V1_0::RadioResponseInfo& info, hidl_bitfield<V1_4::RadioAccessFamily>) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.4 not supported";
    return {};
}

Return<void> RadioResponse::getAvailableBandModesResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::RadioBandMode>& bandModes) {
    LOG_CALL << info.serial;
    networkCb()->getAvailableBandModesResponse(toAidl(info), toAidl(bandModes));
    return {};
}

Return<void> RadioResponse::getAvailableNetworksResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::OperatorInfo>& networkInfos) {
    LOG_CALL << info.serial;
    networkCb()->getAvailableNetworksResponse(toAidl(info), toAidl(networkInfos));
    return {};
}

Return<void> RadioResponse::getBarringInfoResponse(
        const V1_0::RadioResponseInfo& info, const V1_5::CellIdentity& cellIdentity,
        const hidl_vec<V1_5::BarringInfo>& barringInfos) {
    LOG_CALL << info.serial;
    networkCb()->getBarringInfoResponse(toAidl(info), toAidl(cellIdentity), toAidl(barringInfos));
    return {};
}

Return<void> RadioResponse::getCdmaRoamingPreferenceResponse(const V1_0::RadioResponseInfo& info,
                                                             V1_0::CdmaRoamingType type) {
    LOG_CALL << info.serial;
    networkCb()->getCdmaRoamingPreferenceResponse(toAidl(info), aidl::CdmaRoamingType(type));
    return {};
}

Return<void> RadioResponse::getCellInfoListResponse(const V1_0::RadioResponseInfo& info,
                                                    const hidl_vec<V1_0::CellInfo>&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioResponse::getCellInfoListResponse_1_2(const V1_0::RadioResponseInfo& info,
                                                        const hidl_vec<V1_2::CellInfo>&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.2 not supported";
    return {};
}

Return<void> RadioResponse::getCellInfoListResponse_1_4(const V1_0::RadioResponseInfo& info,
                                                        const hidl_vec<V1_4::CellInfo>&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.4 not supported";
    return {};
}

Return<void> RadioResponse::getCellInfoListResponse_1_5(const V1_0::RadioResponseInfo& info,
                                                        const hidl_vec<V1_5::CellInfo>& cellInfo) {
    LOG_CALL << info.serial;
    networkCb()->getCellInfoListResponse(toAidl(info), toAidl(cellInfo));
    return {};
}

Return<void> RadioResponse::getCellInfoListResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                        const hidl_vec<V1_6::CellInfo>& cellInfo) {
    LOG_CALL << info.serial;
    networkCb()->getCellInfoListResponse(toAidl(info), toAidl(cellInfo));
    return {};
}

Return<void> RadioResponse::getDataRegistrationStateResponse(const V1_0::RadioResponseInfo& info,
                                                             const V1_0::DataRegStateResult&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioResponse::getDataRegistrationStateResponse_1_2(
        const V1_0::RadioResponseInfo& info, const V1_2::DataRegStateResult&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.2 not supported";
    return {};
}

Return<void> RadioResponse::getDataRegistrationStateResponse_1_4(
        const V1_0::RadioResponseInfo& info, const V1_4::DataRegStateResult&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.4 not supported";
    return {};
}

Return<void> RadioResponse::getDataRegistrationStateResponse_1_5(
        const V1_0::RadioResponseInfo& info, const V1_5::RegStateResult& dataRegResponse) {
    LOG_CALL << info.serial;
    networkCb()->getDataRegistrationStateResponse(toAidl(info), toAidl(dataRegResponse));
    return {};
}

Return<void> RadioResponse::getDataRegistrationStateResponse_1_6(
        const V1_6::RadioResponseInfo& info, const V1_6::RegStateResult& dataRegResponse) {
    LOG_CALL << info.serial;
    networkCb()->getDataRegistrationStateResponse(toAidl(info), toAidl(dataRegResponse));
    return {};
}

Return<void> RadioResponse::getImsRegistrationStateResponse(  //
        const V1_0::RadioResponseInfo& info, bool isRegd, V1_0::RadioTechnologyFamily ratFamily) {
    LOG_CALL << info.serial;
    networkCb()->getImsRegistrationStateResponse(toAidl(info), isRegd,
                                                 RadioTechnologyFamily(ratFamily));
    return {};
}

Return<void> RadioResponse::getNeighboringCidsResponse(const V1_0::RadioResponseInfo& info,
                                                       const hidl_vec<V1_0::NeighboringCell>&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "getNeighboringCidsResponse is not supposed to be called";
    return {};
}

Return<void> RadioResponse::getNetworkSelectionModeResponse(const V1_0::RadioResponseInfo& info,
                                                            bool manual) {
    LOG_CALL << info.serial;
    networkCb()->getNetworkSelectionModeResponse(toAidl(info), manual);
    return {};
}

Return<void> RadioResponse::getOperatorResponse(  //
        const V1_0::RadioResponseInfo& info, const hidl_string& longName,
        const hidl_string& shortName, const hidl_string& numeric) {
    LOG_CALL << info.serial;
    networkCb()->getOperatorResponse(toAidl(info), longName, shortName, numeric);
    return {};
}

Return<void> RadioResponse::getSignalStrengthResponse(const V1_0::RadioResponseInfo& info,
                                                      const V1_0::SignalStrength&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioResponse::getSignalStrengthResponse_1_2(const V1_0::RadioResponseInfo& info,
                                                          const V1_2::SignalStrength&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.2 not supported";
    return {};
}

Return<void> RadioResponse::getSignalStrengthResponse_1_4(
        const V1_0::RadioResponseInfo& info, const V1_4::SignalStrength& signalStrength) {
    LOG_CALL << info.serial;
    networkCb()->getSignalStrengthResponse(toAidl(info), toAidl(signalStrength));
    return {};
}

Return<void> RadioResponse::getSignalStrengthResponse_1_6(
        const V1_6::RadioResponseInfo& info, const V1_6::SignalStrength& signalStrength) {
    LOG_CALL << info.serial;
    networkCb()->getSignalStrengthResponse(toAidl(info), toAidl(signalStrength));
    return {};
}

Return<void> RadioResponse::getSystemSelectionChannelsResponse(
        const V1_6::RadioResponseInfo& info,
        const hidl_vec<V1_5::RadioAccessSpecifier>& specifiers) {
    LOG_CALL << info.serial;
    networkCb()->getSystemSelectionChannelsResponse(toAidl(info), toAidl(specifiers));
    return {};
}

Return<void> RadioResponse::getVoiceRadioTechnologyResponse(const V1_0::RadioResponseInfo& info,
                                                            V1_0::RadioTechnology rat) {
    LOG_CALL << info.serial;
    networkCb()->getVoiceRadioTechnologyResponse(toAidl(info), RadioTechnology(rat));
    return {};
}

Return<void> RadioResponse::getVoiceRegistrationStateResponse(const V1_0::RadioResponseInfo& info,
                                                              const V1_0::VoiceRegStateResult&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.0 not supported";
    return {};
}

Return<void> RadioResponse::getVoiceRegistrationStateResponse_1_2(
        const V1_0::RadioResponseInfo& info, const V1_2::VoiceRegStateResult&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.2 not supported";
    return {};
}

Return<void> RadioResponse::getVoiceRegistrationStateResponse_1_5(
        const V1_0::RadioResponseInfo& info, const V1_5::RegStateResult& voiceRegResponse) {
    LOG_CALL << info.serial;
    networkCb()->getVoiceRegistrationStateResponse(toAidl(info), toAidl(voiceRegResponse));
    return {};
}

Return<void> RadioResponse::getVoiceRegistrationStateResponse_1_6(
        const V1_6::RadioResponseInfo& info, const V1_6::RegStateResult& voiceRegResponse) {
    LOG_CALL << info.serial;
    networkCb()->getVoiceRegistrationStateResponse(toAidl(info), toAidl(voiceRegResponse));
    return {};
}

Return<void> RadioResponse::isNrDualConnectivityEnabledResponse(const V1_6::RadioResponseInfo& info,
                                                                bool isEnabled) {
    LOG_CALL << info.serial;
    networkCb()->isNrDualConnectivityEnabledResponse(toAidl(info), isEnabled);
    return {};
}

Return<void> RadioResponse::pullLceDataResponse(const V1_0::RadioResponseInfo& info,
                                                const V1_0::LceDataInfo&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "pullLceDataResponse is not supposed to be called";
    return {};
}

Return<void> RadioResponse::setAllowedNetworkTypesBitmapResponse(const V1_6::RadioResponseInfo& i) {
    LOG_CALL << i.serial;
    networkCb()->setAllowedNetworkTypesBitmapResponse(toAidl(i));
    return {};
}

Return<void> RadioResponse::setPreferredNetworkTypeResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setAllowedNetworkTypesBitmapResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setPreferredNetworkTypeBitmapResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "IRadio HAL 1.4 not supported";
    return {};
}

Return<void> RadioResponse::setBandModeResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setBandModeResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setBarringPasswordResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setBarringPasswordResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setCdmaRoamingPreferenceResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setCdmaRoamingPreferenceResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setCellInfoListRateResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setCellInfoListRateResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setIndicationFilterResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setIndicationFilterResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setIndicationFilterResponse_1_5(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setIndicationFilterResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setLinkCapacityReportingCriteriaResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setLinkCapacityReportingCriteriaResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setLinkCapacityReportingCriteriaResponse_1_5(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setLinkCapacityReportingCriteriaResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setLocationUpdatesResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setLocationUpdatesResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setNetworkSelectionModeAutomaticResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setNetworkSelectionModeAutomaticResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setNetworkSelectionModeManualResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setNetworkSelectionModeManualResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setNetworkSelectionModeManualResponse_1_5(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setNetworkSelectionModeManualResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setNrDualConnectivityStateResponse(
        const V1_6::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setNrDualConnectivityStateResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setSignalStrengthReportingCriteriaResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setSignalStrengthReportingCriteriaResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setSignalStrengthReportingCriteriaResponse_1_5(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setSignalStrengthReportingCriteriaResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setSuppServiceNotificationsResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setSuppServiceNotificationsResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setSystemSelectionChannelsResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setSystemSelectionChannelsResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setSystemSelectionChannelsResponse_1_5(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->setSystemSelectionChannelsResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::startNetworkScanResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->startNetworkScanResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::startNetworkScanResponse_1_4(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->startNetworkScanResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::startNetworkScanResponse_1_5(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->startNetworkScanResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::stopNetworkScanResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    networkCb()->stopNetworkScanResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::supplyNetworkDepersonalizationResponse(
        const V1_0::RadioResponseInfo& info, int32_t remainingRetries) {
    LOG_CALL << info.serial;
    networkCb()->supplyNetworkDepersonalizationResponse(toAidl(info), remainingRetries);
    return {};
}

Return<void> RadioResponse::startLceServiceResponse(const V1_0::RadioResponseInfo& info,
                                                    const V1_0::LceStatusInfo&) {
    LOG_CALL << info.serial;
    LOG(WARNING) << "startLceServiceResponse is deprecated";
    return {};
}

Return<void> RadioResponse::stopLceServiceResponse(const V1_0::RadioResponseInfo& info,
                                                   const V1_0::LceStatusInfo&) {
    LOG_CALL << info.serial;
    LOG(WARNING) << "stopLceServiceResponse is deprecated";
    return {};
}

}  // namespace android::hardware::radio::compat
