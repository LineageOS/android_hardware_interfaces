/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.gnss;

import android.hardware.gnss.ElapsedRealtime;
import android.hardware.gnss.GnssClock;
import android.hardware.gnss.GnssConstellationType;
import android.hardware.gnss.GnssMeasurement;

/**
 * Represents a reading of GNSS measurements. For devices launched in Android Q or newer, it is
 * mandatory that these be provided, on request, when the GNSS receiver is searching/tracking
 * signals.
 *
 * - Reporting of GNSS constellation measurements is mandatory.
 * - Reporting of all tracked constellations are encouraged.
 *
 * @hide
 */
@VintfStability
parcelable GnssData {
    /** The array of measurements. */
    GnssMeasurement[] measurements;

    /** The GNSS clock time reading. */
    GnssClock clock;

    /**
     * Timing information of the GNSS data synchronized with SystemClock.elapsedRealtimeNanos()
     * clock.
     */
    ElapsedRealtime elapsedRealtime;

    /**
     * Represents a reading of GNSS AGC value of a constellation type and a frequency band.
     */
    @VintfStability
    parcelable GnssAgc {
        /**
         * Automatic gain control (AGC) level. AGC acts as a variable gain amplifier adjusting the
         * power of the incoming signal. The AGC level may be used to indicate potential
         * interference. Higher gain (and/or lower input power) must be output as a positive number.
         * Hence in cases of strong jamming, in the band of this signal, this value must go more
         * negative. This value must be consistent given the same level of the incoming signal
         * power.
         *
         * Note: Different hardware designs (e.g. antenna, pre-amplification, or other RF HW
         * components) may also affect the typical output of this value on any given hardware design
         * in an open sky test - the important aspect of this output is that changes in this value
         * are indicative of changes on input signal power in the frequency band for this
         * measurement.
         */
        double agcLevelDb;

        /**
         * Constellation type of the SV that transmits the signal.
         */
        GnssConstellationType constellation = GnssConstellationType.UNKNOWN;

        /**
         * Carrier frequency of the signal tracked, for example it can be the
         * GPS central frequency for L1 = 1575.45 MHz, or L2 = 1227.60 MHz, L5 =
         * 1176.45 MHz, varying GLO channels, etc. If the field is not set, it
         * is the primary common use central frequency, e.g. L1 = 1575.45 MHz
         * for GPS.
         *
         * If all the GLO frequencies have a common AGC, the FC0 (frequency
         * channel number 0) of the individual GLO bands is used to represent
         * all the GLO frequencies.
         *
         * For an L1, L5 receiver tracking a satellite on L1 and L5 at the same
         * time, two raw measurement structs must be reported for this same
         * satellite, in one of the measurement structs, all the values related
         * to L1 must be filled, and in the other all of the values related to
         * L5 must be filled.
         */
        long carrierFrequencyHz;
    }

    /**
     * The array of GNSS AGC values.
     *
     * This field must be reported when the GNSS measurement engine is running, even when the
     * GnssMeasurement or GnssClock fields are not reported yet. E.g., when a GNSS signal is too
     * weak to be acquired, the AGC value must still be reported.
     */
    GnssAgc[] gnssAgcs = {};

    /**
     * True indicates that the GNSS chipset switches off duty cycling. In such mode, no clock
     * discontinuities are expected and, when supported, carrier phase should be continuous in good
     * signal conditions. All non-blocklisted, healthy constellations, satellites and frequency
     * bands that are meaningful to positioning accuracy must be tracked and reported in this mode.
     *
     * False indicates that the GNSS chipset optimizes power via duty cycling, constellations and
     * frequency limits, etc.
     */
    boolean isFullTracking;
}
