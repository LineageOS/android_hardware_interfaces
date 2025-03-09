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

/**
 * Mode 0 data for a CS step of Channel Sounding.
 * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.44 for details.
 *
 * Specification: https://www.bluetooth.com/specifications/specs/core60-html/
 */
@VintfStability
parcelable ModeZeroData {
    const byte FLAG_CS_ACCESS_ADDR_SUCCESS = 0x0;
    const byte FLAG_CS_ACCESS_ADDR_ERRORS = 0x1;
    const byte FLAG_CS_ACCESS_ADDR_NOT_FOUND = 0x2;
    const byte FLAG_CS_ACCESS_ADDR_MASK = 0xF; // bit 3 is reserved
    const byte RANDOM_OR_SOUNDING_SEQUENCE_ERROR_COUNT_SHIFT = 4;
    /**
     * bits 0 to 3:
     * ** 0x0 = CS Access Address check is successful, and all bits match the expected sequence
     * ** 0x1 = CS Access Address check contains one or more bit errors
     * ** 0x2 = CS Access Address not found
     * bits 4 to 7: Number of bit errors being reported on the payload with a random or sounding
     * sequence. Value 0 may indicate zero bit errors or no report available.
     * Value 15 may indicate 15 or more bit errors.
     */
    byte packetQuality;
    const byte PACKET_RSSI_UNAVAILABLE = 0x7Fu8;
    /**
     * Range: -127 to +20
     * Unit: dBm
     * Value: 0x7F - RSSI is not available
     */
    byte packetRssiDbm = PACKET_RSSI_UNAVAILABLE;
    /**
     * Antenna identifier used for the RTT packet
     * Value: 0x01 to 0x04
     */
    byte packetAntenna;
    const int MEASURED_FREQ_OFFSET_UNAVAILABLE = 0xFFFFC000;
    /**
     * Measured frequency offset in units of 0.01 ppm (15-bit signed integer, it had been converted
     * as int here.) for initiator, this should be ignored for relector.
     * Range: -100 ppm (0x58F0) to +100 ppm (0x2710)
     * Unit: 0.01 ppm
     * Value: 0xFFFFC000 - Frequency offset is not available
     */
    int initiatorMeasuredFreqOffset = MEASURED_FREQ_OFFSET_UNAVAILABLE;
}
