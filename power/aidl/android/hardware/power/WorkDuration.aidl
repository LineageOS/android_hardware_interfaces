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
     * Timestamp in nanoseconds based on CLOCK_MONOTONIC when the duration
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

    /**
     * Timestamp indicating the approximate time when this frame is intended to
     * present by the app, and will be required for all sessions associated with
     * frame producers. This should always be provided if the session is associated
     * with a pipeline, even if it is not using the GRAPHICS_PIPELINE mode.
     *
     * This timestamp is intended to be used for correlating CompositionData timing
     * information with reported WorkDurations from apps. WorkDurations for
     * sessions associated with a frame producers, without a reasonable value set
     * for this field should be discarded.
     *
     * Intended vsync times can be inferred or retrieved from Choreographer callbacks.
     * While this timestamp is not required to be perfectly accurate, it should
     * roughly correspond with an expected vsync time, and should be discarded otherwise.
     */
    long intendedPresentTimestampNanos;
}
