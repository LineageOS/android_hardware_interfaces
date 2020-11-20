/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.gnss;

/**
 * Cumulative GNSS power statistics since boot.
 */
@VintfStability
parcelable ElapsedRealtime {

    /** Bit mask indicating a valid timestampNs is stored in the ElapsedRealtime parcelable. */
    const int HAS_TIMESTAMP_NS = 1 << 0;

    /**
     * Bit mask indicating a valid timeUncertaintyNs is stored in the ElapsedRealtime parcelable.
     */
    const int HAS_TIME_UNCERTAINTY_NS = 1 << 1;

    /**
     * A bit field of flags indicating the validity of each field in this data structure.
     *
     * The bit masks are the constants with prefix HAS_.
     *
     * Fields may have invalid information in them, if not marked as valid by the corresponding bit
     * in flags.
     */
    int flags;

    /**
     * Estimate of the elapsed time since boot value for the corresponding event in nanoseconds.
     */
    long timestampNs;

    /**
     * Estimate of the relative precision of the alignment of this SystemClock timestamp, with the
     * reported measurements in nanoseconds (68% confidence).
     */
    double timeUncertaintyNs;
}