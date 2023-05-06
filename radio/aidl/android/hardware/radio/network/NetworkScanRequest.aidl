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

package android.hardware.radio.network;

import android.hardware.radio.network.RadioAccessSpecifier;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable NetworkScanRequest {
    const int RADIO_ACCESS_SPECIFIER_MAX_SIZE = 8;

    const int INCREMENTAL_RESULTS_PREIODICITY_RANGE_MIN = 1;
    const int INCREMENTAL_RESULTS_PREIODICITY_RANGE_MAX = 10;

    const int MAX_SEARCH_TIME_RANGE_MIN = 60;
    const int MAX_SEARCH_TIME_RANGE_MAX = 3600;

    const int SCAN_INTERVAL_RANGE_MIN = 5;
    const int SCAN_INTERVAL_RANGE_MAX = 300;

    /**
     * Performs the scan only once
     */
    const int SCAN_TYPE_ONE_SHOT = 0;
    /**
     * Performs the scan periodically until cancelled
     */
    const int SCAN_TYPE_PERIODIC = 1;

    /**
     * Values are SCAN_TYPE_
     */
    int type;
    /**
     * Time interval in seconds between the completion of one scan and the start of a subsequent
     * scan. Implementations may ignore this field unless the 'type' is 'PERIODIC'.
     * Range: SCAN_INTERVAL_RANGE_MIN to SCAN_INTERVAL_RANGE_MAX.
     */
    int interval;
    /**
     * Networks with bands/channels to scan.
     * Maximum length of the vector is RADIO_ACCESS_SPECIFIER_MAX_SIZE.
     */
    RadioAccessSpecifier[] specifiers;
    /**
     * Maximum duration of the periodic search (in seconds). If the search lasts maxSearchTime, it
     * must be terminated. Range: MAX_SEARCH_TIME_RANGE_MIN to MAX_SEARCH_TIME_RANGE_MAX
     */
    int maxSearchTime;
    /**
     * Whether the modem must report incremental results of the network scan to the client.
     * FALSE – Incremental results must not be reported.
     * TRUE  – Incremental must be reported.
     */
    boolean incrementalResults;
    /**
     * Indicates the periodicity with which the modem must report incremental results to the client
     * (in seconds). Implementations may ignore this value if the incremental results are not
     * requested. This value must be less than or equal to maxSearchTime.
     * Range: INCREMENTAL_RESULTS_PREIODICITY_RANGE_MIN to INCREMENTAL_RESULTS_PREIODICITY_RANGE_MAX
     */
    int incrementalResultsPeriodicity;
    /**
     * Describes the List of PLMN ids (MCC-MNC). If any PLMN of this list is found, search must end
     * at that point and results with all PLMN found until that point should be sent as response.
     * If the list is not sent, search to be completed until end and all PLMNs found to be reported.
     */
    String[] mccMncs;
}
