/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef AIDL_STRUCT_UTIL_H_
#define AIDL_STRUCT_UTIL_H_

#include <aidl/android/hardware/wifi/IWifiChip.h>
#include <aidl/android/hardware/wifi/IWifiChipEventCallback.h>
#include <aidl/android/hardware/wifi/NanBandIndex.h>
#include <aidl/android/hardware/wifi/StaBackgroundScanBucketEventReportSchemeMask.h>
#include <aidl/android/hardware/wifi/StaScanDataFlagMask.h>
#include <aidl/android/hardware/wifi/WifiDebugRingBufferFlags.h>
#include <aidl/android/hardware/wifi/WifiIfaceMode.h>

#include <vector>

#include "wifi_legacy_hal.h"

/**
 * This file contains a bunch of functions to convert structs from the legacy
 * HAL to AIDL and vice versa.
 */
namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
namespace aidl_struct_util {

// Chip conversion methods.
bool convertLegacyChipFeaturesToAidl(uint64_t legacy_feature_set, uint32_t* aidl_feature_set);
bool convertLegacyDebugRingBufferStatusToAidl(
        const legacy_hal::wifi_ring_buffer_status& legacy_status,
        WifiDebugRingBufferStatus* aidl_status);
bool convertLegacyVectorOfDebugRingBufferStatusToAidl(
        const std::vector<legacy_hal::wifi_ring_buffer_status>& legacy_status_vec,
        std::vector<WifiDebugRingBufferStatus>* aidl_status_vec);
bool convertLegacyWakeReasonStatsToAidl(const legacy_hal::WakeReasonStats& legacy_stats,
                                        WifiDebugHostWakeReasonStats* aidl_stats);
legacy_hal::wifi_power_scenario convertAidlTxPowerScenarioToLegacy(
        IWifiChip::TxPowerScenario aidl_scenario);
legacy_hal::wifi_latency_mode convertAidlLatencyModeToLegacy(
        IWifiChip::LatencyMode aidl_latency_mode);
bool convertLegacyWifiMacInfosToAidl(
        const std::vector<legacy_hal::WifiMacInfo>& legacy_mac_infos,
        std::vector<IWifiChipEventCallback::RadioModeInfo>* aidl_radio_mode_infos);
legacy_hal::wifi_interface_type convertAidlIfaceTypeToLegacy(IfaceType aidl_interface_type);
legacy_hal::wifi_multi_sta_use_case convertAidlMultiStaUseCaseToLegacy(
        IWifiChip::MultiStaUseCase use_case);
bool convertAidlCoexUnsafeChannelToLegacy(
        const IWifiChip::CoexUnsafeChannel& aidl_unsafe_channel,
        legacy_hal::wifi_coex_unsafe_channel* legacy_unsafe_channel);
bool convertAidlVectorOfCoexUnsafeChannelToLegacy(
        const std::vector<IWifiChip::CoexUnsafeChannel>& aidl_unsafe_channels,
        std::vector<legacy_hal::wifi_coex_unsafe_channel>* legacy_unsafe_channels);
bool convertLegacyRadioCombinationsMatrixToAidl(
        legacy_hal::wifi_radio_combination_matrix* legacy_matrix,
        std::vector<WifiRadioCombination>* aidl_combinations);
WifiBand convertLegacyMacBandToAidlWifiBand(uint32_t band);
WifiAntennaMode convertLegacyAntennaConfigurationToAidl(uint32_t antenna_cfg);
bool convertLegacyIfaceCombinationsMatrixToChipMode(
        legacy_hal::wifi_iface_concurrency_matrix& legacy_matrix, IWifiChip::ChipMode* chip_mode);

// STA iface conversion methods.
bool convertLegacyStaIfaceFeaturesToAidl(uint64_t legacy_feature_set, uint32_t* aidl_feature_set);
bool convertLegacyApfCapabilitiesToAidl(const legacy_hal::PacketFilterCapabilities& legacy_caps,
                                        StaApfPacketFilterCapabilities* aidl_caps);
bool convertLegacyGscanCapabilitiesToAidl(const legacy_hal::wifi_gscan_capabilities& legacy_caps,
                                          StaBackgroundScanCapabilities* aidl_caps);
legacy_hal::wifi_band convertAidlWifiBandToLegacy(WifiBand band);
bool convertAidlGscanParamsToLegacy(const StaBackgroundScanParameters& aidl_scan_params,
                                    legacy_hal::wifi_scan_cmd_params* legacy_scan_params);
// |has_ie_data| indicates whether or not the wifi_scan_result includes 802.11
// Information Elements (IEs)
bool convertLegacyGscanResultToAidl(const legacy_hal::wifi_scan_result& legacy_scan_result,
                                    bool has_ie_data, StaScanResult* aidl_scan_result);
// |cached_results| is assumed to not include IEs.
bool convertLegacyVectorOfCachedGscanResultsToAidl(
        const std::vector<legacy_hal::wifi_cached_scan_results>& legacy_cached_scan_results,
        std::vector<StaScanData>* aidl_scan_datas);
bool convertLegacyLinkLayerMlStatsToAidl(const legacy_hal::LinkLayerMlStats& legacy_ml_stats,
                                         StaLinkLayerStats* aidl_stats);
bool convertLegacyLinkLayerStatsToAidl(const legacy_hal::LinkLayerStats& legacy_stats,
                                       StaLinkLayerStats* aidl_stats);
bool convertLegacyRoamingCapabilitiesToAidl(
        const legacy_hal::wifi_roaming_capabilities& legacy_caps,
        StaRoamingCapabilities* aidl_caps);
bool convertAidlRoamingConfigToLegacy(const StaRoamingConfig& aidl_config,
                                      legacy_hal::wifi_roaming_config* legacy_config);
legacy_hal::fw_roaming_state_t convertAidlRoamingStateToLegacy(StaRoamingState state);
bool convertLegacyVectorOfDebugTxPacketFateToAidl(
        const std::vector<legacy_hal::wifi_tx_report>& legacy_fates,
        std::vector<WifiDebugTxPacketFateReport>* aidl_fates);
bool convertLegacyVectorOfDebugRxPacketFateToAidl(
        const std::vector<legacy_hal::wifi_rx_report>& legacy_fates,
        std::vector<WifiDebugRxPacketFateReport>* aidl_fates);

// NAN iface conversion methods.
void convertToNanStatus(legacy_hal::NanStatusType type, const char* str, size_t max_len,
                        NanStatus* nanStatus);
bool convertAidlNanEnableRequestToLegacy(const NanEnableRequest& aidl_request1,
                                         const NanConfigRequestSupplemental& aidl_request2,
                                         legacy_hal::NanEnableRequest* legacy_request);
bool convertAidlNanConfigRequestToLegacy(const NanConfigRequest& aidl_request1,
                                         const NanConfigRequestSupplemental& aidl_request2,
                                         legacy_hal::NanConfigRequest* legacy_request);
bool convertAidlNanPublishRequestToLegacy(const NanPublishRequest& aidl_request,
                                          legacy_hal::NanPublishRequest* legacy_request);
bool convertAidlNanSubscribeRequestToLegacy(const NanSubscribeRequest& aidl_request,
                                            legacy_hal::NanSubscribeRequest* legacy_request);
bool convertAidlNanTransmitFollowupRequestToLegacy(
        const NanTransmitFollowupRequest& aidl_request,
        legacy_hal::NanTransmitFollowupRequest* legacy_request);
bool convertAidlNanDataPathInitiatorRequestToLegacy(
        const NanInitiateDataPathRequest& aidl_request,
        legacy_hal::NanDataPathInitiatorRequest* legacy_request);
bool convertAidlNanDataPathIndicationResponseToLegacy(
        const NanRespondToDataPathIndicationRequest& aidl_response,
        legacy_hal::NanDataPathIndicationResponse* legacy_response);
bool convertLegacyNanResponseHeaderToAidl(const legacy_hal::NanResponseMsg& legacy_response,
                                          NanStatus* nanStatus);
bool convertLegacyNanCapabilitiesResponseToAidl(const legacy_hal::NanCapabilities& legacy_response,
                                                NanCapabilities* aidl_response);
bool convertLegacyNanMatchIndToAidl(const legacy_hal::NanMatchInd& legacy_ind,
                                    NanMatchInd* aidl_ind);
bool convertLegacyNanFollowupIndToAidl(const legacy_hal::NanFollowupInd& legacy_ind,
                                       NanFollowupReceivedInd* aidl_ind);
bool convertLegacyNanDataPathRequestIndToAidl(const legacy_hal::NanDataPathRequestInd& legacy_ind,
                                              NanDataPathRequestInd* aidl_ind);
bool convertLegacyNanDataPathConfirmIndToAidl(const legacy_hal::NanDataPathConfirmInd& legacy_ind,
                                              NanDataPathConfirmInd* aidl_ind);
bool convertLegacyNanDataPathScheduleUpdateIndToAidl(
        const legacy_hal::NanDataPathScheduleUpdateInd& legacy_ind,
        NanDataPathScheduleUpdateInd* aidl_ind);

// RTT controller conversion methods.
bool convertAidlVectorOfRttConfigToLegacy(const std::vector<RttConfig>& aidl_configs,
                                          std::vector<legacy_hal::wifi_rtt_config>* legacy_configs);
bool convertAidlVectorOfRttConfigToLegacyV3(
        const std::vector<RttConfig>& aidl_configs,
        std::vector<legacy_hal::wifi_rtt_config_v3>* legacy_configs);

bool convertAidlRttLciInformationToLegacy(const RttLciInformation& aidl_info,
                                          legacy_hal::wifi_lci_information* legacy_info);
bool convertAidlRttLcrInformationToLegacy(const RttLcrInformation& aidl_info,
                                          legacy_hal::wifi_lcr_information* legacy_info);
bool convertAidlRttResponderToLegacy(const RttResponder& aidl_responder,
                                     legacy_hal::wifi_rtt_responder* legacy_responder);
bool convertAidlWifiChannelInfoToLegacy(const WifiChannelInfo& aidl_info,
                                        legacy_hal::wifi_channel_info* legacy_info);
bool convertLegacyRttResponderToAidl(const legacy_hal::wifi_rtt_responder& legacy_responder,
                                     RttResponder* aidl_responder);
bool convertLegacyRttCapabilitiesToAidl(
        const legacy_hal::wifi_rtt_capabilities& legacy_capabilities,
        RttCapabilities* aidl_capabilities);
bool convertLegacyRttCapabilitiesV3ToAidl(
        const legacy_hal::wifi_rtt_capabilities_v3& legacy_capabilities_v3,
        RttCapabilities* aidl_capabilities);

bool convertLegacyVectorOfRttResultToAidl(
        const std::vector<const legacy_hal::wifi_rtt_result*>& legacy_results,
        std::vector<RttResult>* aidl_results);
bool convertLegacyVectorOfRttResultV2ToAidl(
        const std::vector<const legacy_hal::wifi_rtt_result_v2*>& legacy_results,
        std::vector<RttResult>* aidl_results);
bool convertLegacyVectorOfRttResultV3ToAidl(
        const std::vector<const legacy_hal::wifi_rtt_result_v3*>& legacy_results,
        std::vector<RttResult>* aidl_results);
uint32_t convertAidlWifiBandToLegacyMacBand(WifiBand band);
uint32_t convertAidlWifiIfaceModeToLegacy(uint32_t aidl_iface_mask);
uint32_t convertAidlUsableChannelFilterToLegacy(uint32_t aidl_filter_mask);
bool convertLegacyWifiUsableChannelsToAidl(
        const std::vector<legacy_hal::wifi_usable_channel>& legacy_usable_channels,
        std::vector<WifiUsableChannel>* aidl_usable_channels);
bool convertLegacyPeerInfoStatsToAidl(const legacy_hal::WifiPeerInfo& legacy_peer_info_stats,
                                      StaPeerInfo* aidl_peer_info_stats);
bool convertLegacyWifiRateInfoToAidl(const legacy_hal::wifi_rate& legacy_rate,
                                     WifiRateInfo* aidl_rate);
bool convertLegacyWifiChipCapabilitiesToAidl(
        const legacy_hal::wifi_chip_capabilities& legacy_chip_capabilities,
        WifiChipCapabilities& aidl_chip_capabilities);
bool convertAidlNanPairingInitiatorRequestToLegacy(const NanPairingRequest& aidl_request,
                                                   legacy_hal::NanPairingRequest* legacy_request);
bool convertAidlNanPairingIndicationResponseToLegacy(
        const NanRespondToPairingIndicationRequest& aidl_response,
        legacy_hal::NanPairingIndicationResponse* legacy_response);
bool convertAidlNanBootstrappingInitiatorRequestToLegacy(
        const NanBootstrappingRequest& aidl_request,
        legacy_hal::NanBootstrappingRequest* legacy_request);
bool convertAidlNanBootstrappingIndicationResponseToLegacy(
        const NanBootstrappingResponse& aidl_response,
        legacy_hal::NanBootstrappingIndicationResponse* legacy_response);
bool convertLegacyNanPairingRequestIndToAidl(const legacy_hal::NanPairingRequestInd& legacy_ind,
                                             NanPairingRequestInd* aidl_ind);
bool convertLegacyNanPairingConfirmIndToAidl(const legacy_hal::NanPairingConfirmInd& legacy_ind,
                                             NanPairingConfirmInd* aidl_ind);
bool convertLegacyNanBootstrappingRequestIndToAidl(
        const legacy_hal::NanBootstrappingRequestInd& legacy_ind,
        NanBootstrappingRequestInd* aidl_ind);
bool convertLegacyNanBootstrappingConfirmIndToAidl(
        const legacy_hal::NanBootstrappingConfirmInd& legacy_ind,
        NanBootstrappingConfirmInd* aidl_ind);
uint32_t convertAidlChannelCategoryToLegacy(uint32_t aidl_channel_category_mask);
bool convertCachedScanReportToAidl(const legacy_hal::WifiCachedScanReport& report,
                                   CachedScanData* aidl_scan_data);
bool convertCachedScanResultToAidl(const legacy_hal::wifi_cached_scan_result& legacy_scan_result,
                                   uint64_t ts_us, CachedScanResult* aidl_scan_result);
WifiRatePreamble convertScanResultFlagsToPreambleType(int flags);
bool convertTwtCapabilitiesToAidl(const legacy_hal::wifi_twt_capabilities legacy_twt_capabs,
                                  TwtCapabilities* aidl_twt_capabs);
bool convertAidlTwtRequestToLegacy(const TwtRequest aidl_twt_request,
                                   legacy_hal::wifi_twt_request* legacy_twt_request);
IWifiStaIfaceEventCallback::TwtErrorCode convertLegacyHalTwtErrorCodeToAidl(
        legacy_hal::wifi_twt_error_code legacy_error_code);
IWifiStaIfaceEventCallback::TwtTeardownReasonCode convertLegacyHalTwtReasonCodeToAidl(
        legacy_hal::wifi_twt_teardown_reason_code legacy_reason_code);
bool convertLegacyHalTwtSessionToAidl(legacy_hal::wifi_twt_session twt_session,
                                      TwtSession* aidl_twt_session);
bool convertLegacyHalTwtSessionStatsToAidl(legacy_hal::wifi_twt_session_stats twt_stats,
                                           TwtSessionStats* aidl_twt_stats);
}  // namespace aidl_struct_util
}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // AIDL_STRUCT_UTIL_H_
