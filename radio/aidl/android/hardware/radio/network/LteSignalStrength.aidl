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

package android.hardware.radio.network;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable LteSignalStrength {
    /**
     * Valid values are (0-31, 99) as defined in TS 27.007 8.5; INT_MAX means invalid/unreported.
     */
    int signalStrength;
    /**
     * The current Reference Signal Receive Power in dBm multiplied by -1. Range: 44 to 140 dBm;
     * INT_MAX: 0x7FFFFFFF denotes invalid/unreported value. Reference: 3GPP TS 36.133 9.1.4
     */
    int rsrp;
    /**
     * The current Reference Signal Receive Quality in dB multiplied by -1. Range: 20 to 3 dB;
     * INT_MAX: 0x7FFFFFFF denotes invalid/unreported value. Reference: 3GPP TS 36.133 9.1.7
     */
    int rsrq;
    /**
     * The current reference signal signal-to-noise ratio in 0.1 dB units.
     * Range: -200 to +300 (-200 = -20.0 dB, +300 = 30dB).
     * INT_MAX: 0x7FFFFFFF denotes invalid/unreported value. Reference: 3GPP TS 36.101 8.1.1
     */
    int rssnr;
    /**
     * The current Channel Quality Indicator. Range: 0 to 15.
     * INT_MAX: 0x7FFFFFFF denotes invalid/unreported value. Reference: 3GPP TS 36.101 9.2, 9.3, A.4
     */
    int cqi;
    /**
     * Timing advance in micro seconds for a one way trip from cell to device. Approximate distance
     * is calculated using 300m/us * timingAdvance. Range: 0 to 1282 inclusive.
     * INT_MAX: 0x7FFFFFFF denotes invalid/unreported value. Reference: 3GPP 36.213 section 4.2.3
     */
    int timingAdvance;
    /**
     * CSI channel quality indicator (CQI) table index. There are multiple CQI tables.
     * The definition of CQI in each table is different.
     * Reference: 3GPP TS 136.213 section 7.2.3.
     * Range [1, 6], INT_MAX means invalid/unreported.
     */
    int cqiTableIndex;
}
