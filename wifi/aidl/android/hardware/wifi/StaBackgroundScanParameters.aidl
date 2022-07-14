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

import android.hardware.wifi.StaBackgroundScanBucketParameters;

/**
 * Background Scan parameters that can be specified in background scan
 * requests.
 */
@VintfStability
parcelable StaBackgroundScanParameters {
    /**
     * GCD of all bucket periods (in milliseconds).
     */
    int basePeriodInMs;
    /**
     * Maximum number of APs that must be stored for each scan. If the maximum
     * is reached, then the highest RSSI results must be returned.
     * Max length: |StaBackgroundScanLimits.MAX_AP_CACHE_PER_SCAN|.
     */
    int maxApPerScan;
    /**
     * % cache buffer filled threshold at which the host must be notified of
     * batched scan results.
     */
    int reportThresholdPercent;
    /**
     * Threshold at which the AP must be woken up, in number of scans.
     */
    int reportThresholdNumScans;
    /**
     * List of buckets to be scheduled.
     * Max length: |StaBackgroundScanLimits.MAX_BUCKETS|.
     */
    StaBackgroundScanBucketParameters[] buckets;
}
