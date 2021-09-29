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
 * Constraints for changing vsync period.
 */
@VintfStability
parcelable VsyncPeriodChangeConstraints {
    /**
     * Time in CLOCK_MONOTONIC after which the vsync period may change
     * (i.e., the vsync period must not change before this time).
     */
    long desiredTimeNanos;
    /**
     * If true, requires that the vsync period change must happen seamlessly without
     * a noticeable visual artifact.
     */
    boolean seamlessRequired;
}
