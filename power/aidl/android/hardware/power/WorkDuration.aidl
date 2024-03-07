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

package android.hardware.power;

@VintfStability
parcelable WorkDuration {
    /**
     * Time stamp in nanoseconds based on CLOCK_MONOTONIC when the duration
     * sample was measured.
     */
    long timeStampNanos;

    /**
     * Total work duration in nanoseconds.
     */
    long durationNanos;

    /**
     * Timestamp in nanoseconds based on CLOCK_MONOTONIC when the work starts.
     * The work period start timestamp could be zero if the call is from
     * the legacy SDK/NDK reportActualWorkDuration API.
     */
    long workPeriodStartTimestampNanos;

    /**
     * CPU work duration in nanoseconds.
     * The CPU work duration could be the same as the total work duration if
     * the call is from the legacy SDK/NDK reportActualWorkDuration API.
     */
    long cpuDurationNanos;

    /**
     * GPU work duration in nanoseconds.
     * The GPU work duration could be zero if the call is from the legacy
     * SDK/NDK reportActualWorkDuration API.
     */
    long gpuDurationNanos;
}
