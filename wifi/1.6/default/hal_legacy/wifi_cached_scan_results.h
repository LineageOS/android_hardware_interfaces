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

#ifndef __WIFI_CACHED_SCAN_RESULTS_H__
#define __WIFI_CACHED_SCAN_RESULTS_H__

#include "wifi_hal.h"

#define WIFI_CACHED_SCAN_RESULT_FLAGS_NONE (0)
/* Element ID 61 (HT Operation) is present (see HT 7.3.2) */
#define WIFI_CACHED_SCAN_RESULT_FLAGS_HT_OPS_PRESENT (1 << 0)
/* Element ID 192 (VHT Operation) is present (see VHT 8.4.2)  */
#define WIFI_CACHED_SCAN_RESULT_FLAGS_VHT_OPS_PRESENT (1 << 1)
/* Element ID 255 + Extension 36 (HE Operation) is present
 * (see 802.11ax 9.4.2.1)
 */
#define WIFI_CACHED_SCAN_RESULT_FLAGS_HE_OPS_PRESENT (1 << 2)
/* Element ID 255 + Extension 106 (HE Operation) is present
 * (see 802.11be D1.5 9.4.2.1)
 */
#define WIFI_CACHED_SCAN_RESULT_FLAGS_EHT_OPS_PRESENT (1 << 3)
/* Element ID 127 (Extended Capabilities) is present, and bit 70
 * (Fine Timing Measurement Responder) is set to 1
 * (see IEEE Std 802.11-2016 9.4.2.27)
 */
#define WIFI_CACHED_SCAN_RESULT_FLAGS_IS_FTM_RESPONDER (1 << 4)

/**
 * Provides information about a single access point (AP) detected in a scan.
 */
typedef struct {
    /* Number of milliseconds prior to ts in the enclosing
     * wifi_cached_scan_result_report struct when
     * the probe response or beacon frame that
     * was used to populate this structure was received.
     */
    u32 age_ms;
    /* The Capability Information field */
    u16 capability;
    /* null terminated */
    u8 ssid[33];
    u8 ssid_len;
    u8 bssid[6];
    /* A set of flags from WIFI_CACHED_SCAN_RESULT_FLAGS_* */
    u8 flags;
    s8 rssi;
    wifi_channel_spec chanspec;
} wifi_cached_scan_result;

/*
 * Data structure sent with events of type WifiCachedScanResult.
 */
typedef struct {
    /* time since boot (in microsecond) when the result was retrieved */
    wifi_timestamp ts;
    /* If 0, indicates that all frequencies in current regulation were
     * scanned. Otherwise, indicates the number of frequencies scanned, as
     * specified in scanned_freq_list.
     */
    u16 scanned_freq_num;
    /* Pointer to an array containing scanned_freq_num values comprising the
     * set of frequencies that were scanned. Frequencies are specified as
     * channel center frequencies in MHz. May be NULL if scannedFreqListLen is
     * 0.
     */
    const u32* scanned_freq_list;
    /* The total number of cached results returned. */
    u8 result_cnt;
    /* Pointer to an array containing result_cnt entries. May be NULL if
     * result_cnt is 0.
     */
    const wifi_cached_scan_result* results;
} wifi_cached_scan_report;

/* callback for reporting cached scan report */
typedef struct {
    void (*on_cached_scan_results)(wifi_cached_scan_report* cache_report);
} wifi_cached_scan_result_handler;
#endif
