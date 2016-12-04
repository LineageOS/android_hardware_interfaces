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

#include <android-base/logging.h>
#include <utils/SystemClock.h>

#include "hidl_struct_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
namespace hidl_struct_util {

uint8_t convertHidlScanReportEventFlagToLegacy(
    StaBackgroundScanBucketEventReportSchemeMask hidl_flag) {
  using HidlFlag = StaBackgroundScanBucketEventReportSchemeMask;
  switch (hidl_flag) {
    case HidlFlag::EACH_SCAN:
      return REPORT_EVENTS_EACH_SCAN;
    case HidlFlag::FULL_RESULTS:
      return REPORT_EVENTS_FULL_RESULTS;
    case HidlFlag::NO_BATCH:
      return REPORT_EVENTS_NO_BATCH;
  };
}

StaScanDataFlagMask convertLegacyScanDataFlagToHidl(uint8_t legacy_flag) {
  switch (legacy_flag) {
    case legacy_hal::WIFI_SCAN_FLAG_INTERRUPTED:
      return StaScanDataFlagMask::INTERRUPTED;
  };
  CHECK(false) << "Unknown legacy flag: " << legacy_flag;
  // To silence the compiler warning about reaching the end of non-void
  // function.
  return {};
}

IWifiChip::ChipCapabilityMask convertLegacyLoggerFeatureToHidlChipCapability(
    uint32_t feature) {
  using HidlChipCaps = IWifiChip::ChipCapabilityMask;
  switch (feature) {
    case legacy_hal::WIFI_LOGGER_MEMORY_DUMP_SUPPORTED:
      return HidlChipCaps::DEBUG_MEMORY_FIRMWARE_DUMP_SUPPORTED;
    case legacy_hal::WIFI_LOGGER_DRIVER_DUMP_SUPPORTED:
      return HidlChipCaps::DEBUG_MEMORY_DRIVER_DUMP_SUPPORTED;
    case legacy_hal::WIFI_LOGGER_CONNECT_EVENT_SUPPORTED:
      return HidlChipCaps::DEBUG_RING_BUFFER_CONNECT_EVENT_SUPPORTED;
    case legacy_hal::WIFI_LOGGER_POWER_EVENT_SUPPORTED:
      return HidlChipCaps::DEBUG_RING_BUFFER_POWER_EVENT_SUPPORTED;
    case legacy_hal::WIFI_LOGGER_WAKE_LOCK_SUPPORTED:
      return HidlChipCaps::DEBUG_RING_BUFFER_WAKELOCK_EVENT_SUPPORTED;
  };
  CHECK(false) << "Unknown legacy feature: " << feature;
  return {};
}

IWifiStaIface::StaIfaceCapabilityMask
convertLegacyLoggerFeatureToHidlStaIfaceCapability(uint32_t feature) {
  using HidlStaIfaceCaps = IWifiStaIface::StaIfaceCapabilityMask;
  switch (feature) {
    case legacy_hal::WIFI_LOGGER_PACKET_FATE_SUPPORTED:
      return HidlStaIfaceCaps::DEBUG_PACKET_FATE_SUPPORTED;
  };
  CHECK(false) << "Unknown legacy feature: " << feature;
  return {};
}

IWifiStaIface::StaIfaceCapabilityMask
convertLegacyFeatureToHidlStaIfaceCapability(uint32_t feature) {
  using HidlStaIfaceCaps = IWifiStaIface::StaIfaceCapabilityMask;
  switch (feature) {
    case WIFI_FEATURE_GSCAN:
      return HidlStaIfaceCaps::BACKGROUND_SCAN;
    case WIFI_FEATURE_LINK_LAYER_STATS:
      return HidlStaIfaceCaps::LINK_LAYER_STATS;
  };
  CHECK(false) << "Unknown legacy feature: " << feature;
  return {};
}

bool convertLegacyFeaturesToHidlChipCapabilities(
    uint32_t legacy_logger_feature_set, uint32_t* hidl_caps) {
  if (!hidl_caps) {
    return false;
  }
  *hidl_caps = 0;
  using HidlChipCaps = IWifiChip::ChipCapabilityMask;
  for (const auto feature : {legacy_hal::WIFI_LOGGER_MEMORY_DUMP_SUPPORTED,
                             legacy_hal::WIFI_LOGGER_DRIVER_DUMP_SUPPORTED,
                             legacy_hal::WIFI_LOGGER_CONNECT_EVENT_SUPPORTED,
                             legacy_hal::WIFI_LOGGER_POWER_EVENT_SUPPORTED,
                             legacy_hal::WIFI_LOGGER_WAKE_LOCK_SUPPORTED}) {
    if (feature & legacy_logger_feature_set) {
      *hidl_caps |= convertLegacyLoggerFeatureToHidlChipCapability(feature);
    }
  }
  // There is no flags for these 2 in the legacy feature set. Adding it to the
  // set because all the current devices support it.
  *hidl_caps |= HidlChipCaps::DEBUG_RING_BUFFER_VENDOR_DATA_SUPPORTED;
  *hidl_caps |= HidlChipCaps::DEBUG_HOST_WAKE_REASON_STATS;
  return true;
}

WifiDebugRingBufferFlags convertLegacyDebugRingBufferFlagsToHidl(
    uint32_t flag) {
  switch (flag) {
    case WIFI_RING_BUFFER_FLAG_HAS_BINARY_ENTRIES:
      return WifiDebugRingBufferFlags::HAS_BINARY_ENTRIES;
    case WIFI_RING_BUFFER_FLAG_HAS_ASCII_ENTRIES:
      return WifiDebugRingBufferFlags::HAS_ASCII_ENTRIES;
  };
  CHECK(false) << "Unknown legacy flag: " << flag;
  return {};
}

bool convertLegacyDebugRingBufferStatusToHidl(
    const legacy_hal::wifi_ring_buffer_status& legacy_status,
    WifiDebugRingBufferStatus* hidl_status) {
  if (!hidl_status) {
    return false;
  }
  hidl_status->ringName = reinterpret_cast<const char*>(legacy_status.name);
  for (const auto flag : {WIFI_RING_BUFFER_FLAG_HAS_BINARY_ENTRIES,
                          WIFI_RING_BUFFER_FLAG_HAS_ASCII_ENTRIES}) {
    if (flag & legacy_status.flags) {
      hidl_status->flags |=
          static_cast<std::underlying_type<WifiDebugRingBufferFlags>::type>(
              convertLegacyDebugRingBufferFlagsToHidl(flag));
    }
  }
  hidl_status->ringId = legacy_status.ring_id;
  hidl_status->sizeInBytes = legacy_status.ring_buffer_byte_size;
  // Calculate free size of the ring the buffer. We don't need to send the
  // exact read/write pointers that were there in the legacy HAL interface.
  if (legacy_status.written_bytes >= legacy_status.read_bytes) {
    hidl_status->freeSizeInBytes =
        legacy_status.ring_buffer_byte_size -
        (legacy_status.written_bytes - legacy_status.read_bytes);
  } else {
    hidl_status->freeSizeInBytes =
        legacy_status.read_bytes - legacy_status.written_bytes;
  }
  hidl_status->verboseLevel = legacy_status.verbose_level;
  return true;
}

bool convertLegacyVectorOfDebugRingBufferStatusToHidl(
    const std::vector<legacy_hal::wifi_ring_buffer_status>& legacy_status_vec,
    std::vector<WifiDebugRingBufferStatus>* hidl_status_vec) {
  if (!hidl_status_vec) {
    return false;
  }
  hidl_status_vec->clear();
  for (const auto& legacy_status : legacy_status_vec) {
    WifiDebugRingBufferStatus hidl_status;
    if (!convertLegacyDebugRingBufferStatusToHidl(legacy_status,
                                                  &hidl_status)) {
      return false;
    }
    hidl_status_vec->push_back(hidl_status);
  }
  return true;
}

bool convertLegacyWakeReasonStatsToHidl(
    const legacy_hal::WakeReasonStats& legacy_stats,
    WifiDebugHostWakeReasonStats* hidl_stats) {
  if (!hidl_stats) {
    return false;
  }
  hidl_stats->totalCmdEventWakeCnt =
      legacy_stats.wake_reason_cnt.total_cmd_event_wake;
  hidl_stats->cmdEventWakeCntPerType = legacy_stats.cmd_event_wake_cnt;
  hidl_stats->totalDriverFwLocalWakeCnt =
      legacy_stats.wake_reason_cnt.total_driver_fw_local_wake;
  hidl_stats->driverFwLocalWakeCntPerType =
      legacy_stats.driver_fw_local_wake_cnt;
  hidl_stats->totalRxPacketWakeCnt =
      legacy_stats.wake_reason_cnt.total_rx_data_wake;
  hidl_stats->rxPktWakeDetails.rxUnicastCnt =
      legacy_stats.wake_reason_cnt.rx_wake_details.rx_unicast_cnt;
  hidl_stats->rxPktWakeDetails.rxMulticastCnt =
      legacy_stats.wake_reason_cnt.rx_wake_details.rx_multicast_cnt;
  hidl_stats->rxPktWakeDetails.rxBroadcastCnt =
      legacy_stats.wake_reason_cnt.rx_wake_details.rx_broadcast_cnt;
  hidl_stats->rxMulticastPkWakeDetails.ipv4RxMulticastAddrCnt =
      legacy_stats.wake_reason_cnt.rx_multicast_wake_pkt_info
          .ipv4_rx_multicast_addr_cnt;
  hidl_stats->rxMulticastPkWakeDetails.ipv6RxMulticastAddrCnt =
      legacy_stats.wake_reason_cnt.rx_multicast_wake_pkt_info
          .ipv6_rx_multicast_addr_cnt;
  hidl_stats->rxMulticastPkWakeDetails.otherRxMulticastAddrCnt =
      legacy_stats.wake_reason_cnt.rx_multicast_wake_pkt_info
          .other_rx_multicast_addr_cnt;
  hidl_stats->rxIcmpPkWakeDetails.icmpPkt =
      legacy_stats.wake_reason_cnt.rx_wake_pkt_classification_info.icmp_pkt;
  hidl_stats->rxIcmpPkWakeDetails.icmp6Pkt =
      legacy_stats.wake_reason_cnt.rx_wake_pkt_classification_info.icmp6_pkt;
  hidl_stats->rxIcmpPkWakeDetails.icmp6Ra =
      legacy_stats.wake_reason_cnt.rx_wake_pkt_classification_info.icmp6_ra;
  hidl_stats->rxIcmpPkWakeDetails.icmp6Na =
      legacy_stats.wake_reason_cnt.rx_wake_pkt_classification_info.icmp6_na;
  hidl_stats->rxIcmpPkWakeDetails.icmp6Ns =
      legacy_stats.wake_reason_cnt.rx_wake_pkt_classification_info.icmp6_ns;
  return true;
}

bool convertLegacyFeaturesToHidlStaCapabilities(
    uint32_t legacy_feature_set,
    uint32_t legacy_logger_feature_set,
    uint32_t* hidl_caps) {
  if (!hidl_caps) {
    return false;
  }
  *hidl_caps = 0;
  using HidlStaIfaceCaps = IWifiStaIface::StaIfaceCapabilityMask;
  for (const auto feature : {legacy_hal::WIFI_LOGGER_PACKET_FATE_SUPPORTED}) {
    if (feature & legacy_logger_feature_set) {
      *hidl_caps |= convertLegacyLoggerFeatureToHidlStaIfaceCapability(feature);
    }
  }
  for (const auto feature :
       {WIFI_FEATURE_GSCAN, WIFI_FEATURE_LINK_LAYER_STATS}) {
    if (feature & legacy_feature_set) {
      *hidl_caps |= convertLegacyFeatureToHidlStaIfaceCapability(feature);
    }
  }
  // There is no flag for this one in the legacy feature set. Adding it to the
  // set because all the current devices support it.
  *hidl_caps |= HidlStaIfaceCaps::APF;
  return true;
}

bool convertLegacyApfCapabilitiesToHidl(
    const legacy_hal::PacketFilterCapabilities& legacy_caps,
    StaApfPacketFilterCapabilities* hidl_caps) {
  if (!hidl_caps) {
    return false;
  }
  hidl_caps->version = legacy_caps.version;
  hidl_caps->maxLength = legacy_caps.max_len;
  return true;
}

bool convertLegacyScanCapabilitiesToHidl(
    const legacy_hal::wifi_gscan_capabilities& legacy_caps,
    StaBackgroundScanCapabilities* hidl_caps) {
  if (!hidl_caps) {
    return false;
  }
  hidl_caps->maxCacheSize = legacy_caps.max_scan_cache_size;
  hidl_caps->maxBuckets = legacy_caps.max_scan_buckets;
  hidl_caps->maxApCachePerScan = legacy_caps.max_ap_cache_per_scan;
  hidl_caps->maxReportingThreshold = legacy_caps.max_scan_reporting_threshold;
  return true;
}

bool convertHidlScanParamsToLegacy(
    const StaBackgroundScanParameters& hidl_scan_params,
    legacy_hal::wifi_scan_cmd_params* legacy_scan_params) {
  if (!legacy_scan_params) {
    return false;
  }
  legacy_scan_params->base_period = hidl_scan_params.basePeriodInMs;
  legacy_scan_params->max_ap_per_scan = hidl_scan_params.maxApPerScan;
  legacy_scan_params->report_threshold_percent =
      hidl_scan_params.reportThresholdPercent;
  legacy_scan_params->report_threshold_num_scans =
      hidl_scan_params.reportThresholdNumScans;
  // TODO(b/33194311): Expose these max limits in the HIDL interface.
  if (hidl_scan_params.buckets.size() > MAX_BUCKETS) {
    return false;
  }
  legacy_scan_params->num_buckets = hidl_scan_params.buckets.size();
  for (uint32_t bucket_idx = 0; bucket_idx < hidl_scan_params.buckets.size();
       bucket_idx++) {
    const StaBackgroundScanBucketParameters& hidl_bucket_spec =
        hidl_scan_params.buckets[bucket_idx];
    legacy_hal::wifi_scan_bucket_spec& legacy_bucket_spec =
        legacy_scan_params->buckets[bucket_idx];
    legacy_bucket_spec.bucket = bucket_idx;
    legacy_bucket_spec.band =
        static_cast<legacy_hal::wifi_band>(hidl_bucket_spec.band);
    legacy_bucket_spec.period = hidl_bucket_spec.periodInMs;
    legacy_bucket_spec.max_period = hidl_bucket_spec.exponentialMaxPeriodInMs;
    legacy_bucket_spec.base = hidl_bucket_spec.exponentialBase;
    legacy_bucket_spec.step_count = hidl_bucket_spec.exponentialStepCount;
    legacy_bucket_spec.report_events = 0;
    using HidlFlag = StaBackgroundScanBucketEventReportSchemeMask;
    for (const auto flag :
         {HidlFlag::EACH_SCAN, HidlFlag::FULL_RESULTS, HidlFlag::NO_BATCH}) {
      if (hidl_bucket_spec.eventReportScheme &
          static_cast<std::underlying_type<HidlFlag>::type>(flag)) {
        legacy_bucket_spec.report_events |=
            convertHidlScanReportEventFlagToLegacy(flag);
      }
    }
    // TODO(b/33194311): Expose these max limits in the HIDL interface.
    if (hidl_bucket_spec.frequencies.size() > MAX_CHANNELS) {
      return false;
    }
    legacy_bucket_spec.num_channels = hidl_bucket_spec.frequencies.size();
    for (uint32_t freq_idx = 0; freq_idx < hidl_bucket_spec.frequencies.size();
         freq_idx++) {
      legacy_bucket_spec.channels[freq_idx].channel =
          hidl_bucket_spec.frequencies[freq_idx];
    }
  }
  return true;
}

bool convertLegacyIeToHidl(
    const legacy_hal::wifi_information_element& legacy_ie,
    WifiInformationElement* hidl_ie) {
  if (!hidl_ie) {
    return false;
  }
  hidl_ie->id = legacy_ie.id;
  hidl_ie->data =
      std::vector<uint8_t>(legacy_ie.data, legacy_ie.data + legacy_ie.len);
  return true;
}

bool convertLegacyIeBlobToHidl(const uint8_t* ie_blob,
                               uint32_t ie_blob_len,
                               std::vector<WifiInformationElement>* hidl_ies) {
  if (!ie_blob || !hidl_ies) {
    return false;
  }
  const uint8_t* ies_begin = ie_blob;
  const uint8_t* ies_end = ie_blob + ie_blob_len;
  const uint8_t* next_ie = ies_begin;
  using wifi_ie = legacy_hal::wifi_information_element;
  constexpr size_t kIeHeaderLen = sizeof(wifi_ie);
  // Each IE should atleast have the header (i.e |id| & |len| fields).
  while (next_ie + kIeHeaderLen <= ies_end) {
    const wifi_ie& legacy_ie = (*reinterpret_cast<const wifi_ie*>(next_ie));
    uint32_t curr_ie_len = kIeHeaderLen + legacy_ie.len;
    if (next_ie + curr_ie_len > ies_end) {
      return false;
    }
    WifiInformationElement hidl_ie;
    if (!convertLegacyIeToHidl(legacy_ie, &hidl_ie)) {
      return false;
    }
    hidl_ies->push_back(std::move(hidl_ie));
    next_ie += curr_ie_len;
  }
  // Ensure that the blob has been fully consumed.
  return (next_ie == ies_end);
}

bool convertLegacyScanResultToHidl(
    const legacy_hal::wifi_scan_result& legacy_scan_result,
    bool has_ie_data,
    StaScanResult* hidl_scan_result) {
  if (!hidl_scan_result) {
    return false;
  }
  hidl_scan_result->timeStampInUs = legacy_scan_result.ts;
  hidl_scan_result->ssid = std::vector<uint8_t>(
      legacy_scan_result.ssid,
      legacy_scan_result.ssid + sizeof(legacy_scan_result.ssid));
  memcpy(hidl_scan_result->bssid.data(),
         legacy_scan_result.bssid,
         hidl_scan_result->bssid.size());
  hidl_scan_result->frequency = legacy_scan_result.channel;
  hidl_scan_result->rssi = legacy_scan_result.rssi;
  hidl_scan_result->beaconPeriodInMs = legacy_scan_result.beacon_period;
  hidl_scan_result->capability = legacy_scan_result.capability;
  if (has_ie_data) {
    std::vector<WifiInformationElement> ies;
    if (!convertLegacyIeBlobToHidl(
            reinterpret_cast<const uint8_t*>(legacy_scan_result.ie_data),
            legacy_scan_result.ie_length,
            &ies)) {
      return false;
    }
    hidl_scan_result->informationElements = std::move(ies);
  }
  return true;
}

bool convertLegacyCachedScanResultsToHidl(
    const legacy_hal::wifi_cached_scan_results& legacy_cached_scan_result,
    StaScanData* hidl_scan_data) {
  if (!hidl_scan_data) {
    return false;
  }
  for (const auto flag : {legacy_hal::WIFI_SCAN_FLAG_INTERRUPTED}) {
    if (legacy_cached_scan_result.flags & flag) {
      hidl_scan_data->flags |=
          static_cast<std::underlying_type<StaScanDataFlagMask>::type>(
              convertLegacyScanDataFlagToHidl(flag));
    }
  }
  hidl_scan_data->bucketsScanned = legacy_cached_scan_result.buckets_scanned;

  CHECK(legacy_cached_scan_result.num_results >= 0 &&
        legacy_cached_scan_result.num_results <= MAX_AP_CACHE_PER_SCAN);
  std::vector<StaScanResult> hidl_scan_results;
  for (int32_t result_idx = 0;
       result_idx < legacy_cached_scan_result.num_results;
       result_idx++) {
    StaScanResult hidl_scan_result;
    if (!convertLegacyScanResultToHidl(
            legacy_cached_scan_result.results[result_idx],
            false,
            &hidl_scan_result)) {
      return false;
    }
    hidl_scan_results.push_back(hidl_scan_result);
  }
  hidl_scan_data->results = std::move(hidl_scan_results);
  return true;
}

bool convertLegacyVectorOfCachedScanResultsToHidl(
    const std::vector<legacy_hal::wifi_cached_scan_results>&
        legacy_cached_scan_results,
    std::vector<StaScanData>* hidl_scan_datas) {
  if (!hidl_scan_datas) {
    return false;
  }
  for (const auto& legacy_cached_scan_result : legacy_cached_scan_results) {
    StaScanData hidl_scan_data;
    if (!convertLegacyCachedScanResultsToHidl(legacy_cached_scan_result,
                                              &hidl_scan_data)) {
      return false;
    }
    hidl_scan_datas->push_back(hidl_scan_data);
  }
  return true;
}

WifiDebugTxPacketFate convertLegacyDebugTxPacketFateToHidl(
    legacy_hal::wifi_tx_packet_fate fate) {
  switch (fate) {
    case legacy_hal::TX_PKT_FATE_ACKED:
      return WifiDebugTxPacketFate::ACKED;
    case legacy_hal::TX_PKT_FATE_SENT:
      return WifiDebugTxPacketFate::SENT;
    case legacy_hal::TX_PKT_FATE_FW_QUEUED:
      return WifiDebugTxPacketFate::FW_QUEUED;
    case legacy_hal::TX_PKT_FATE_FW_DROP_INVALID:
      return WifiDebugTxPacketFate::FW_DROP_INVALID;
    case legacy_hal::TX_PKT_FATE_FW_DROP_NOBUFS:
      return WifiDebugTxPacketFate::FW_DROP_NOBUFS;
    case legacy_hal::TX_PKT_FATE_FW_DROP_OTHER:
      return WifiDebugTxPacketFate::FW_DROP_OTHER;
    case legacy_hal::TX_PKT_FATE_DRV_QUEUED:
      return WifiDebugTxPacketFate::DRV_QUEUED;
    case legacy_hal::TX_PKT_FATE_DRV_DROP_INVALID:
      return WifiDebugTxPacketFate::DRV_DROP_INVALID;
    case legacy_hal::TX_PKT_FATE_DRV_DROP_NOBUFS:
      return WifiDebugTxPacketFate::DRV_DROP_NOBUFS;
    case legacy_hal::TX_PKT_FATE_DRV_DROP_OTHER:
      return WifiDebugTxPacketFate::DRV_DROP_OTHER;
  };
  CHECK(false) << "Unknown legacy fate type: " << fate;
}

WifiDebugRxPacketFate convertLegacyDebugRxPacketFateToHidl(
    legacy_hal::wifi_rx_packet_fate fate) {
  switch (fate) {
    case legacy_hal::RX_PKT_FATE_SUCCESS:
      return WifiDebugRxPacketFate::SUCCESS;
    case legacy_hal::RX_PKT_FATE_FW_QUEUED:
      return WifiDebugRxPacketFate::FW_QUEUED;
    case legacy_hal::RX_PKT_FATE_FW_DROP_FILTER:
      return WifiDebugRxPacketFate::FW_DROP_FILTER;
    case legacy_hal::RX_PKT_FATE_FW_DROP_INVALID:
      return WifiDebugRxPacketFate::FW_DROP_INVALID;
    case legacy_hal::RX_PKT_FATE_FW_DROP_NOBUFS:
      return WifiDebugRxPacketFate::FW_DROP_NOBUFS;
    case legacy_hal::RX_PKT_FATE_FW_DROP_OTHER:
      return WifiDebugRxPacketFate::FW_DROP_OTHER;
    case legacy_hal::RX_PKT_FATE_DRV_QUEUED:
      return WifiDebugRxPacketFate::DRV_QUEUED;
    case legacy_hal::RX_PKT_FATE_DRV_DROP_FILTER:
      return WifiDebugRxPacketFate::DRV_DROP_FILTER;
    case legacy_hal::RX_PKT_FATE_DRV_DROP_INVALID:
      return WifiDebugRxPacketFate::DRV_DROP_INVALID;
    case legacy_hal::RX_PKT_FATE_DRV_DROP_NOBUFS:
      return WifiDebugRxPacketFate::DRV_DROP_NOBUFS;
    case legacy_hal::RX_PKT_FATE_DRV_DROP_OTHER:
      return WifiDebugRxPacketFate::DRV_DROP_OTHER;
  };
  CHECK(false) << "Unknown legacy fate type: " << fate;
}

WifiDebugPacketFateFrameType convertLegacyDebugPacketFateFrameTypeToHidl(
    legacy_hal::frame_type type) {
  switch (type) {
    case legacy_hal::FRAME_TYPE_UNKNOWN:
      return WifiDebugPacketFateFrameType::UNKNOWN;
    case legacy_hal::FRAME_TYPE_ETHERNET_II:
      return WifiDebugPacketFateFrameType::ETHERNET_II;
    case legacy_hal::FRAME_TYPE_80211_MGMT:
      return WifiDebugPacketFateFrameType::MGMT_80211;
  };
  CHECK(false) << "Unknown legacy frame type: " << type;
}

bool convertLegacyDebugPacketFateFrameToHidl(
    const legacy_hal::frame_info& legacy_frame,
    WifiDebugPacketFateFrameInfo* hidl_frame) {
  if (!hidl_frame) {
    return false;
  }
  hidl_frame->frameType =
      convertLegacyDebugPacketFateFrameTypeToHidl(legacy_frame.payload_type);
  hidl_frame->frameLen = legacy_frame.frame_len;
  hidl_frame->driverTimestampUsec = legacy_frame.driver_timestamp_usec;
  hidl_frame->firmwareTimestampUsec = legacy_frame.firmware_timestamp_usec;
  const uint8_t* frame_begin = reinterpret_cast<const uint8_t*>(
      legacy_frame.frame_content.ethernet_ii_bytes);
  hidl_frame->frameContent =
      std::vector<uint8_t>(frame_begin, frame_begin + legacy_frame.frame_len);
  return true;
}

bool convertLegacyDebugTxPacketFateToHidl(
    const legacy_hal::wifi_tx_report& legacy_fate,
    WifiDebugTxPacketFateReport* hidl_fate) {
  if (!hidl_fate) {
    return false;
  }
  hidl_fate->fate = convertLegacyDebugTxPacketFateToHidl(legacy_fate.fate);
  return convertLegacyDebugPacketFateFrameToHidl(legacy_fate.frame_inf,
                                                 &hidl_fate->frameInfo);
}

bool convertLegacyVectorOfDebugTxPacketFateToHidl(
    const std::vector<legacy_hal::wifi_tx_report>& legacy_fates,
    std::vector<WifiDebugTxPacketFateReport>* hidl_fates) {
  if (!hidl_fates) {
    return false;
  }
  hidl_fates->clear();
  for (const auto& legacy_fate : legacy_fates) {
    WifiDebugTxPacketFateReport hidl_fate;
    if (!convertLegacyDebugTxPacketFateToHidl(legacy_fate, &hidl_fate)) {
      return false;
    }
    hidl_fates->push_back(hidl_fate);
  }
  return true;
}

bool convertLegacyDebugRxPacketFateToHidl(
    const legacy_hal::wifi_rx_report& legacy_fate,
    WifiDebugRxPacketFateReport* hidl_fate) {
  if (!hidl_fate) {
    return false;
  }
  hidl_fate->fate = convertLegacyDebugRxPacketFateToHidl(legacy_fate.fate);
  return convertLegacyDebugPacketFateFrameToHidl(legacy_fate.frame_inf,
                                                 &hidl_fate->frameInfo);
}

bool convertLegacyVectorOfDebugRxPacketFateToHidl(
    const std::vector<legacy_hal::wifi_rx_report>& legacy_fates,
    std::vector<WifiDebugRxPacketFateReport>* hidl_fates) {
  if (!hidl_fates) {
    return false;
  }
  hidl_fates->clear();
  for (const auto& legacy_fate : legacy_fates) {
    WifiDebugRxPacketFateReport hidl_fate;
    if (!convertLegacyDebugRxPacketFateToHidl(legacy_fate, &hidl_fate)) {
      return false;
    }
    hidl_fates->push_back(hidl_fate);
  }
  return true;
}

bool convertLegacyLinkLayerStatsToHidl(
    const legacy_hal::LinkLayerStats& legacy_stats,
    StaLinkLayerStats* hidl_stats) {
  if (!hidl_stats) {
    return false;
  }
  // iface legacy_stats conversion.
  hidl_stats->iface.beaconRx = legacy_stats.iface.beacon_rx;
  hidl_stats->iface.avgRssiMgmt = legacy_stats.iface.rssi_mgmt;
  hidl_stats->iface.wmeBePktStats.rxMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].rx_mpdu;
  hidl_stats->iface.wmeBePktStats.txMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].tx_mpdu;
  hidl_stats->iface.wmeBePktStats.lostMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].mpdu_lost;
  hidl_stats->iface.wmeBePktStats.retries =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_BE].retries;
  hidl_stats->iface.wmeBkPktStats.rxMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].rx_mpdu;
  hidl_stats->iface.wmeBkPktStats.txMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].tx_mpdu;
  hidl_stats->iface.wmeBkPktStats.lostMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].mpdu_lost;
  hidl_stats->iface.wmeBkPktStats.retries =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_BK].retries;
  hidl_stats->iface.wmeViPktStats.rxMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].rx_mpdu;
  hidl_stats->iface.wmeViPktStats.txMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].tx_mpdu;
  hidl_stats->iface.wmeViPktStats.lostMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].mpdu_lost;
  hidl_stats->iface.wmeViPktStats.retries =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_VI].retries;
  hidl_stats->iface.wmeVoPktStats.rxMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].rx_mpdu;
  hidl_stats->iface.wmeVoPktStats.txMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].tx_mpdu;
  hidl_stats->iface.wmeVoPktStats.lostMpdu =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].mpdu_lost;
  hidl_stats->iface.wmeVoPktStats.retries =
      legacy_stats.iface.ac[legacy_hal::WIFI_AC_VO].retries;
  // radio legacy_stats conversion.
  hidl_stats->radio.onTimeInMs = legacy_stats.radio.on_time;
  hidl_stats->radio.txTimeInMs = legacy_stats.radio.tx_time;
  hidl_stats->radio.rxTimeInMs = legacy_stats.radio.rx_time;
  hidl_stats->radio.onTimeInMsForScan = legacy_stats.radio.on_time_scan;
  hidl_stats->radio.txTimeInMsPerLevel = legacy_stats.radio_tx_time_per_levels;
  // Timestamp in the HAL wrapper here since it's not provided in the legacy
  // HAL API.
  hidl_stats->timeStampInMs = uptimeMillis();
  return true;
}

legacy_hal::NanPublishType convertHidlNanPublishTypeToLegacy(
    NanPublishType type) {
  switch (type) {
    case NanPublishType::UNSOLICITED:
      return legacy_hal::NAN_PUBLISH_TYPE_UNSOLICITED;
    case NanPublishType::SOLICITED:
      return legacy_hal::NAN_PUBLISH_TYPE_SOLICITED;
    case NanPublishType::UNSOLICITED_SOLICITED:
      return legacy_hal::NAN_PUBLISH_TYPE_UNSOLICITED_SOLICITED;
  };
  CHECK(false);
}

legacy_hal::NanTxType convertHidlNanTxTypeToLegacy(NanTxType type) {
  switch (type) {
    case NanTxType::BROADCAST:
      return legacy_hal::NAN_TX_TYPE_BROADCAST;
    case NanTxType::UNICAST:
      return legacy_hal::NAN_TX_TYPE_UNICAST;
  };
  CHECK(false);
}

legacy_hal::NanMatchAlg convertHidlNanMatchAlgToLegacy(NanMatchAlg type) {
  switch (type) {
    case NanMatchAlg::MATCH_ONCE:
      return legacy_hal::NAN_MATCH_ALG_MATCH_ONCE;
    case NanMatchAlg::MATCH_CONTINUOUS:
      return legacy_hal::NAN_MATCH_ALG_MATCH_CONTINUOUS;
    case NanMatchAlg::MATCH_NEVER:
      return legacy_hal::NAN_MATCH_ALG_MATCH_NEVER;
  };
  CHECK(false);
}

legacy_hal::NanSubscribeType convertHidlNanSubscribeTypeToLegacy(
    NanSubscribeType type) {
  switch (type) {
    case NanSubscribeType::ACTIVE:
      return legacy_hal::NAN_SUBSCRIBE_TYPE_ACTIVE;
    case NanSubscribeType::PASSIVE:
      return legacy_hal::NAN_SUBSCRIBE_TYPE_PASSIVE;
  };
  CHECK(false);
}

legacy_hal::NanSRFType convertHidlNanSrfTypeToLegacy(NanSrfType type) {
  switch (type) {
    case NanSrfType::BLOOM_FILTER:
      return legacy_hal::NAN_SRF_ATTR_BLOOM_FILTER;
    case NanSrfType::PARTIAL_MAC_ADDR:
      return legacy_hal::NAN_SRF_ATTR_PARTIAL_MAC_ADDR;
  };
  CHECK(false);
}

legacy_hal::NanSRFIncludeType convertHidlNanSrfIncludeTypeToLegacy(
    NanSrfIncludeType type) {
  switch (type) {
    case NanSrfIncludeType::DO_NOT_RESPOND:
      return legacy_hal::NAN_SRF_INCLUDE_DO_NOT_RESPOND;
    case NanSrfIncludeType::RESPOND:
      return legacy_hal::NAN_SRF_INCLUDE_RESPOND;
  };
  CHECK(false);
}

NanStatusType convertLegacyNanStatusTypeToHidl(
    legacy_hal::NanStatusType /* type */) {
  // TODO: The |NanStatusType| has changed in legacy HAL and no longer in sync
  // with the HIDL interface.
  return NanStatusType::SUCCESS;
}

NanResponseType convertLegacyNanResponseTypeToHidl(
    legacy_hal::NanResponseType type) {
  switch (type) {
    case legacy_hal::NAN_RESPONSE_ENABLED:
      return NanResponseType::ENABLED;
    case legacy_hal::NAN_RESPONSE_DISABLED:
      return NanResponseType::DISABLED;
    case legacy_hal::NAN_RESPONSE_PUBLISH:
      return NanResponseType::PUBLISH;
    case legacy_hal::NAN_RESPONSE_PUBLISH_CANCEL:
      return NanResponseType::PUBLISH_CANCEL;
    case legacy_hal::NAN_RESPONSE_TRANSMIT_FOLLOWUP:
      return NanResponseType::TRANSMIT_FOLLOWUP;
    case legacy_hal::NAN_RESPONSE_SUBSCRIBE:
      return NanResponseType::SUBSCRIBE;
    case legacy_hal::NAN_RESPONSE_SUBSCRIBE_CANCEL:
      return NanResponseType::SUBSCRIBE_CANCEL;
    case legacy_hal::NAN_RESPONSE_STATS:
      // Not present in HIDL. Is going to be deprecated in legacy HAL as well.
      CHECK(0);
    case legacy_hal::NAN_RESPONSE_CONFIG:
      return NanResponseType::CONFIG;
    case legacy_hal::NAN_RESPONSE_TCA:
      // Not present in HIDL. Is going to be deprecated in legacy HAL as well.
      CHECK(0);
    case legacy_hal::NAN_RESPONSE_ERROR:
      return NanResponseType::ERROR;
    case legacy_hal::NAN_RESPONSE_BEACON_SDF_PAYLOAD:
      return NanResponseType::BEACON_SDF_PAYLOAD;
    case legacy_hal::NAN_GET_CAPABILITIES:
      return NanResponseType::GET_CAPABILITIES;
    case legacy_hal::NAN_DP_INTERFACE_CREATE:
      return NanResponseType::DP_INTERFACE_CREATE;
    case legacy_hal::NAN_DP_INTERFACE_DELETE:
      return NanResponseType::DP_INTERFACE_DELETE;
    case legacy_hal::NAN_DP_INITIATOR_RESPONSE:
      return NanResponseType::DP_INITIATOR_RESPONSE;
    case legacy_hal::NAN_DP_RESPONDER_RESPONSE:
      return NanResponseType::DP_RESPONDER_RESPONSE;
    case legacy_hal::NAN_DP_END:
      return NanResponseType::DP_END;
  };
  CHECK(false) << "Unknown legacy type: " << type;
}

bool convertHidlNanEnableRequestToLegacy(
    const NanEnableRequest& hidl_request,
    legacy_hal::NanEnableRequest* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  legacy_request->master_pref = hidl_request.masterPref;
  legacy_request->cluster_low = hidl_request.clusterLow;
  legacy_request->cluster_high = hidl_request.clusterHigh;
  legacy_request->config_support_5g = hidl_request.validSupport5gVal;
  legacy_request->support_5g_val = hidl_request.support5gVal;
  legacy_request->config_sid_beacon = hidl_request.validSidBeaconVal;
  legacy_request->sid_beacon_val = hidl_request.sidBeaconVal;
  legacy_request->config_2dot4g_rssi_close =
      hidl_request.valid2dot4gRssiCloseVal;
  legacy_request->rssi_close_2dot4g_val = hidl_request.rssiClose2dot4gVal;
  legacy_request->config_2dot4g_rssi_middle =
      hidl_request.valid2dot4gRssiMiddleVal;
  legacy_request->rssi_middle_2dot4g_val = hidl_request.rssiMiddle2dot4gVal;
  legacy_request->config_2dot4g_rssi_proximity =
      hidl_request.valid2dot4gRssiProximityVal;
  legacy_request->rssi_proximity_2dot4g_val =
      hidl_request.rssiProximity2dot4gVal;
  legacy_request->config_hop_count_limit = hidl_request.validHopCountLimitVal;
  legacy_request->hop_count_limit_val = hidl_request.hopCountLimitVal;
  legacy_request->config_2dot4g_support = hidl_request.valid2dot4gSupportVal;
  legacy_request->support_2dot4g_val = hidl_request.support2dot4gVal;
  legacy_request->config_2dot4g_beacons = hidl_request.valid2dot4gBeaconsVal;
  legacy_request->beacon_2dot4g_val = hidl_request.beacon2dot4gVal;
  legacy_request->config_2dot4g_sdf = hidl_request.valid2dot4gSdfVal;
  legacy_request->sdf_2dot4g_val = hidl_request.sdf2dot4gVal;
  legacy_request->config_5g_beacons = hidl_request.valid5gBeaconsVal;
  legacy_request->beacon_5g_val = hidl_request.beacon5gVal;
  legacy_request->config_5g_sdf = hidl_request.valid5gSdfVal;
  legacy_request->sdf_5g_val = hidl_request.sdf5gVal;
  legacy_request->config_5g_rssi_close = hidl_request.valid5gRssiCloseVal;
  legacy_request->rssi_close_5g_val = hidl_request.rssiClose5gVal;
  legacy_request->config_5g_rssi_middle = hidl_request.valid5gRssiMiddleVal;
  legacy_request->rssi_middle_5g_val = hidl_request.rssiMiddle5gVal;
  legacy_request->config_5g_rssi_close_proximity =
      hidl_request.valid5gRssiCloseProximityVal;
  legacy_request->rssi_close_proximity_5g_val =
      hidl_request.rssiCloseProximity5gVal;
  legacy_request->config_rssi_window_size = hidl_request.validRssiWindowSizeVal;
  legacy_request->rssi_window_size_val = hidl_request.rssiWindowSizeVal;
  legacy_request->config_oui = hidl_request.validOuiVal;
  legacy_request->oui_val = hidl_request.ouiVal;
  legacy_request->config_intf_addr = hidl_request.validIntfAddrVal;
  CHECK(hidl_request.intfAddrVal.size() ==
        sizeof(legacy_request->intf_addr_val));
  memcpy(legacy_request->intf_addr_val,
         hidl_request.intfAddrVal.data(),
         hidl_request.intfAddrVal.size());
  legacy_request->config_cluster_attribute_val =
      hidl_request.configClusterAttributeVal;
  legacy_request->config_scan_params = hidl_request.validScanParamsVal;
  if (hidl_request.scanParamsVal.dwellTime.size() >
      sizeof(legacy_request->scan_params_val.dwell_time)) {
    return false;
  }
  memcpy(legacy_request->scan_params_val.dwell_time,
         hidl_request.scanParamsVal.dwellTime.data(),
         hidl_request.scanParamsVal.dwellTime.size());
  if (hidl_request.scanParamsVal.scanPeriod.size() >
      sizeof(legacy_request->scan_params_val.scan_period)) {
    return false;
  }
  memcpy(legacy_request->scan_params_val.scan_period,
         hidl_request.scanParamsVal.scanPeriod.data(),
         hidl_request.scanParamsVal.scanPeriod.size());
  legacy_request->config_random_factor_force =
      hidl_request.validRandomFactorForceVal;
  legacy_request->random_factor_force_val = hidl_request.randomFactorForceVal;
  legacy_request->config_hop_count_force = hidl_request.validHopCountLimitVal;
  legacy_request->hop_count_force_val = hidl_request.hopCountLimitVal;
  legacy_request->config_24g_channel = hidl_request.valid24gChannelVal;
  legacy_request->channel_24g_val = hidl_request.channel24gVal;
  legacy_request->config_5g_channel = hidl_request.valid5gChannelVal;
  legacy_request->channel_5g_val = hidl_request.channel5gVal;
  return true;
}

bool convertHidlNanPublishRequestToLegacy(
    const NanPublishRequest& hidl_request,
    legacy_hal::NanPublishRequest* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  legacy_request->publish_id = hidl_request.publishId;
  legacy_request->ttl = hidl_request.ttl;
  legacy_request->period = hidl_request.period;
  legacy_request->publish_type =
      convertHidlNanPublishTypeToLegacy(hidl_request.publishType);
  legacy_request->tx_type = convertHidlNanTxTypeToLegacy(hidl_request.txType);
  legacy_request->publish_count = hidl_request.publishCount;
  if (hidl_request.serviceName.size() > sizeof(legacy_request->service_name)) {
    return false;
  }
  legacy_request->service_name_len = hidl_request.serviceName.size();
  memcpy(legacy_request->service_name,
         hidl_request.serviceName.c_str(),
         hidl_request.serviceName.size());
  legacy_request->publish_match_indicator =
      convertHidlNanMatchAlgToLegacy(hidl_request.publishMatchIndicator);
  if (hidl_request.serviceSpecificInfo.size() >
      sizeof(legacy_request->service_specific_info)) {
    return false;
  }
  legacy_request->service_specific_info_len =
      hidl_request.serviceSpecificInfo.size();
  memcpy(legacy_request->service_specific_info,
         hidl_request.serviceSpecificInfo.data(),
         hidl_request.serviceSpecificInfo.size());
  if (hidl_request.rxMatchFilter.size() >
      sizeof(legacy_request->rx_match_filter)) {
    return false;
  }
  legacy_request->rx_match_filter_len = hidl_request.rxMatchFilter.size();
  memcpy(legacy_request->rx_match_filter,
         hidl_request.rxMatchFilter.data(),
         hidl_request.rxMatchFilter.size());
  if (hidl_request.txMatchFilter.size() >
      sizeof(legacy_request->tx_match_filter)) {
    return false;
  }
  legacy_request->tx_match_filter_len = hidl_request.txMatchFilter.size();
  memcpy(legacy_request->tx_match_filter,
         hidl_request.txMatchFilter.data(),
         hidl_request.txMatchFilter.size());
  legacy_request->rssi_threshold_flag = hidl_request.useRssiThreshold;
  legacy_request->connmap = hidl_request.connmap;
  legacy_request->recv_indication_cfg = hidl_request.recvIndicationCfg;
  return true;
}

bool convertHidlNanPublishCancelRequestToLegacy(
    const NanPublishCancelRequest& hidl_request,
    legacy_hal::NanPublishCancelRequest* legacy_request) {
  legacy_request->publish_id = hidl_request.publishId;
  return true;
}

bool convertHidlNanSubscribeRequestToLegacy(
    const NanSubscribeRequest& hidl_request,
    legacy_hal::NanSubscribeRequest* legacy_request) {
  if (!legacy_request) {
    return false;
  }
  legacy_request->subscribe_id = hidl_request.subscribeId;
  legacy_request->ttl = hidl_request.ttl;
  legacy_request->period = hidl_request.period;
  legacy_request->subscribe_type =
      convertHidlNanSubscribeTypeToLegacy(hidl_request.subscribeType);
  legacy_request->serviceResponseFilter =
      convertHidlNanSrfTypeToLegacy(hidl_request.serviceResponseFilter);
  legacy_request->serviceResponseInclude =
      convertHidlNanSrfIncludeTypeToLegacy(hidl_request.serviceResponseInclude);
  legacy_request->useServiceResponseFilter =
      hidl_request.shouldUseServiceResponseFilter
          ? legacy_hal::NAN_USE_SRF
          : legacy_hal::NAN_DO_NOT_USE_SRF;
  legacy_request->ssiRequiredForMatchIndication =
      hidl_request.isSsiRequiredForMatchIndication
          ? legacy_hal::NAN_SSI_REQUIRED_IN_MATCH_IND
          : legacy_hal::NAN_SSI_NOT_REQUIRED_IN_MATCH_IND;
  legacy_request->subscribe_match_indicator =
      convertHidlNanMatchAlgToLegacy(hidl_request.subscribeMatchIndicator);
  legacy_request->subscribe_count = hidl_request.subscribeCount;
  if (hidl_request.serviceName.size() > sizeof(legacy_request->service_name)) {
    return false;
  }
  legacy_request->service_name_len = hidl_request.serviceName.size();
  memcpy(legacy_request->service_name,
         hidl_request.serviceName.c_str(),
         hidl_request.serviceName.size());
  if (hidl_request.serviceSpecificInfo.size() >
      sizeof(legacy_request->service_specific_info)) {
    return false;
  }
  legacy_request->service_specific_info_len =
      hidl_request.serviceSpecificInfo.size();
  memcpy(legacy_request->service_specific_info,
         hidl_request.serviceSpecificInfo.data(),
         hidl_request.serviceSpecificInfo.size());
  if (hidl_request.rxMatchFilter.size() >
      sizeof(legacy_request->rx_match_filter)) {
    return false;
  }
  legacy_request->rx_match_filter_len = hidl_request.rxMatchFilter.size();
  memcpy(legacy_request->rx_match_filter,
         hidl_request.rxMatchFilter.data(),
         hidl_request.rxMatchFilter.size());
  if (hidl_request.txMatchFilter.size() >
      sizeof(legacy_request->tx_match_filter)) {
    return false;
  }
  legacy_request->tx_match_filter_len = hidl_request.txMatchFilter.size();
  memcpy(legacy_request->tx_match_filter,
         hidl_request.txMatchFilter.data(),
         hidl_request.txMatchFilter.size());
  legacy_request->rssi_threshold_flag = hidl_request.useRssiThreshold;
  legacy_request->connmap = hidl_request.connmap;
  if (hidl_request.intfAddr.size() > NAN_MAX_SUBSCRIBE_MAX_ADDRESS) {
    return false;
  }
  legacy_request->num_intf_addr_present = hidl_request.intfAddr.size();
  for (uint32_t i = 0; i < hidl_request.intfAddr.size(); i++) {
    CHECK(hidl_request.intfAddr[i].size() ==
          sizeof(legacy_request->intf_addr[i]));
    memcpy(legacy_request->intf_addr[i],
           hidl_request.intfAddr[i].data(),
           hidl_request.intfAddr[i].size());
  }
  legacy_request->recv_indication_cfg = hidl_request.recvIndicationCfg;
  return true;
}

bool convertHidlNanSubscribeCancelRequestToLegacy(
    const NanSubscribeCancelRequest& /* hidl_request */,
    legacy_hal::NanSubscribeCancelRequest* /* legacy_request */) {
  return false;
}

bool convertHidlNanTransmitFollowupRequestToLegacy(
    const NanTransmitFollowupRequest& /* hidl_request */,
    legacy_hal::NanTransmitFollowupRequest* /* legacy_request */) {
  return false;
}

bool convertHidlNanConfigRequestToLegacy(
    const NanConfigRequest& /* hidl_request */,
    legacy_hal::NanConfigRequest* /* legacy_request */) {
  return false;
}

bool convertHidlNanBeaconSdfPayloadRequestToLegacy(
    const NanBeaconSdfPayloadRequest& /* hidl_request */,
    legacy_hal::NanBeaconSdfPayloadRequest* /* legacy_request */) {
  return false;
}

bool convertHidlNanDataPathInitiatorRequestToLegacy(
    const NanDataPathInitiatorRequest& /* hidl_request */,
    legacy_hal::NanDataPathInitiatorRequest* /* legacy_request */) {
  return false;
}

bool convertHidlNanDataPathIndicationResponseToLegacy(
    const NanDataPathIndicationResponse& /* hidl_response */,
    legacy_hal::NanDataPathIndicationResponse* /* legacy_response */) {
  return false;
}

bool convertHidlNanDataPathEndRequestToLegacy(
    const NanDataPathEndRequest& /* hidl_request */,
    legacy_hal::NanDataPathEndRequest* /* legacy_request */) {
  return false;
}

bool convertLegacyNanResponseHeaderToHidl(
    const legacy_hal::NanResponseMsg& legacy_response,
    NanResponseMsgHeader* hidl_response) {
  if (!hidl_response) {
    return false;
  }
  hidl_response->status =
      convertLegacyNanStatusTypeToHidl(legacy_response.status);
  hidl_response->value = legacy_response.value;
  hidl_response->responseType =
      convertLegacyNanResponseTypeToHidl(legacy_response.response_type);
  return true;
}

bool convertLegacyNanPublishResponseToHidl(
    const legacy_hal::NanPublishResponse& /* legacy_response */,
    NanPublishResponse* /* hidl_response */) {
  return false;
}

bool convertLegacyNanSubscribeResponseToHidl(
    const legacy_hal::NanSubscribeResponse& /* legacy_response */,
    NanSubscribeResponse* /* hidl_response */) {
  return false;
}

bool convertLegacyNanDataPathResponseToHidl(
    const legacy_hal::NanDataPathRequestResponse& /* legacy_response */,
    NanDataPathResponse* /* hidl_response */) {
  return false;
}

bool convertLegacyNanCapabilitiesResponseToHidl(
    const legacy_hal::NanCapabilities& /* legacy_response */,
    NanCapabilitiesResponse* /* hidl_response */) {
  return false;
}

bool convertLegacyNanPublishTerminatedIndToHidl(
    const legacy_hal::NanPublishTerminatedInd& /* legacy_ind */,
    NanPublishTerminatedInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanMatchIndToHidl(
    const legacy_hal::NanMatchInd& /* legacy_ind */,
    NanMatchInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanMatchExpiredIndToHidl(
    const legacy_hal::NanMatchExpiredInd& /* legacy_ind */,
    NanMatchExpiredInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanSubscribeTerminatedIndToHidl(
    const legacy_hal::NanSubscribeTerminatedInd& /* legacy_ind */,
    NanSubscribeTerminatedInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanFollowupIndToHidl(
    const legacy_hal::NanFollowupInd& /* legacy_ind */,
    NanFollowupInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanDiscEngEventIndToHidl(
    const legacy_hal::NanDiscEngEventInd& /* legacy_ind */,
    NanDiscEngEventInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanDisabledIndToHidl(
    const legacy_hal::NanDisabledInd& /* legacy_ind */,
    NanDisabledInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanBeaconSdfPayloadIndToHidl(
    const legacy_hal::NanBeaconSdfPayloadInd& /* legacy_ind */,
    NanBeaconSdfPayloadInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanDataPathRequestIndToHidl(
    const legacy_hal::NanDataPathRequestInd& /* legacy_ind */,
    NanDataPathRequestInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanDataPathConfirmIndToHidl(
    const legacy_hal::NanDataPathConfirmInd& /* legacy_ind */,
    NanDataPathConfirmInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanDataPathEndIndToHidl(
    const legacy_hal::NanDataPathEndInd& /* legacy_ind */,
    NanDataPathEndInd* /* hidl_ind */) {
  return false;
}

bool convertLegacyNanTransmitFollowupIndToHidl(
    const legacy_hal::NanTransmitFollowupInd& /* legacy_ind */,
    NanTransmitFollowupInd* /* hidl_ind */) {
  return false;
}

legacy_hal::wifi_rtt_type convertHidlRttTypeToLegacy(RttType type) {
  switch (type) {
    case RttType::ONE_SIDED:
      return legacy_hal::RTT_TYPE_1_SIDED;
    case RttType::TWO_SIDED:
      return legacy_hal::RTT_TYPE_2_SIDED;
  };
  CHECK(false);
}

RttType convertLegacyRttTypeToHidl(legacy_hal::wifi_rtt_type type) {
  switch (type) {
    case legacy_hal::RTT_TYPE_1_SIDED:
      return RttType::ONE_SIDED;
    case legacy_hal::RTT_TYPE_2_SIDED:
      return RttType::TWO_SIDED;
  };
  CHECK(false) << "Unknown legacy type: " << type;
}

legacy_hal::rtt_peer_type convertHidlRttPeerTypeToLegacy(RttPeerType type) {
  switch (type) {
    case RttPeerType::AP:
      return legacy_hal::RTT_PEER_AP;
    case RttPeerType::STA:
      return legacy_hal::RTT_PEER_STA;
    case RttPeerType::P2P_GO:
      return legacy_hal::RTT_PEER_P2P_GO;
    case RttPeerType::P2P_CLIENT:
      return legacy_hal::RTT_PEER_P2P_CLIENT;
    case RttPeerType::NAN:
      return legacy_hal::RTT_PEER_NAN;
  };
  CHECK(false);
}

legacy_hal::wifi_channel_width convertHidlWifiChannelWidthToLegacy(
    WifiChannelWidthInMhz type) {
  switch (type) {
    case WifiChannelWidthInMhz::WIDTH_20:
      return legacy_hal::WIFI_CHAN_WIDTH_20;
    case WifiChannelWidthInMhz::WIDTH_40:
      return legacy_hal::WIFI_CHAN_WIDTH_40;
    case WifiChannelWidthInMhz::WIDTH_80:
      return legacy_hal::WIFI_CHAN_WIDTH_80;
    case WifiChannelWidthInMhz::WIDTH_160:
      return legacy_hal::WIFI_CHAN_WIDTH_160;
    case WifiChannelWidthInMhz::WIDTH_80P80:
      return legacy_hal::WIFI_CHAN_WIDTH_80P80;
    case WifiChannelWidthInMhz::WIDTH_5:
      return legacy_hal::WIFI_CHAN_WIDTH_5;
    case WifiChannelWidthInMhz::WIDTH_10:
      return legacy_hal::WIFI_CHAN_WIDTH_10;
    case WifiChannelWidthInMhz::WIDTH_INVALID:
      return legacy_hal::WIFI_CHAN_WIDTH_INVALID;
  };
  CHECK(false);
}

WifiChannelWidthInMhz convertLegacyWifiChannelWidthToHidl(
    legacy_hal::wifi_channel_width type) {
  switch (type) {
    case legacy_hal::WIFI_CHAN_WIDTH_20:
      return WifiChannelWidthInMhz::WIDTH_20;
    case legacy_hal::WIFI_CHAN_WIDTH_40:
      return WifiChannelWidthInMhz::WIDTH_40;
    case legacy_hal::WIFI_CHAN_WIDTH_80:
      return WifiChannelWidthInMhz::WIDTH_80;
    case legacy_hal::WIFI_CHAN_WIDTH_160:
      return WifiChannelWidthInMhz::WIDTH_160;
    case legacy_hal::WIFI_CHAN_WIDTH_80P80:
      return WifiChannelWidthInMhz::WIDTH_80P80;
    case legacy_hal::WIFI_CHAN_WIDTH_5:
      return WifiChannelWidthInMhz::WIDTH_5;
    case legacy_hal::WIFI_CHAN_WIDTH_10:
      return WifiChannelWidthInMhz::WIDTH_10;
    case legacy_hal::WIFI_CHAN_WIDTH_INVALID:
      return WifiChannelWidthInMhz::WIDTH_INVALID;
  };
  CHECK(false) << "Unknown legacy type: " << type;
}

legacy_hal::wifi_rtt_preamble convertHidlRttPreambleToLegacy(RttPreamble type) {
  switch (type) {
    case RttPreamble::LEGACY:
      return legacy_hal::WIFI_RTT_PREAMBLE_LEGACY;
    case RttPreamble::HT:
      return legacy_hal::WIFI_RTT_PREAMBLE_HT;
    case RttPreamble::VHT:
      return legacy_hal::WIFI_RTT_PREAMBLE_VHT;
  };
  CHECK(false);
}

RttPreamble convertLegacyRttPreambleToHidl(legacy_hal::wifi_rtt_preamble type) {
  switch (type) {
    case legacy_hal::WIFI_RTT_PREAMBLE_LEGACY:
      return RttPreamble::LEGACY;
    case legacy_hal::WIFI_RTT_PREAMBLE_HT:
      return RttPreamble::HT;
    case legacy_hal::WIFI_RTT_PREAMBLE_VHT:
      return RttPreamble::VHT;
  };
  CHECK(false) << "Unknown legacy type: " << type;
}

legacy_hal::wifi_rtt_bw convertHidlRttBwToLegacy(RttBw type) {
  switch (type) {
    case RttBw::BW_5MHZ:
      return legacy_hal::WIFI_RTT_BW_5;
    case RttBw::BW_10MHZ:
      return legacy_hal::WIFI_RTT_BW_10;
    case RttBw::BW_20MHZ:
      return legacy_hal::WIFI_RTT_BW_20;
    case RttBw::BW_40MHZ:
      return legacy_hal::WIFI_RTT_BW_40;
    case RttBw::BW_80MHZ:
      return legacy_hal::WIFI_RTT_BW_80;
    case RttBw::BW_160MHZ:
      return legacy_hal::WIFI_RTT_BW_160;
  };
  CHECK(false);
}

RttBw convertLegacyRttBwToHidl(legacy_hal::wifi_rtt_bw type) {
  switch (type) {
    case legacy_hal::WIFI_RTT_BW_5:
      return RttBw::BW_5MHZ;
    case legacy_hal::WIFI_RTT_BW_10:
      return RttBw::BW_10MHZ;
    case legacy_hal::WIFI_RTT_BW_20:
      return RttBw::BW_20MHZ;
    case legacy_hal::WIFI_RTT_BW_40:
      return RttBw::BW_40MHZ;
    case legacy_hal::WIFI_RTT_BW_80:
      return RttBw::BW_80MHZ;
    case legacy_hal::WIFI_RTT_BW_160:
      return RttBw::BW_160MHZ;
  };
  CHECK(false) << "Unknown legacy type: " << type;
}

legacy_hal::wifi_motion_pattern convertHidlRttMotionPatternToLegacy(
    RttMotionPattern type) {
  switch (type) {
    case RttMotionPattern::NOT_EXPECTED:
      return legacy_hal::WIFI_MOTION_NOT_EXPECTED;
    case RttMotionPattern::EXPECTED:
      return legacy_hal::WIFI_MOTION_EXPECTED;
    case RttMotionPattern::UNKNOWN:
      return legacy_hal::WIFI_MOTION_UNKNOWN;
  };
  CHECK(false);
}

WifiRatePreamble convertLegacyWifiRatePreambleToHidl(uint8_t preamble) {
  switch (preamble) {
    case 0:
      return WifiRatePreamble::OFDM;
    case 1:
      return WifiRatePreamble::CCK;
    case 2:
      return WifiRatePreamble::HT;
    case 3:
      return WifiRatePreamble::VHT;
    default:
      return WifiRatePreamble::RESERVED;
  };
  CHECK(false) << "Unknown legacy preamble: " << preamble;
}

WifiRateNss convertLegacyWifiRateNssToHidl(uint8_t nss) {
  switch (nss) {
    case 0:
      return WifiRateNss::NSS_1x1;
    case 1:
      return WifiRateNss::NSS_2x2;
    case 2:
      return WifiRateNss::NSS_3x3;
    case 3:
      return WifiRateNss::NSS_4x4;
  };
  CHECK(false) << "Unknown legacy nss: " << nss;
  return {};
}

RttStatus convertLegacyRttStatusToHidl(legacy_hal::wifi_rtt_status status) {
  switch (status) {
    case legacy_hal::RTT_STATUS_SUCCESS:
      return RttStatus::SUCCESS;
    case legacy_hal::RTT_STATUS_FAILURE:
      return RttStatus::FAILURE;
    case legacy_hal::RTT_STATUS_FAIL_NO_RSP:
      return RttStatus::FAIL_NO_RSP;
    case legacy_hal::RTT_STATUS_FAIL_REJECTED:
      return RttStatus::FAIL_REJECTED;
    case legacy_hal::RTT_STATUS_FAIL_NOT_SCHEDULED_YET:
      return RttStatus::FAIL_NOT_SCHEDULED_YET;
    case legacy_hal::RTT_STATUS_FAIL_TM_TIMEOUT:
      return RttStatus::FAIL_TM_TIMEOUT;
    case legacy_hal::RTT_STATUS_FAIL_AP_ON_DIFF_CHANNEL:
      return RttStatus::FAIL_AP_ON_DIFF_CHANNEL;
    case legacy_hal::RTT_STATUS_FAIL_NO_CAPABILITY:
      return RttStatus::FAIL_NO_CAPABILITY;
    case legacy_hal::RTT_STATUS_ABORTED:
      return RttStatus::ABORTED;
    case legacy_hal::RTT_STATUS_FAIL_INVALID_TS:
      return RttStatus::FAIL_INVALID_TS;
    case legacy_hal::RTT_STATUS_FAIL_PROTOCOL:
      return RttStatus::FAIL_PROTOCOL;
    case legacy_hal::RTT_STATUS_FAIL_SCHEDULE:
      return RttStatus::FAIL_SCHEDULE;
    case legacy_hal::RTT_STATUS_FAIL_BUSY_TRY_LATER:
      return RttStatus::FAIL_BUSY_TRY_LATER;
    case legacy_hal::RTT_STATUS_INVALID_REQ:
      return RttStatus::INVALID_REQ;
    case legacy_hal::RTT_STATUS_NO_WIFI:
      return RttStatus::NO_WIFI;
    case legacy_hal::RTT_STATUS_FAIL_FTM_PARAM_OVERRIDE:
      return RttStatus::FAIL_FTM_PARAM_OVERRIDE;
  };
  CHECK(false) << "Unknown legacy status: " << status;
}

bool convertHidlWifiChannelInfoToLegacy(
    const WifiChannelInfo& hidl_info,
    legacy_hal::wifi_channel_info* legacy_info) {
  if (!legacy_info) {
    return false;
  }
  legacy_info->width = convertHidlWifiChannelWidthToLegacy(hidl_info.width);
  legacy_info->center_freq = hidl_info.centerFreq;
  legacy_info->center_freq0 = hidl_info.centerFreq0;
  legacy_info->center_freq1 = hidl_info.centerFreq1;
  return true;
}

bool convertLegacyWifiChannelInfoToHidl(
    const legacy_hal::wifi_channel_info& legacy_info,
    WifiChannelInfo* hidl_info) {
  if (!hidl_info) {
    return false;
  }
  hidl_info->width = convertLegacyWifiChannelWidthToHidl(legacy_info.width);
  hidl_info->centerFreq = legacy_info.center_freq;
  hidl_info->centerFreq0 = legacy_info.center_freq0;
  hidl_info->centerFreq1 = legacy_info.center_freq1;
  return true;
}

bool convertHidlRttConfigToLegacy(const RttConfig& hidl_config,
                                  legacy_hal::wifi_rtt_config* legacy_config) {
  if (!legacy_config) {
    return false;
  }
  CHECK(hidl_config.addr.size() == sizeof(legacy_config->addr));
  memcpy(legacy_config->addr, hidl_config.addr.data(), hidl_config.addr.size());
  legacy_config->type = convertHidlRttTypeToLegacy(hidl_config.type);
  legacy_config->peer = convertHidlRttPeerTypeToLegacy(hidl_config.peer);
  if (!convertHidlWifiChannelInfoToLegacy(hidl_config.channel,
                                          &legacy_config->channel)) {
    return false;
  }
  legacy_config->burst_period = hidl_config.burstPeriod;
  legacy_config->num_burst = hidl_config.numBurst;
  legacy_config->num_frames_per_burst = hidl_config.numFramesPerBurst;
  legacy_config->num_retries_per_rtt_frame = hidl_config.numRetriesPerRttFrame;
  legacy_config->num_retries_per_ftmr = hidl_config.numRetriesPerFtmr;
  legacy_config->LCI_request = hidl_config.mustRequestLci;
  legacy_config->LCR_request = hidl_config.mustRequestLcr;
  legacy_config->burst_duration = hidl_config.burstDuration;
  legacy_config->preamble =
      convertHidlRttPreambleToLegacy(hidl_config.preamble);
  legacy_config->bw = convertHidlRttBwToLegacy(hidl_config.bw);
  return true;
}

bool convertHidlRttLciInformationToLegacy(
    const RttLciInformation& hidl_info,
    legacy_hal::wifi_lci_information* legacy_info) {
  if (!legacy_info) {
    return false;
  }
  legacy_info->latitude = hidl_info.latitude;
  legacy_info->longitude = hidl_info.longitude;
  legacy_info->altitude = hidl_info.altitude;
  legacy_info->latitude_unc = hidl_info.latitudeUnc;
  legacy_info->longitude_unc = hidl_info.longitudeUnc;
  legacy_info->altitude_unc = hidl_info.altitudeUnc;
  legacy_info->motion_pattern =
      convertHidlRttMotionPatternToLegacy(hidl_info.motionPattern);
  legacy_info->floor = hidl_info.floor;
  legacy_info->height_above_floor = hidl_info.heightAboveFloor;
  legacy_info->height_unc = hidl_info.heightUnc;
  return true;
}

bool convertHidlRttLcrInformationToLegacy(
    const RttLcrInformation& hidl_info,
    legacy_hal::wifi_lcr_information* legacy_info) {
  if (!legacy_info) {
    return false;
  }
  CHECK(hidl_info.countryCode.size() == sizeof(legacy_info->country_code));
  memcpy(legacy_info->country_code,
         hidl_info.countryCode.data(),
         hidl_info.countryCode.size());
  if (hidl_info.civicInfo.size() > sizeof(legacy_info->civic_info)) {
    return false;
  }
  legacy_info->length = hidl_info.civicInfo.size();
  memcpy(legacy_info->civic_info,
         hidl_info.civicInfo.c_str(),
         hidl_info.civicInfo.size());
  return true;
}

bool convertHidlRttResponderToLegacy(
    const RttResponder& hidl_responder,
    legacy_hal::wifi_rtt_responder* legacy_responder) {
  if (!legacy_responder) {
    return false;
  }
  if (!convertHidlWifiChannelInfoToLegacy(hidl_responder.channel,
                                          &legacy_responder->channel)) {
    return false;
  }
  legacy_responder->preamble =
      convertHidlRttPreambleToLegacy(hidl_responder.preamble);
  return true;
}

bool convertLegacyRttResponderToHidl(
    const legacy_hal::wifi_rtt_responder& legacy_responder,
    RttResponder* hidl_responder) {
  if (!hidl_responder) {
    return false;
  }
  if (!convertLegacyWifiChannelInfoToHidl(legacy_responder.channel,
                                          &hidl_responder->channel)) {
    return false;
  }
  hidl_responder->preamble =
      convertLegacyRttPreambleToHidl(legacy_responder.preamble);
  return true;
}

bool convertLegacyRttCapabilitiesToHidl(
    const legacy_hal::wifi_rtt_capabilities& legacy_capabilities,
    RttCapabilities* hidl_capabilities) {
  if (!hidl_capabilities) {
    return false;
  }
  hidl_capabilities->rttOneSidedSupported =
      legacy_capabilities.rtt_one_sided_supported;
  hidl_capabilities->rttFtmSupported = legacy_capabilities.rtt_ftm_supported;
  hidl_capabilities->lciSupported = legacy_capabilities.lci_support;
  hidl_capabilities->lcrSupported = legacy_capabilities.lcr_support;
  hidl_capabilities->responderSupported =
      legacy_capabilities.responder_supported;
  for (const auto flag : {legacy_hal::WIFI_RTT_PREAMBLE_LEGACY,
                          legacy_hal::WIFI_RTT_PREAMBLE_HT,
                          legacy_hal::WIFI_RTT_PREAMBLE_VHT}) {
    if (legacy_capabilities.preamble_support & flag) {
      hidl_capabilities->preambleSupport |=
          static_cast<std::underlying_type<RttPreamble>::type>(
              convertLegacyRttPreambleToHidl(flag));
    }
  }
  for (const auto flag : {legacy_hal::WIFI_RTT_BW_5,
                          legacy_hal::WIFI_RTT_BW_10,
                          legacy_hal::WIFI_RTT_BW_20,
                          legacy_hal::WIFI_RTT_BW_40,
                          legacy_hal::WIFI_RTT_BW_80,
                          legacy_hal::WIFI_RTT_BW_160}) {
    if (legacy_capabilities.bw_support & flag) {
      hidl_capabilities->bwSupport |=
          static_cast<std::underlying_type<RttBw>::type>(
              convertLegacyRttBwToHidl(flag));
    }
  }
  hidl_capabilities->mcVersion = legacy_capabilities.mc_version;
  return true;
}

bool convertLegacyWifiRateInfoToHidl(const legacy_hal::wifi_rate& legacy_rate,
                                     WifiRateInfo* hidl_rate) {
  if (!hidl_rate) {
    return false;
  }
  hidl_rate->preamble =
      convertLegacyWifiRatePreambleToHidl(legacy_rate.preamble);
  hidl_rate->nss = convertLegacyWifiRateNssToHidl(legacy_rate.nss);
  hidl_rate->bw = convertLegacyWifiChannelWidthToHidl(
      static_cast<legacy_hal::wifi_channel_width>(legacy_rate.bw));
  hidl_rate->rateMcsIdx = legacy_rate.rateMcsIdx;
  hidl_rate->bitRateInKbps = legacy_rate.bitrate;
  return true;
}

bool convertLegacyRttResultToHidl(
    const legacy_hal::wifi_rtt_result& legacy_result, RttResult* hidl_result) {
  if (!hidl_result) {
    return false;
  }
  CHECK(sizeof(legacy_result.addr) == hidl_result->addr.size());
  memcpy(
      hidl_result->addr.data(), legacy_result.addr, sizeof(legacy_result.addr));
  hidl_result->burstNum = legacy_result.burst_num;
  hidl_result->measurementNumber = legacy_result.measurement_number;
  hidl_result->successNumber = legacy_result.success_number;
  hidl_result->numberPerBurstPeer = legacy_result.number_per_burst_peer;
  hidl_result->status = convertLegacyRttStatusToHidl(legacy_result.status);
  hidl_result->retryAfterDuration = legacy_result.retry_after_duration;
  hidl_result->type = convertLegacyRttTypeToHidl(legacy_result.type);
  hidl_result->rssi = legacy_result.rssi;
  hidl_result->rssiSpread = legacy_result.rssi_spread;
  if (!convertLegacyWifiRateInfoToHidl(legacy_result.tx_rate,
                                       &hidl_result->txRate)) {
    return false;
  }
  if (!convertLegacyWifiRateInfoToHidl(legacy_result.rx_rate,
                                       &hidl_result->rxRate)) {
    return false;
  }
  hidl_result->rtt = legacy_result.rtt;
  hidl_result->rttSd = legacy_result.rtt_sd;
  hidl_result->rttSpread = legacy_result.rtt_spread;
  hidl_result->distanceInMm = legacy_result.distance_mm;
  hidl_result->distanceSdInMm = legacy_result.distance_sd_mm;
  hidl_result->distanceSpreadInMm = legacy_result.distance_spread_mm;
  hidl_result->timeStampInUs = legacy_result.ts;
  hidl_result->burstDurationInMs = legacy_result.burst_duration;
  hidl_result->negotiatedBurstNum = legacy_result.negotiated_burst_num;
  if (!convertLegacyIeToHidl(*legacy_result.LCI, &hidl_result->lci)) {
    return false;
  }
  if (!convertLegacyIeToHidl(*legacy_result.LCR, &hidl_result->lcr)) {
    return false;
  }
  return true;
}
}  // namespace hidl_struct_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
