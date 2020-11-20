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

import android.hardware.gnss.GnssSignalType;

/**
 * Represents an estimate of the GNSS clock time.
 */
@VintfStability
parcelable GnssClock {
    /** Bit mask indicating a valid 'leap second' is stored in the GnssClock. */
    const int HAS_LEAP_SECOND        = 1 << 0;
    /** Bit mask indicating a valid 'time uncertainty' is stored in the GnssClock. */
    const int HAS_TIME_UNCERTAINTY   = 1 << 1;
    /** Bit mask indicating a valid 'full bias' is stored in the GnssClock. */
    const int HAS_FULL_BIAS          = 1 << 2;
    /** Bit mask indicating a valid 'bias' is stored in the GnssClock. */
    const int HAS_BIAS               = 1 << 3;
    /** Bit mask indicating a valid 'bias uncertainty' is stored in the GnssClock. */
    const int HAS_BIAS_UNCERTAINTY   = 1 << 4;
    /** Bit mask indicating a valid 'drift' is stored in the GnssClock. */
    const int HAS_DRIFT              = 1 << 5;
    /** Bit mask indicating a valid 'drift uncertainty' is stored in the GnssClock. */
    const int HAS_DRIFT_UNCERTAINTY  = 1 << 6;

    /**
     * A bitfield of flags indicating the validity of the fields in this data
     * structure.
     *
     * The bit masks are the constants with perfix HAS_.
     *
     * Fields for which there is no corresponding flag must be filled in
     * with a valid value.  For convenience, these are marked as mandatory.
     *
     * Others fields may have invalid information in them, if not marked as
     * valid by the corresponding bit in gnssClockFlags.
     */
    int gnssClockFlags;

    /**
     * Leap second data.
     * The sign of the value is defined by the following equation:
     *      utcTimeNs = timeNs - (fullBiasNs + biasNs) - leapSecond *
     *      1,000,000,000
     *
     * If this data is available, gnssClockFlags must contain
     * HAS_LEAP_SECOND.
     */
    int leapSecond;

    /**
     * The GNSS receiver internal clock value. This is the local hardware clock
     * value.
     *
     * For local hardware clock, this value is expected to be monotonically
     * increasing while the hardware clock remains powered on. (For the case of a
     * HW clock that is not continuously on, see the
     * hwClockDiscontinuityCount field). The receiver's estimate of GNSS time
     * can be derived by subtracting the sum of fullBiasNs and biasNs (when
     * available) from this value.
     *
     * This GNSS time must be the best estimate of current GNSS time
     * that GNSS receiver can achieve.
     *
     * Sub-nanosecond accuracy can be provided by means of the 'biasNs' field.
     * The value contains the timeUncertaintyNs in it.
     *
     * This value is mandatory.
     */
    long timeNs;

    /**
     * 1-Sigma uncertainty associated with the clock's time in nanoseconds.
     * The uncertainty is represented as an absolute (single sided) value.
     *
     * If the data is available, gnssClockFlags must contain
     * HAS_TIME_UNCERTAINTY. Ths value is ideally zero, as the time
     * 'latched' by timeNs is defined as the reference clock vs. which all
     * other times (and corresponding uncertainties) are measured.
     */
    double timeUncertaintyNs;

    /**
     * The difference between hardware clock ('time' field) inside GNSS receiver
     * and the true GPS time since 0000Z, January 6, 1980, in nanoseconds.
     *
     * The sign of the value is defined by the following equation:
     *      local estimate of GPS time = timeNs - (fullBiasNs + biasNs)
     *
     * If receiver has computed time for a non-GPS constellation, the time offset of
     * that constellation versus GPS time must be applied to fill this value.
     *
     * The error estimate for the sum of this and the biasNs is the biasUncertaintyNs.
     *
     * If the data is available gnssClockFlags must contain HAS_FULL_BIAS.
     *
     * This value is mandatory if the receiver has estimated GPS time.
     */
    long fullBiasNs;

    /**
     * Sub-nanosecond bias - used with fullBiasNS, see fullBiasNs for details.
     *
     * The error estimate for the sum of this and the fullBiasNs is the
     * biasUncertaintyNs.
     *
     * If the data is available gnssClockFlags must contain HAS_BIAS.
     *
     * This value is mandatory if the receiver has estimated GPS time.
     */
    double biasNs;

    /**
     * 1-Sigma uncertainty associated with the local estimate of GNSS time (clock
     * bias) in nanoseconds. The uncertainty is represented as an absolute
     * (single sided) value.
     *
     * The caller is responsible for using this uncertainty (it can be very
     * large before the GPS time has been fully resolved.)
     *
     * If the data is available gnssClockFlags must contain HAS_BIAS_UNCERTAINTY.
     *
     * This value is mandatory if the receiver has estimated GPS time.
     */
    double biasUncertaintyNs;

    /**
     * The clock's drift in nanoseconds (per second).
     *
     * A positive value means that the frequency is higher than the nominal
     * frequency, and that the (fullBiasNs + biasNs) is growing more positive
     * over time.
     *
     * If the data is available gnssClockFlags must contain HAS_DRIFT.
     *
     * This value is mandatory if the receiver has estimated GPS time.
     */
    double driftNsps;

    /**
     * 1-Sigma uncertainty associated with the clock's drift in nanoseconds (per
     * second).
     * The uncertainty is represented as an absolute (single sided) value.
     *
     * If the data is available gnssClockFlags must contain HAS_DRIFT_UNCERTAINTY.
     *
     * This value is mandatory if the receiver has estimated GPS time.
     */
    double driftUncertaintyNsps;

    /**
     * This field must be incremented, when there are discontinuities in the
     * hardware clock.
     *
     * A "discontinuity" is meant to cover the case of a switch from one source
     * of clock to another.  A single free-running crystal oscillator (XO)
     * will generally not have any discontinuities, and this can be set and
     * left at 0.
     *
     * If, however, the timeNs value (HW clock) is derived from a composite of
     * sources, that is not as smooth as a typical XO, or is otherwise stopped &
     * restarted, then this value shall be incremented each time a discontinuity
     * occurs.  (E.g. this value can start at zero at device boot-up and
     * increment each time there is a change in clock continuity. In the
     * unlikely event that this value reaches full scale, rollover (not
     * clamping) is required, such that this value continues to change, during
     * subsequent discontinuity events.)
     *
     * While this number stays the same, between GnssClock reports, it can be
     * safely assumed that the timeNs value has been running continuously, e.g.
     * derived from a single, high quality clock (XO like, or better, that is
     * typically used during continuous GNSS signal sampling.)
     *
     * It is expected, esp. during periods where there are few GNSS signals
     * available, that the HW clock be discontinuity-free as long as possible,
     * as this avoids the need to use (waste) a GNSS measurement to fully
     * re-solve for the GNSS clock bias and drift, when using the accompanying
     * measurements, from consecutive GnssData reports.
     *
     * This value is mandatory.
     */
    int hwClockDiscontinuityCount;

    /**
     * Reference GNSS signal type for inter-signal bias.
     */
    GnssSignalType referenceSignalTypeForIsb;
}