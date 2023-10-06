/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

/**
 * Frontend Status Type.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendStatusType {
    /**
     * Lock status for Demod.
     */
    DEMOD_LOCK,

    /**
     * Signal to Noise Ratio.
     */
    SNR,

    /**
     * Bit Error Ratio.
     */
    BER,

    /**
     * Packages Error Ratio.
     */
    PER,

    /**
     * Bit Error Ratio before FEC.
     */
    PRE_BER,

    /**
     * Signal Quality (0..100). Good data over total data in percent can be
     * used as a way to present Signal Quality.
     */
    SIGNAL_QUALITY,

    /**
     * Signal Strength.
     */
    SIGNAL_STRENGTH,

    /**
     * Symbol Rate.
     */
    SYMBOL_RATE,

    /**
     * Forward Error Correction Type.
     */
    FEC,

    /**
     * Modulation Type.
     */
    MODULATION,

    /**
     * Spectral Inversion Type.
     */
    SPECTRAL,

    /**
     * LNB Voltage.
     */
    LNB_VOLTAGE,

    /**
     * Physical Layer Pipe ID.
     */
    PLP_ID,

    /**
     * Status for Emergency Warning Broadcasting System.
     */
    EWBS,

    /**
     * Automatic Gain Control.
     */
    AGC,

    /**
     * Low Noise Amplifier.
     */
    LNA,

    /**
     * Error status by layer.
     */
    LAYER_ERROR,

    /**
     * Moduration Error Ratio.
     */
    MER,

    /**
     * Difference between tuning frequency and actual locked frequency.
     */
    FREQ_OFFSET,

    /**
     * Hierarchy for DVBT.
     */
    HIERARCHY,

    /**
     * Lock status for RF.
     */
    RF_LOCK,

    /**
     * Current tuned PLP information in a frequency band for ATSC3 frontend.
     */
    ATSC3_PLP_INFO,

    /**
     * Modulation Types.
     */
    MODULATIONS,

    /**
     * Bit Error Ratios.
     */
    BERS,
    /**
     * Code Rates.
     */
    CODERATES,

    /**
     * Extended Bandwidth.
     */
    BANDWIDTH,

    /**
     * Extended Guard Intervals.
     */
    GUARD_INTERVAL,

    /**
     * Extended Transmission Mode.
     */
    TRANSMISSION_MODE,

    /**
     * Uncorrectable Error Counts of the frontend's Physical Layer Pipe (PLP)
     * since the last tune operation.
     */
    UEC,

    /**
     * DVB-T2 System Id.
     */
    T2_SYSTEM_ID,

    /**
     * Frontend Interleaving Modes.
     */
    INTERLEAVINGS,

    /**
     * Segments in ISDB-T Specification of all the channels.
     */
    ISDBT_SEGMENTS,

    /**
     * Transport Stream Data Rate in BPS of the current channel.
     */
    TS_DATA_RATES,

    /**
     * Roll Off Type status of the frontend.
     */
    ROLL_OFF,

    /**
     * If the frontend currently supports MISO or not.
     */
    IS_MISO,

    /**
     * If the frontend code rate is linear or not.
     */
    IS_LINEAR,

    /**
     * If short frames is enabled or not.
     */
    IS_SHORT_FRAMES,

    /**
     * ISDB-T Mode.
     */
    ISDBT_MODE,

    /**
     * ISDB-T Partial Reception Flag.
     */
    ISDBT_PARTIAL_RECEPTION_FLAG,

    /**
     * Stream ID list included in a transponder.
     */
    STREAM_ID_LIST,

    /**
     * DVB-T Cell Id.
     */
    DVBT_CELL_IDS,

    /**
     * All PLP information in a frequency band for ATSC3 frontend, which includes both tuned
     * and not tuned PLPs for currently watching service.
     */
    ATSC3_ALL_PLP_INFO,

    /**
     * IPTV Content URL.
     */
    IPTV_CONTENT_URL,

    /**
     * Number of packets lost.
     */
    IPTV_PACKETS_LOST,

    /**
     * Number of packets received.
     */
    IPTV_PACKETS_RECEIVED,

    /**
     * Worst jitter (milliseconds).
     */
    IPTV_WORST_JITTER_MS,

    /**
     * Average jitter (milliseconds).
     */
    IPTV_AVERAGE_JITTER_MS,
}
