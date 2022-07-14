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

/**
 * Mask of event reporting schemes that can be specified in background scan
 * requests.
 */
@VintfStability
@Backing(type="int")
enum StaBackgroundScanBucketEventReportSchemeMask {
    /**
     * Report a scan completion event after scan. If this is not set, then scan
     * completion events must be reported if report_threshold_percent or
     * report_threshold_num_scans is reached.
     */
    EACH_SCAN = 1 << 0,
    /**
     * Forward scan results (beacons/probe responses + IEs) in real time to HAL,
     * in addition to completion events.
     * Note: To keep backward compatibility, fire completion events regardless
     * of REPORT_EVENTS_EACH_SCAN.
     */
    FULL_RESULTS = 1 << 1,
    /**
     * Controls if scans for this bucket must be placed in the results buffer.
     */
    NO_BATCH = 1 << 2,
}
