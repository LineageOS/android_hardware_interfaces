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

#include <android/hardware/wifi/1.0/IWifiChip.h>
#include <android/hardware/wifi/1.0/types.h>
#include <android/hardware/wifi/1.2/types.h>
#include <android/hardware/wifi/1.3/types.h>
#include <android/hardware/wifi/1.4/IWifiChipEventCallback.h>
#include <android/hardware/wifi/1.4/types.h>
#include <android/hardware/wifi/1.6/IWifiChip.h>
#include <android/hardware/wifi/1.6/types.h>

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
namespace V1_6 {
namespace implementation {
namespace hidl_struct_util {
using namespace android::hardware::wifi::V1_0;

// Chip conversion methods.
bool convertLegacyFeaturesToHidlChipCapabilities(uint64_t legacy_feature_set,
                                                 uint32_t legacy_logger_feature_set,
                                                 uint32_t* hidl_caps);
bool convertLegacyDebugRingBufferStatusToHidl(
        const legacy_hal::wifi_ring_buffer_status& legacy_status,
        WifiDebugRingBufferStatus* hidl_status);
bool convertLegacyVectorOfDebugRingBufferStatusToHidl(
        const std::vector<legacy_hal::wifi_ring_buffer_status>& legacy_status_vec,
        std::vector<WifiDebugRingBufferStatus>* hidl_status_vec);
bool convertLegacyWakeReasonStatsToHidl(const legacy_hal::WakeReasonStats& legacy_stats,
                                        WifiDebugHostWakeReasonStats* hidl_stats);
legacy_hal::wifi_power_scenario convertHidlTxPowerScenarioToLegacy(
        V1_1::IWifiChip::TxPowerScenario hidl_scenario);
legacy_hal::wifi_latency_mode convertHidlLatencyModeToLegacy(
        V1_3::IWifiChip::LatencyMode hidl_latency_mode);
legacy_hal::wifi_power_scenario convertHidlTxPowerScenarioToLegacy_1_2(
        V1_2::IWifiChip::TxPowerScenario hidl_scenario);
bool convertLegacyWifiMacInfosToHidl(
        const std::vector<legacy_hal::WifiMacInfo>& legacy_mac_infos,
        std::vector<V1_4::IWifiChipEventCallback::RadioModeInfo>* hidl_radio_mode_infos);
legacy_hal::wifi_interface_type convertHidlIfaceTypeToLegacy(IfaceType hidl_interface_type);
legacy_hal::wifi_multi_sta_use_case convertHidlMultiStaUseCaseToLegacy(
        V1_5::IWifiChip::MultiStaUseCase use_case);
bool convertHidlCoexUnsafeChannelToLegacy(
        const V1_5::IWifiChip::CoexUnsafeChannel& hidl_unsafe_channel,
        legacy_hal::wifi_coex_unsafe_channel* legacy_unsafe_channel);
bool convertHidlVectorOfCoexUnsafeChannelToLegacy(
        const std::vector<V1_5::IWifiChip::CoexUnsafeChannel>& hidl_unsafe_channels,
        std::vector<legacy_hal::wifi_coex_unsafe_channel>* legacy_unsafe_channels);
bool convertLegacyRadioCombinationsMatrixToHidl(
        legacy_hal::wifi_radio_combination_matrix* legacy_matrix,
        V1_6::WifiRadioCombinationMatrix* hidl_matrix);
V1_5::WifiBand convertLegacyMacBandToHidlWifiBand(uint32_t band);
V1_6::WifiAntennaMode convertLegacyAntennaConfigurationToHidl(uint32_t antenna_cfg);

// STA iface conversion methods.
bool convertLegacyFeaturesToHidlStaCapabilities(uint64_t legacy_feature_set,
                                                uint32_t legacy_logger_feature_set,
                                                uint32_t* hidl_caps);
bool convertLegacyApfCapabilitiesToHidl(const legacy_hal::PacketFilterCapabilities& legacy_caps,
                                        StaApfPacketFilterCapabilities* hidl_caps);
bool convertLegacyGscanCapabilitiesToHidl(const legacy_hal::wifi_gscan_capabilities& legacy_caps,
                                          StaBackgroundScanCapabilities* hidl_caps);
legacy_hal::wifi_band convertHidlWifiBandToLegacy(V1_0::WifiBand band);
bool convertHidlGscanParamsToLegacy(const StaBackgroundScanParameters& hidl_scan_params,
                                    legacy_hal::wifi_scan_cmd_params* legacy_scan_params);
// |has_ie_data| indicates whether or not the wifi_scan_result includes 802.11
// Information Elements (IEs)
bool convertLegacyGscanResultToHidl(const legacy_hal::wifi_scan_result& legacy_scan_result,
                                    bool has_ie_data, StaScanResult* hidl_scan_result);
// |cached_results| is assumed to not include IEs.
bool convertLegacyVectorOfCachedGscanResultsToHidl(
        const std::vector<legacy_hal::wifi_cached_scan_results>& legacy_cached_scan_results,
        std::vector<StaScanData>* hidl_scan_datas);
bool convertLegacyLinkLayerStatsToHidl(const legacy_hal::LinkLayerStats& legacy_stats,
                                       V1_6::StaLinkLayerStats* hidl_stats);
bool convertLegacyRoamingCapabilitiesToHidl(
        const legacy_hal::wifi_roaming_capabilities& legacy_caps,
        StaRoamingCapabilities* hidl_caps);
bool convertHidlRoamingConfigToLegacy(const StaRoamingConfig& hidl_config,
                                      legacy_hal::wifi_roaming_config* legacy_config);
legacy_hal::fw_roaming_state_t convertHidlRoamingStateToLegacy(StaRoamingState state);
bool convertLegacyVectorOfDebugTxPacketFateToHidl(
        const std::vector<legacy_hal::wifi_tx_report>& legacy_fates,
        std::vector<WifiDebugTxPacketFateReport>* hidl_fates);
bool convertLegacyVectorOfDebugRxPacketFateToHidl(
        const std::vector<legacy_hal::wifi_rx_report>& legacy_fates,
        std::vector<WifiDebugRxPacketFateReport>* hidl_fates);

// NAN iface conversion methods.
void convertToWifiNanStatus(legacy_hal::NanStatusType type, const char* str, size_t max_len,
                            WifiNanStatus* wifiNanStatus);
bool convertHidlNanEnableRequestToLegacy(const V1_4::NanEnableRequest& hidl_request,
                                         legacy_hal::NanEnableRequest* legacy_request);
bool convertHidlNanConfigRequestToLegacy(const V1_4::NanConfigRequest& hidl_request,
                                         legacy_hal::NanConfigRequest* legacy_request);
bool convertHidlNanEnableRequest_1_6ToLegacy(
        const V1_4::NanEnableRequest& hidl_request1,
        const V1_6::NanConfigRequestSupplemental& hidl_request2,
        legacy_hal::NanEnableRequest* legacy_request);
bool convertHidlNanConfigRequest_1_6ToLegacy(
        const V1_4::NanConfigRequest& hidl_request1,
        const V1_6::NanConfigRequestSupplemental& hidl_request2,
        legacy_hal::NanConfigRequest* legacy_request);
bool convertHidlNanPublishRequestToLegacy(const V1_6::NanPublishRequest& hidl_request,
                                          legacy_hal::NanPublishRequest* legacy_request);
bool convertHidlNanSubscribeRequestToLegacy(const V1_0::NanSubscribeRequest& hidl_request,
                                            legacy_hal::NanSubscribeRequest* legacy_request);
bool convertHidlNanTransmitFollowupRequestToLegacy(
        const NanTransmitFollowupRequest& hidl_request,
        legacy_hal::NanTransmitFollowupRequest* legacy_request);
bool convertHidlNanDataPathInitiatorRequestToLegacy(
        const V1_0::NanInitiateDataPathRequest& hidl_request,
        legacy_hal::NanDataPathInitiatorRequest* legacy_request);
bool convertHidlNanDataPathIndicationResponseToLegacy(
        const V1_0::NanRespondToDataPathIndicationRequest& hidl_response,
        legacy_hal::NanDataPathIndicationResponse* legacy_response);
bool convertHidlNanDataPathInitiatorRequest_1_6ToLegacy(
        const V1_6::NanInitiateDataPathRequest& hidl_request,
        legacy_hal::NanDataPathInitiatorRequest* legacy_request);
bool convertHidlNanDataPathIndicationResponse_1_6ToLegacy(
        const V1_6::NanRespondToDataPathIndicationRequest& hidl_response,
        legacy_hal::NanDataPathIndicationResponse* legacy_response);

bool convertLegacyNanResponseHeaderToHidl(const legacy_hal::NanResponseMsg& legacy_response,
                                          WifiNanStatus* wifiNanStatus);
bool convertLegacyNanCapabilitiesResponseToHidl(const legacy_hal::NanCapabilities& legacy_response,
                                                V1_6::NanCapabilities* hidl_response);
bool convertLegacyNanMatchIndToHidl(const legacy_hal::NanMatchInd& legacy_ind,
                                    V1_6::NanMatchInd* hidl_ind);
bool convertLegacyNanFollowupIndToHidl(const legacy_hal::NanFollowupInd& legacy_ind,
                                       NanFollowupReceivedInd* hidl_ind);
bool convertLegacyNanDataPathRequestIndToHidl(const legacy_hal::NanDataPathRequestInd& legacy_ind,
                                              NanDataPathRequestInd* hidl_ind);
bool convertLegacyNanDataPathConfirmIndToHidl(const legacy_hal::NanDataPathConfirmInd& legacy_ind,
                                              V1_6::NanDataPathConfirmInd* hidl_ind);
bool convertLegacyNanDataPathScheduleUpdateIndToHidl(
        const legacy_hal::NanDataPathScheduleUpdateInd& legacy_ind,
        V1_6::NanDataPathScheduleUpdateInd* hidl_ind);

// RTT controller conversion methods.
bool convertHidlVectorOfRttConfigToLegacy(const std::vector<V1_6::RttConfig>& hidl_configs,
                                          std::vector<legacy_hal::wifi_rtt_config>* legacy_configs);
bool convertHidlRttLciInformationToLegacy(const RttLciInformation& hidl_info,
                                          legacy_hal::wifi_lci_information* legacy_info);
bool convertHidlRttLcrInformationToLegacy(const RttLcrInformation& hidl_info,
                                          legacy_hal::wifi_lcr_information* legacy_info);
bool convertHidlRttResponderToLegacy(const V1_6::RttResponder& hidl_responder,
                                     legacy_hal::wifi_rtt_responder* legacy_responder);
bool convertHidlWifiChannelInfoToLegacy(const V1_6::WifiChannelInfo& hidl_info,
                                        legacy_hal::wifi_channel_info* legacy_info);
bool convertLegacyRttResponderToHidl(const legacy_hal::wifi_rtt_responder& legacy_responder,
                                     V1_6::RttResponder* hidl_responder);
bool convertLegacyRttCapabilitiesToHidl(
        const legacy_hal::wifi_rtt_capabilities& legacy_capabilities,
        V1_6::RttCapabilities* hidl_capabilities);
bool convertLegacyVectorOfRttResultToHidl(
        const std::vector<const legacy_hal::wifi_rtt_result*>& legacy_results,
        std::vector<V1_6::RttResult>* hidl_results);
uint32_t convertHidlWifiBandToLegacyMacBand(V1_5::WifiBand band);
uint32_t convertHidlWifiIfaceModeToLegacy(uint32_t hidl_iface_mask);
uint32_t convertHidlUsableChannelFilterToLegacy(uint32_t hidl_filter_mask);
bool convertLegacyWifiUsableChannelsToHidl(
        const std::vector<legacy_hal::wifi_usable_channel>& legacy_usable_channels,
        std::vector<V1_6::WifiUsableChannel>* hidl_usable_channels);
bool convertLegacyPeerInfoStatsToHidl(const legacy_hal::WifiPeerInfo& legacy_peer_info_stats,
                                      V1_6::StaPeerInfo* hidl_peer_info_stats);
bool convertLegacyWifiRateInfoToHidl(const legacy_hal::wifi_rate& legacy_rate,
                                     V1_6::WifiRateInfo* hidl_rate);
}  // namespace hidl_struct_util
}  // namespace implementation
}  // namespace V1_6
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // HIDL_STRUCT_UTIL_H_
