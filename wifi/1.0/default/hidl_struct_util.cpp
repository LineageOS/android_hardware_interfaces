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

uint8_t ConvertHidlReportEventFlagToLegacy(
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
            ConvertHidlReportEventFlagToLegacy(flag);
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
    hidl_ie.id = legacy_ie.id;
    hidl_ie.data =
        std::vector<uint8_t>(legacy_ie.data, legacy_ie.data + legacy_ie.len);
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
  hidl_scan_data->flags = legacy_cached_scan_result.flags;
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
}  // namespace hidl_struct_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
