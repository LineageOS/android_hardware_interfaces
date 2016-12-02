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

// Convert hidl gscan params to legacy gscan params.
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
}  // namespace hidl_struct_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // HIDL_STRUCT_UTIL_H_
