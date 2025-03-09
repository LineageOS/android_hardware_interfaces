/*
 * Copyright 2024 The Android Open Source Project
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

package android.hardware.bluetooth.ranging;

import android.hardware.bluetooth.ranging.PctIQSample;

/**
 * Mode 2 data for a CS step of Channel Sounding.
 * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.44 for details.
 *
 * Specification: https://www.bluetooth.com/specifications/specs/core60-html/
 */
@VintfStability
parcelable ModeTwoData {
    /**
     * Antenna Permutation Index for the chosen Num_Antenna_Paths parameter used during the
     * phase measurement stage of the CS step
     */
    byte antennaPermutationIndex;
    /**
     * The I and Q sample of Phase Correction Term for (Num_Antenna_Paths + 1) CS tone
     * The order is the same as the BT core defined
     */
    PctIQSample[] tonePctIQSamples;

    const int TONE_QUALITY_HIGH = 0x0;
    const int TONE_QUALITY_MEDIUM = 0x1;
    const int TONE_QUALITY_LOW = 0x2;
    const int TONE_QUALITY_UNAVAILABLE = 0x3;
    const int EXTENSION_SLOT_NONE = 0x0;
    const int EXTENSION_SLOT_TONE_NOT_EXPECTED_TO_BE_PRESENT = 0x1;
    const int EXTENSION_SLOT_TONE_EXPECTED_TO_BE_PRESENT = 0x2;
    /**
     * Shift amount for extension slot (bits 4 to 7).
     */
    const int EXTENSION_SLOT_SHIFT = 4;
    /**
     * Tone quality indicator for (Num_Antenna_Paths + 1) CS tone
     * bits 0 to 3:
     * ** 0x0 = Tone quality is high
     * ** 0x1 = Tone quality is medium
     * ** 0x2 = Tone quality is low
     * ** 0x3 = Tone quality is unavailable
     * bits 4 to 7:
     * ** 0x0 = Not tone extension slot
     * ** 0x1 = Tone extension slot; tone not expected to be present
     * ** 0x2 = Tone extension slot; tone expected to be present
     */
    byte[] toneQualityIndicators;
}
