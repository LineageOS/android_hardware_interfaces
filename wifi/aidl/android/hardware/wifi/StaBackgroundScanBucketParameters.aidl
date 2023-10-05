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

import android.hardware.wifi.WifiBand;

/**
 * Background Scan parameters per bucket that can be specified in background
 * scan requests.
 */
@VintfStability
parcelable StaBackgroundScanBucketParameters {
    /**
     * Bucket index. This index is used to report results in
     * |StaScanData.bucketsScanned|.
     */
    int bucketIdx;
    /**
     * Bands to scan or |BAND_UNSPECIFIED| if frequencies list must be used
     * instead.
     */
    WifiBand band;
    /**
     * Channel frequencies (in Mhz) to scan if |band| is set to
     * |BAND_UNSPECIFIED|.
     * Max length: |StaBackgroundScanLimits.MAX_CHANNELS|.
     */
    int[] frequencies;
    /**
     * Period at which this bucket must be scanned (in milliseconds). Must be an integer
     * multiple of the |basePeriodInMs| specified in the BackgroundScanParameters.
     */
    int periodInMs;
    /**
     * Bitset of |StaBackgroundScanBucketEventReportSchemeMask| values controlling
     * when events for this bucket must be reported.
     */
    int eventReportScheme;
    /**
     * For exponential back off. If |exponentialMaxPeriodInMs| is non-zero or
     * different than period, then this bucket is an exponential backoff bucket
     * and the scan period must grow exponentially as per formula:
     *   actual_period(N) = period * (base ^ (N/step_count))
     * to this maximum period (in milliseconds).
     */
    int exponentialMaxPeriodInMs;
    /**
     * For exponential back off. Multiplier: new_period=old_period * base
     */
    int exponentialBase;
    /**
     * For exponential back off. Number of scans to perform for a given
     * period.
     */
    int exponentialStepCount;
}
