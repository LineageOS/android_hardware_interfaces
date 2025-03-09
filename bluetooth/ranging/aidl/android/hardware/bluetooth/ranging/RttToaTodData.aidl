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

/**
 * The ToA/ToD data for the initator or reflector.
 * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.44 for details.
 *
 * Specification: https://www.bluetooth.com/specifications/specs/core60-html/
 */
@VintfStability
union RttToaTodData {
    const int TOA_TOD_UNAVAILABLE = 0xFFFF8000;
    /**
     * Time difference in units of 0.5 nanoseconds between the time of arrival and
     * the time of departure of the CS packets at the initiator during a CS step
     * (16-bit signed integer, it had been converted to 'int' here), where the known
     * nominal offsets are excluded.
     * value: 0xFFFF8000 (0x8000 as signed 16-bit) - Time difference is not available
     */
    int toaTodInitiator = TOA_TOD_UNAVAILABLE;
    /**
     * Time difference in units of 0.5 nanoseconds between the time of departure
     * and the time of arrival of the CS packets at the reflector during a CS step
     * (16-bit signed integer,  it had been converted to 'int' here), where the known
     * nominal offsets are excluded.
     * value: 0xFFFF8000 (0x8000 as signed 16-bit) - Time difference is not available
     */
    int todToaReflector = TOA_TOD_UNAVAILABLE;
}
