/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef HIDL_STRUCT_UTIL_H_
#define HIDL_STRUCT_UTIL_H_

#include <vector>

#include <android/hardware/wifi/1.0/IWifi.h>

#include "wifi_legacy_hal.h"

/**
 * This file contains a bunch of functions to convert structs from the legacy
 * HAL to HIDL and vice versa.
 * TODO(b/32093047): Add unit tests for these conversion methods in the VTS test
 * suite.
 */
namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
namespace hidl_struct_util {

// STA iface conversion methods.
bool convertHidlScanParamsToLegacy(
    const StaBackgroundScanParameters& hidl_scan_params,
    legacy_hal::wifi_scan_cmd_params* legacy_scan_params);
// Convert the blob of packed IE elements to vector of
// |WifiInformationElement| structures.
bool convertLegacyIeBlobToHidl(const uint8_t* ie_blob,
                               uint32_t ie_blob_len,
                               std::vector<WifiInformationElement>* hidl_ies);
// |has_ie_data| indicates whether or not the wifi_scan_result includes 802.11
// Information Elements (IEs)
bool convertLegacyScanResultToHidl(
    const legacy_hal::wifi_scan_result& legacy_scan_result,
    bool has_ie_data,
    StaScanResult* hidl_scan_result);
// |cached_results| is assumed to not include IEs.
bool convertLegacyVectorOfCachedScanResultsToHidl(
    const std::vector<legacy_hal::wifi_cached_scan_results>&
        legacy_cached_scan_results,
    std::vector<StaScanData>* hidl_scan_datas);
bool convertLegacyLinkLayerStatsToHidl(
    const legacy_hal::LinkLayerStats& legacy_stats,
    StaLinkLayerStats* hidl_stats);

// NAN iface conversion methods.
bool convertHidlNanEnableRequestToLegacy(
    const NanEnableRequest& hidl_request,
    legacy_hal::NanEnableRequest* legacy_request);
bool convertHidlNanPublishRequestToLegacy(
    const NanPublishRequest& hidl_request,
    legacy_hal::NanPublishRequest* legacy_request);
bool convertHidlNanPublishCancelRequestToLegacy(
    const NanPublishCancelRequest& hidl_request,
    legacy_hal::NanPublishCancelRequest* legacy_request);
bool convertHidlNanSubscribeRequestToLegacy(
    const NanSubscribeRequest& hidl_request,
    legacy_hal::NanSubscribeRequest* legacy_request);
bool convertHidlNanSubscribeCancelRequestToLegacy(
    const NanSubscribeCancelRequest& hidl_request,
    legacy_hal::NanSubscribeCancelRequest* legacy_request);
bool convertHidlNanTransmitFollowupRequestToLegacy(
    const NanTransmitFollowupRequest& hidl_request,
    legacy_hal::NanTransmitFollowupRequest* legacy_request);
bool convertHidlNanConfigRequestToLegacy(
    const NanConfigRequest& hidl_request,
    legacy_hal::NanConfigRequest* legacy_request);
bool convertHidlNanBeaconSdfPayloadRequestToLegacy(
    const NanBeaconSdfPayloadRequest& hidl_request,
    legacy_hal::NanBeaconSdfPayloadRequest* legacy_request);
bool convertHidlNanDataPathInitiatorRequestToLegacy(
    const NanDataPathInitiatorRequest& hidl_request,
    legacy_hal::NanDataPathInitiatorRequest* legacy_request);
bool convertHidlNanDataPathIndicationResponseToLegacy(
    const NanDataPathIndicationResponse& hidl_response,
    legacy_hal::NanDataPathIndicationResponse* legacy_response);
bool convertHidlNanDataPathEndRequestToLegacy(
    const NanDataPathEndRequest& hidl_request,
    legacy_hal::NanDataPathEndRequest* legacy_request);
bool convertLegacyNanResponseHeaderToHidl(
    const legacy_hal::NanResponseMsg& legacy_response,
    NanResponseMsgHeader* hidl_response);
bool convertLegacyNanPublishResponseToHidl(
    const legacy_hal::NanPublishResponse& legacy_response,
    NanPublishResponse* hidl_response);
bool convertLegacyNanSubscribeResponseToHidl(
    const legacy_hal::NanSubscribeResponse& legacy_response,
    NanSubscribeResponse* hidl_response);
bool convertLegacyNanDataPathResponseToHidl(
    const legacy_hal::NanDataPathRequestResponse& legacy_response,
    NanDataPathResponse* hidl_response);
bool convertLegacyNanCapabilitiesResponseToHidl(
    const legacy_hal::NanCapabilities& legacy_response,
    NanCapabilitiesResponse* hidl_response);
bool convertLegacyNanPublishTerminatedIndToHidl(
    const legacy_hal::NanPublishTerminatedInd& legacy_ind,
    NanPublishTerminatedInd* hidl_ind);
bool convertLegacyNanMatchIndToHidl(const legacy_hal::NanMatchInd& legacy_ind,
                                    NanMatchInd* hidl_ind);
bool convertLegacyNanMatchExpiredIndToHidl(
    const legacy_hal::NanMatchExpiredInd& legacy_ind,
    NanMatchExpiredInd* hidl_ind);
bool convertLegacyNanSubscribeTerminatedIndToHidl(
    const legacy_hal::NanSubscribeTerminatedInd& legacy_ind,
    NanSubscribeTerminatedInd* hidl_ind);
bool convertLegacyNanFollowupIndToHidl(
    const legacy_hal::NanFollowupInd& legacy_ind, NanFollowupInd* hidl_ind);
bool convertLegacyNanDiscEngEventIndToHidl(
    const legacy_hal::NanDiscEngEventInd& legacy_ind,
    NanDiscEngEventInd* hidl_ind);
bool convertLegacyNanDisabledIndToHidl(
    const legacy_hal::NanDisabledInd& legacy_ind, NanDisabledInd* hidl_ind);
bool convertLegacyNanBeaconSdfPayloadIndToHidl(
    const legacy_hal::NanBeaconSdfPayloadInd& legacy_ind,
    NanBeaconSdfPayloadInd* hidl_ind);
bool convertLegacyNanDataPathRequestIndToHidl(
    const legacy_hal::NanDataPathRequestInd& legacy_ind,
    NanDataPathRequestInd* hidl_ind);
bool convertLegacyNanDataPathConfirmIndToHidl(
    const legacy_hal::NanDataPathConfirmInd& legacy_ind,
    NanDataPathConfirmInd* hidl_ind);
bool convertLegacyNanDataPathEndIndToHidl(
    const legacy_hal::NanDataPathEndInd& legacy_ind,
    NanDataPathEndInd* hidl_ind);
bool convertLegacyNanTransmitFollowupIndToHidl(
    const legacy_hal::NanTransmitFollowupInd& legacy_ind,
    NanTransmitFollowupInd* hidl_ind);
}  // namespace hidl_struct_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // HIDL_STRUCT_UTIL_H_
