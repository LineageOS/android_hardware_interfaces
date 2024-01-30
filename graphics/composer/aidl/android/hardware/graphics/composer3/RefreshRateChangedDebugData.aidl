/**
 * Copyright 2023, The Android Open Source Project
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

@VintfStability
parcelable RefreshRateChangedDebugData {
    /**
     * The display for which the debug data is for.
     */
    long display;

    /**
     * The display vsync period in nanoseconds.
     */
    int vsyncPeriodNanos;

    /**
     * The refresh period of the display in nanoseconds.
     * On VRR (Variable Refresh Rate) displays, refreshPeriodNanos can be different from the
     * vsyncPeriodNanos because not every vsync cycle of the display is a refresh cycle.
     * This should be set to the current refresh period.
     * On non-VRR displays this value should be equal to vsyncPeriodNanos
     *
     * @see vsyncPeriodNanos
     */
    int refreshPeriodNanos;
}
