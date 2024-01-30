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
parcelable NrSignalStrength {
    /**
     * SS reference signal received power, multiplied by -1.
     * Reference: 3GPP TS 38.215.
     * Range [44, 140], INT_MAX means invalid/unreported.
     */
    int ssRsrp;
    /**
     * SS reference signal received quality, multiplied by -1.
     * Reference: 3GPP TS 38.215, 3GPP TS 38.133 section 10.
     * Range [-20 dB, 43 dB], INT_MAX means invalid/unreported.
     */
    int ssRsrq;
    /**
     * SS signal-to-noise and interference ratio.
     * Reference: 3GPP TS 38.215 section 5.1.*, 3GPP TS 38.133 section 10.1.16.1.
     * Range [-23, 40], INT_MAX means invalid/unreported.
     */
    int ssSinr;
    /**
     * CSI reference signal received power, multiplied by -1.
     * Reference: 3GPP TS 38.215.
     * Range [44, 140], INT_MAX means invalid/unreported.
     */
    int csiRsrp;
    /**
     * CSI reference signal received quality, multiplied by -1.
     * Reference: 3GPP TS 38.215.
     * Range [3, 20], INT_MAX means invalid/unreported.
     */
    int csiRsrq;
    /**
     * CSI signal-to-noise and interference ratio.
     * Reference: 3GPP TS 138.215 section 5.1.*, 3GPP TS 38.133 section 10.1.16.1.
     * Range [-23, 40], INT_MAX means invalid/unreported.
     */
    int csiSinr;
    /**
     * CSI channel quality indicator (CQI) table index. There are multiple CQI tables.
     * The definition of CQI in each table is different.
     * Reference: 3GPP TS 138.214 section 5.2.2.1.
     * Range [1, 3], INT_MAX means invalid/unreported.
     */
    int csiCqiTableIndex;
    /**
     * CSI channel quality indicator (CQI) for all subbands. If the CQI report is for the entire
     * wideband, a single CQI index is provided. If the CQI report is for all subbands, one CQI
     * index is provided for each subband, in ascending order of subband index. If CQI is not
     * available, the CQI report is empty.
     * Reference: 3GPP TS 138.214 section 5.2.2.1.
     * Range [0, 15], 0xFF means invalid/unreported.
     */
    byte[] csiCqiReport;
    /**
     * Timing advance in micro seconds for a one way trip from cell to device. Approximate distance
     * is calculated using 300m/us * timingAdvance. Range: 0 to 1282 inclusive.
     * INT_MAX: 0x7FFFFFFF denotes invalid/unreported value.
     * Reference: 3GPP 36.213 section 4.2.3
     */
    int timingAdvance = 0x7FFFFFFF;
}
