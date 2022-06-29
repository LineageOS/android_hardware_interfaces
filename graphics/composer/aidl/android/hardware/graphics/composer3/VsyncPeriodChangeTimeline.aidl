/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.composer3;

/**
 * Timing for a vsync period change.
 */
@VintfStability
parcelable VsyncPeriodChangeTimeline {
    /**
     * The time in CLOCK_MONOTONIC when the new display will start to refresh at
     * the new vsync period.
     */
    long newVsyncAppliedTimeNanos;
    /**
     * Set to true if the client is required to send a frame to be displayed before
     * the change can take place.
     */
    boolean refreshRequired;
    /**
     * The time in CLOCK_MONOTONIC when the client is expected to send the new frame.
     * Should be ignored if refreshRequired is false.
     */
    long refreshTimeNanos;
}
