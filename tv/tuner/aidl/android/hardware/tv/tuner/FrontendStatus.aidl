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
import android.hardware.tv.tuner.FrontendModulation;
import android.hardware.tv.tuner.FrontendModulationStatus;
import android.hardware.tv.tuner.FrontendRollOff;
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

    byte plpId;

    boolean isEWBS;

    /**
     * AGC value is normalized from 0 to 255.
     */
    byte agc;

    boolean isLnaOn;

    boolean[] isLayerError;

    /**
     * MER value measured by 0.001 dB
     */
    int mer;

    /**
     * Frequency difference in Hertz.
     */
    int freqOffset;

    FrontendDvbtHierarchy hierarchy;

    boolean isRfLocked;

    /**
     * A list of PLP status for tuned PLPs for ATSC3 frontend.
     */
    FrontendStatusAtsc3PlpInfo[] plpInfo;

    /**
     * Modulation status.
     */
    FrontendModulation[] modulations;

    /**
     * Bit error ratio status.
     */
    int[] bers;

    /**
     * Code rate status.
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
    char systemId;

    /**
     * Frontend Interleaving Modes.
     */
    FrontendInterleaveMode[] interleaving;

    /**
     * Segments in ISDB-T Specification of all the channels.
     */
    byte[] isdbtSegment;

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
}
