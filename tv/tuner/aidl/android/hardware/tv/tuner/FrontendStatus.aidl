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

import android.hardware.tv.tuner.FrontendBandwidth;
import android.hardware.tv.tuner.FrontendDvbtHierarchy;
import android.hardware.tv.tuner.FrontendGuardInterval;
import android.hardware.tv.tuner.FrontendInnerFec;
import android.hardware.tv.tuner.FrontendInterleaveMode;
import android.hardware.tv.tuner.FrontendIsdbtMode;
import android.hardware.tv.tuner.FrontendIsdbtPartialReceptionFlag;
import android.hardware.tv.tuner.FrontendModulation;
import android.hardware.tv.tuner.FrontendModulationStatus;
import android.hardware.tv.tuner.FrontendRollOff;
import android.hardware.tv.tuner.FrontendScanAtsc3PlpInfo;
import android.hardware.tv.tuner.FrontendSpectralInversion;
import android.hardware.tv.tuner.FrontendStatusAtsc3PlpInfo;
import android.hardware.tv.tuner.FrontendTransmissionMode;
import android.hardware.tv.tuner.LnbVoltage;

/**
 * The status for Frontend.
 * @hide
 */
@VintfStability
union FrontendStatus {
    /**
     * Lock status for Demod in True/False.
     */
    boolean isDemodLocked;

    /**
     * SNR value measured by 0.001 dB.
     */
    int snr;

    /**
     * The number of error bit per 1 billion bits.
     */
    int ber;

    /**
     * The number of error package per 1 billion packages.
     */
    int per;

    /**
     * The number of error bit per 1 billion bits before FEC.
     */
    int preBer;

    /**
     * Signal Quality in percent.
     */
    int signalQuality;

    /**
     * Signal Strength measured by 0.001 dBm.
     */
    int signalStrength;

    /**
     * Symbols per second
     */
    int symbolRate;

    FrontendInnerFec innerFec;

    FrontendModulationStatus modulationStatus;

    FrontendSpectralInversion inversion;

    LnbVoltage lnbVoltage;

    int plpId;

    boolean isEWBS;

    /**
     * AGC value is normalized from 0 to 255.
     * Larger AGC values indicate it is applying more gain.
     */
    int agc;

    boolean isLnaOn;

    /**
     * Layer Error status.
     *
     * The order of the vectors is in ascending order of the required CNR (Contrast-to-noise ratio).
     * The most robust layer is the first. For example, in ISDB-T, isLayerError[0] is the
     * information of layer A. isLayerError[1] is the information of layer B.
     */
    boolean[] isLayerError;

    /**
     * MER value measured by 0.001 dB
     */
    int mer;

    /**
     * Frequency difference in Hertz.
     */
    long freqOffset;

    FrontendDvbtHierarchy hierarchy;

    boolean isRfLocked;

    /**
     * A list of PLP status for tuned PLPs for ATSC3 frontend.
     */
    FrontendStatusAtsc3PlpInfo[] plpInfo;

    /**
     * Modulation status.
     *
     * The order of the vectors is in ascending order of the required CNR (Contrast-to-noise ratio).
     * The most robust layer is the first. For example, in ISDB-T, modulations[0] is the information
     * of layer A. modulations[1] is the information of layer B.
     */
    FrontendModulation[] modulations;

    /**
     * Bit error ratio status.
     *
     * The order of the vectors is in ascending order of the required CNR (Contrast-to-noise ratio).
     * The most robust layer is the first. For example, in ISDB-T, bers[0] is the information of
     * layer A. bers[1] is the information of layer B.
     */
    int[] bers;

    /**
     * Code rate status.
     *
     * The order of the vectors is in ascending order of the required CN. The most robust layer is
     * the first. For example, in ISDB-T, codeRates[0] is the information of layer A. codeRates[1]
     * is the information of layer B.
     */
    FrontendInnerFec[] codeRates;

    /**
     * Bandwidth status.
     */
    FrontendBandwidth bandwidth;

    /**
     * Guard interval status.
     */
    FrontendGuardInterval interval;

    /**
     * Transmission mode status.
     */
    FrontendTransmissionMode transmissionMode;

    /**
     * Uncorrectable Error Counts of the frontend's Physical Layer Pipe (PLP)
     * since the last tune operation.
     */
    int uec;

    /**
     * The current DVB-T2 system id status.
     */
    int systemId;

    /**
     * Frontend Interleaving Modes.
     *
     * The order of the vectors is in ascending order of the required CNR (Contrast-to-noise ratio).
     * The most robust layer is the first. For example, in ISDB-T, interleaving[0] is the
     * information of layer A. interleaving[1] is the information of layer B.
     */
    FrontendInterleaveMode[] interleaving;

    /**
     * Segments in ISDB-T Specification of all the channels.
     *
     * The order of the vectors is in ascending order of the required CNR (Contrast-to-noise ratio).
     * The most robust layer is the first. For example, in ISDB-T, isdbtSegment[0] is the
     * information of layer A. isdbtSegment[1] is the information of layer B.
     */
    int[] isdbtSegment;

    /**
     * Transport Stream Data Rate in BPS of the current channel.
     */
    int[] tsDataRate;

    /**
     * Roll Off Type status of the frontend.
     */
    FrontendRollOff rollOff;

    /**
     * If the frontend currently supports MISO or not.
     */
    boolean isMiso;

    /**
     * If the frontend code rate is linear or not.
     */
    boolean isLinear;

    /**
     * If short frames is enabled or not.
     */
    boolean isShortFrames;

    /**
     * ISDB-T Mode.
     */
    FrontendIsdbtMode isdbtMode;

    /**
     * ISDB-T Partial Reception Flag.
     */
    FrontendIsdbtPartialReceptionFlag partialReceptionFlag;

    /**
     * Stream ID list included in a transponder.
     */
    int[] streamIdList;

    /**
     * DVB-T Cell Id.
     */
    int[] dvbtCellIds;

    /**
     * A list of all PLPs in the frequency band for ATSC3 frontend, which includes both tuned
     * and not tuned PLPs for currently watching service.
     */
    FrontendScanAtsc3PlpInfo[] allPlpInfo;

    /**
     * IPTV Content URL
     */
    String iptvContentUrl = "";

    /**
     * Packets Received (IPTV - UDP/RTP).
     */
    long iptvPacketsReceived;

    /**
     * Packets Lost (IPTV - RTP).
     */
    long iptvPacketsLost;

    /**
     * Worst jitter (milliseconds).
     */
    int iptvWorstJitterMs;

    /**
     * Average jitter (milliseconds).
     */
    int iptvAverageJitterMs;
}
