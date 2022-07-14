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

package android.hardware.wifi;

import android.hardware.wifi.StaScanData;
import android.hardware.wifi.StaScanResult;

@VintfStability
oneway interface IWifiStaIfaceEventCallback {
    /**
     * Called for each received beacon/probe response for a scan with the
     * |REPORT_EVENTS_FULL_RESULTS| flag set in
     * |StaBackgroundScanBucketParameters.eventReportScheme|.
     *
     * @param cmdId Command Id corresponding to the request.
     * @param bucketsScanned Bitset where each bit indicates if the bucket with
     *        that index (starting at 0) was scanned.
     * @param result Full scan result for an AP.
     */
    void onBackgroundFullScanResult(in int cmdId, in int bucketsScanned, in StaScanResult result);

    /**
     * Callback indicating that an ongoing background scan request has failed.
     * The background scan needs to be restarted to continue scanning.
     *
     * @param cmdId Command Id corresponding to the request.
     */
    void onBackgroundScanFailure(in int cmdId);

    /**
     * Called when the |StaBackgroundScanBucketParameters.eventReportScheme| flags
     * for at least one bucket that was just scanned was
     * |REPORT_EVENTS_EACH_SCAN|, or one of the configured thresholds was
     * breached.
     *
     * @param cmdId Command Id corresponding to the request.
     * @param scanDatas List of scan result for all AP's seen since last callback.
     */
    void onBackgroundScanResults(in int cmdId, in StaScanData[] scanDatas);

    /**
     * Called when the RSSI of the currently connected access point goes beyond the
     * thresholds set via |IWifiStaIface.startRssiMonitoring|.
     *
     * @param cmdId Command Id corresponding to the request.
     * @param currBssid BSSID of the currently connected access point.
     * @param currRssi RSSI of the currently connected access point.
     */
    void onRssiThresholdBreached(in int cmdId, in byte[6] currBssid, in int currRssi);
}
