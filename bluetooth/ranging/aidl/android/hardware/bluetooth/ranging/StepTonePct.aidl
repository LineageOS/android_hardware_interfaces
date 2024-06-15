/*
 * Copyright 2023 The Android Open Source Project
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

import android.hardware.bluetooth.ranging.ComplexNumber;

/**
 * Tone PCT data with quality indicator from a mode-2 or mode-3 step.
 */
@VintfStability
parcelable StepTonePct {
    /**
     * PCT measured from mode-2 or mode-3 steps
     * (in ascending order of antenna position with tone extension data at the end).
     */
    List<ComplexNumber> tonePcts;
    const int TONE_QUALITY_GOOD = 0;
    const int TONE_QUALITY_MEDIUM = 1;
    const int TONE_QUALITY_LOW = 2;
    const int TONE_QUALITY_UNAVAILABLE = 3;
    const int EXTENSION_SLOT_NONE = 0;
    const int EXTENSION_SLOT_TONE_NOT_EXPECTED_TO_BE_PRESENT = 1;
    const int EXTENSION_SLOT_TONE_EXPECTED_TO_BE_PRESENT = 2;
    /**
     * Shift amount for extension slot (bits 4 to 7).
     */
    const int EXTENSION_SLOT_SHIFT_AMOUNT = 4;
    /**
     * Tone_Quality_Indicator defined in the LE CS Subevent Result event
     *
     * Bits 0 to 3:
     * 0x0 = Tone quality is good
     * 0x1 = Tone quality is medium
     * 0x2 = Tone quality is low
     * 0x3 = Tone quality is unavailable
     *
     * Bits 4 to 7:
     * 0x0 = Not tone extension slot
     * 0x1 = Tone extension slot; tone not expected to be present
     * 0x2 = Tone extension slot; tone expected to be present
     *
     * See: https://bluetooth.com/specifications/specs/channel-sounding-cr-pr/
     */
    byte[] toneQualityIndicator;

    const byte TONE_EXTENSION_ANTENNA_1 = 0x0;
    const byte TONE_EXTENSION_ANTENNA_2 = 0x1;
    const byte TONE_EXTENSION_ANTENNA_3 = 0x2;
    const byte TONE_EXTENSION_ANTENNA_4 = 0x3;
    const byte TONE_EXTENSION_UNUSED = 0xFFu8;
    /**
     * Tone Extension Antenna Index indicates the Antenna position used in tone extension slot
     *
     * 0x00 = A1
     * 0x01 = A2
     * 0x02 = A3
     * 0x03 = A4
     * 0xFF = Tone extension not used
     */
    byte toneExtensionAntennaIndex;
}
