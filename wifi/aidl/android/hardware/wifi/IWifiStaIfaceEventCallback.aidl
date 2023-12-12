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
import android.hardware.wifi.TwtSession;
import android.hardware.wifi.TwtSessionStats;

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

    @VintfStability
    @Backing(type="byte")
    enum TwtErrorCode {
        /** Unknown failure */
        FAILURE_UNKNOWN,
        /** TWT session is already resumed */
        ALREADY_RESUMED,
        /** TWT session is already suspended */
        ALREADY_SUSPENDED,
        /** Invalid parameters */
        INVALID_PARAMS,
        /** Maximum number of sessions reached */
        MAX_SESSION_REACHED,
        /** Requested operation is not available */
        NOT_AVAILABLE,
        /** Requested operation is not supported */
        NOT_SUPPORTED,
        /** Requested operation is not supported by the peer */
        PEER_NOT_SUPPORTED,
        /** Requested operation is rejected by the peer */
        PEER_REJECTED,
        /** Requested operation is timed out */
        TIMEOUT,
    }

    @VintfStability
    @Backing(type="byte")
    enum TwtTeardownReasonCode {
        /** Unknown reason */
        UNKNOWN,
        /** Teardown requested by the framework */
        LOCALLY_REQUESTED,
        /** Teardown initiated internally by the firmware or driver */
        INTERNALLY_INITIATED,
        /** Teardown initiated by the peer */
        PEER_INITIATED,
    }

    /**
     * Called to indicate a TWT failure. If there is no command associated with this failure cmdId
     * will be 0.
     *
     * @param cmdId Id used to identify the command. The value 0 indicates no associated command.
     * @param error error code.
     */
    void onTwtFailure(in int cmdId, in TwtErrorCode error);

    /**
     * Called when a Target Wake Time session is created. See |IWifiStaIface.twtSessionSetup|.
     *
     * @param cmdId Id used to identify the command.
     * @param twtSession TWT session.
     */
    void onTwtSessionCreate(in int cmdId, in TwtSession twtSession);

    /**
     * Called when a Target Wake Time session is updated. See |IWifiStaIface.twtSessionUpdate|.
     *
     * @param cmdId Id used to identify the command.
     * @param twtSession TWT session.
     */
    void onTwtSessionUpdate(in int cmdId, in TwtSession twtSession);

    /**
     * Called when the Target Wake Time session is torndown.
     * See |IWifiStaIface.twtSessionTeardown|.
     *
     * @param cmdId Id used to identify the command. The value 0 indicates no associated command.
     * @param twtSessionId TWT session id.
     * @param reasonCode reason code for the TWT teardown.
     */
    void onTwtSessionTeardown(
            in int cmdId, in int twtSessionId, in TwtTeardownReasonCode reasonCode);

    /**
     * Called when TWT session stats available. See |IWifiStaIface.twtSessionGetStats|.
     *
     * @param cmdId Id used to identify the command.
     * @param twtSessionId TWT session id.
     * @param twtSessionStats TWT session stats.
     */
    void onTwtSessionStats(in int cmdId, in int twtSessionId, in TwtSessionStats twtSessionStats);

    /**
     * Called when the Target Wake Time session is suspended.
     * See |IWifiStaIface.twtSessionSuspend|.
     *
     * @param cmdId Id used to identify the command. The value 0 indicates no associated command.
     * @param twtSessionId TWT session id.
     */
    void onTwtSessionSuspend(in int cmdId, in int twtSessionId);

    /**
     * Called when the Target Wake Time session is resumed. See |IWifiStaIface.twtSessionResume|.
     *
     * @param cmdId Id used to identify the command. The value 0 indicates no associated command.
     * @param twtSessionId TWT session id.
     */
    void onTwtSessionResume(in int cmdId, in int twtSessionId);
}
