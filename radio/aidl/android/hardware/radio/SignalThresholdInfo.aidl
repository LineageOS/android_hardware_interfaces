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

package android.hardware.radio;

import android.hardware.radio.SignalMeasurementType;

/**
 * Contains the threshold values of each signal measurement type.
 */
@VintfStability
parcelable SignalThresholdInfo {
    /**
     * Signal Measurement Type
     */
    SignalMeasurementType signalMeasurement;
    /**
     * A hysteresis time in milliseconds to prevent flapping. A value of 0 disables hysteresis.
     */
    int hysteresisMs;
    /**
     * An interval in dB defining the required magnitude change between reports. This must be
     * smaller than the smallest threshold delta. An interval value of 0 disables hysteresis.
     */
    int hysteresisDb;
    /**
     * List of threshold values. Range and unit must reference specific SignalMeasurementType.
     * The threshold values for which to apply criteria. A vector size of 0 disables the use of
     * thresholds for reporting.
     */
    int[] thresholds;
    /**
     * Indicates whether the reporting criteria of the corresponding measurement is enabled
     * (true) or disabled (false). If enabled, modem must trigger the report based on the criteria.
     * If disabled, modem must not trigger the report based on the criteria.
     */
    boolean isEnabled;
}
