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

package android.hardware.power.stats;

@VintfStability
parcelable EnergyMeasurement {
    /**
     * ID of the Channel associated with this measurement
     */
    int id;
    /**
     * Time of data capture in milliseconds since boot (CLOCK_BOOTTIME clock)
     */
    long timestampMs;
    /**
     * Duration in milliseconds that energy has been accumulated
     */
    long durationMs;
    /**
     * Accumulated energy in microwatt-seconds (uWs)
     */
    long energyUWs;
}
