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
 * LE CS Procedure Enable Complete data
 * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.43 for details.
 *
 * Specification: https://www.bluetooth.com/specifications/specs/core60-html/
 */
@VintfStability
parcelable ProcedureEnableConfig {
    /**
     * Antenna Configuration Index as described in [Vol 6] Part A, Section 5.3
     * Value: 0x00 to 0x07
     */
    byte toneAntennaConfigSelection;
    /**
     * Duration for each CS subevent in microseconds
     * Value: 1250 μs to 4 s
     */
    int subeventLenUs;
    /**
     * Number of CS subevents anchored off the same ACL connection event
     * Value: 0x01 to 0x20
     */
    byte subeventsPerEvent;
    /**
     * Time between consecutive CS subevents anchored off the same ACL connection event.
     * Unit: 0.625 ms
     */
    int subeventInterval;
    /**
     * Number of ACL connection events between consecutive CS event anchor points
     */
    int eventInterval;
    /**
     * Number of ACL connection events between consecutive CS procedure anchor points
     */
    int procedureInterval;
    /**
     * Number of CS procedures to be scheduled.
     * Value: 0x0000 to 0xFFFF
     * Value 0: CS procedures to continue until disabled
     */
    int procedureCount;
    /**
     * Maximum duration for each CS procedure
     * Range: 0x0001 to 0xFFFF
     * unit: 0.625 ms
     * Time range: 0.625 ms to 40.959375 s
     */
    int maxProcedureLen;
}
