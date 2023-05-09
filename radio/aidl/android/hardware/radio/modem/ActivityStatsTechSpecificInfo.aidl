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

package android.hardware.radio.modem;

import android.hardware.radio.AccessNetwork;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable ActivityStatsTechSpecificInfo {
    /** Indicates the frequency range is unknown. */
    const int FREQUENCY_RANGE_UNKNOWN = 0;
    /** Indicates the frequency range is below 1GHz. */
    const int FREQUENCY_RANGE_LOW = 1;
    /** Indicates the frequency range is between 1GHz and 3GHz. */
    const int FREQUENCY_RANGE_MID = 2;
    /** Indicates the frequency range is between 3GHz and 6GHz. */
    const int FREQUENCY_RANGE_HIGH = 3;
    /** Indicates the frequency range is above 6GHz (millimeter wave frequency). */
    const int FREQUENCY_RANGE_MMWAVE = 4;
    /**
     * Radio access technology. Set UNKNOWN if the Activity statistics
     * is RAT independent.
     */
    AccessNetwork rat;
    /**
     * Frequency range. Values are FREQUENCY_RANGE_
     * Set FREQUENCY_RANGE_UNKNOWN if the Activity statistics when frequency range
     * is not applicable.
     */
    int frequencyRange;
    /**
     * Each index represent total time (in ms) during which the transmitter is active/awake for a
     * particular power range as shown below.
     * index 0 = tx_power <= 0dBm
     * index 1 = 0dBm < tx_power <= 5dBm
     * index 2 = 5dBm < tx_power <= 15dBm
     * index 3 = 15dBm < tx_power <= 20dBm
     * index 4 = tx_power > 20dBm
     */
    int[] txmModetimeMs;
    /**
     * Total time (in ms) for which receiver is active/awake and the transmitter is inactive
     */
    int rxModeTimeMs;
}
