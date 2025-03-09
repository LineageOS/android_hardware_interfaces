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

import android.hardware.bluetooth.ranging.StepData;
import android.hardware.bluetooth.ranging.SubeventAbortReason;

/**
 * The subevent data within a CS procedure of Channel Sounding.
 * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.44 for details.
 *
 * Specification: https://www.bluetooth.com/specifications/specs/core60-html/
 */
@VintfStability
parcelable SubeventResultData {
    /**
     * Starting ACL connection event counter for the results reported in the event
     */
    int startAclConnEventCounter;
    const int FREQ_COMPENSATION_UNAVAILABLE = 0xFFFFC000;
    /**
     * Frequency compensation value in units of 0.01 ppm (15-bit signed integer, it had been
     * converted as int here.)
     * Unit: 0.01 ppm
     * 0xFFFFC000 - Frequency compensation value is not available, or the role is not initiator
     */
    int frequencyCompensation = FREQ_COMPENSATION_UNAVAILABLE;
    /**
     * Reference power level
     * Range: -127 to 20
     * Unit: dBm
     */
    byte referencePowerLevelDbm;
    /**
     * 0x00 Ignored because phase measurement does not occur during the CS step
     * 0x01 to 0x04 Number of antenna paths used during the phase measurement stage of the CS step
     */
    byte numAntennaPaths;
    /**
     * Indicates the abort reason
     */
    SubeventAbortReason subeventAbortReason;
    /**
     * The measured data for all steps
     */
    StepData[] stepData;
    /**
     * Timestamp when all subevent data are received by the host; Not defined by the spec.
     * Using epoch time in nano seconds (e.g., 1697673127175).
     */
    long timestampNanos;
}
