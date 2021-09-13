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

@VintfStability
union RadioFrequencyInfo {
    /**
     * Indicates the frequency range is below 1GHz.
     */
    const int FREQUENCY_RANGE_LOW = 1;
    /**
     * Indicates the frequency range is between 1GHz and 3GHz.
     */
    const int FREQUENCY_RANGE_MID = 2;
    /**
     * Indicates the frequency range is between 3GHz and 6GHz.
     */
    const int FREQUENCY_RANGE_HIGH = 3;
    /**
     * Indicates the frequency range is above 6GHz (millimeter wave frequency).
     */
    const int FREQUENCY_RANGE_MMWAVE = 4;

    boolean noinit;
    /**
     * A rough frequency range.
     * Values are FREQUENCY_RANGE_
     */
    int range;
    /**
     * The Absolute Radio Frequency Channel Number.
     */
    int channelNumber;
}
