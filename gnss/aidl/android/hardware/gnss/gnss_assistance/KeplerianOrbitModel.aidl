/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.gnss.gnss_assistance;

/**
 * Contains Keplerian orbit model parameters for GPS/Galileo/QZSS/Beidou.
 * For GPS, this is defined in IS-GPS-200 Table 20-II.
 * For Galileo, this is defined in Galileo-OS-SIS-ICD-v2.1 5.1.1.
 * For QZSS, this is defined in IS-QZSS-PNT section 4.1.2.
 * For Baidou, this is defined in BDS-SIS-ICD-B1I-3.0 section 5.2.4.12.
 *
 * @hide
 */
@VintfStability
parcelable KeplerianOrbitModel {
    /** Square root of the semi-major axis in square root of meters. */
    double rootA;

    /** Eccentricity. */
    double eccentricity;

    /** Inclination angle at reference time in radians. */
    double i0;

    /** Rate of change of inclination angle in radians per second. */
    double iDot;

    /** Argument of perigee in radians. */
    double omega;

    /** Longitude of ascending node of orbit plane at beginning of week in radians. */
    double omega0;

    /** Rate of right ascension in radians per second. */
    double omegaDot;

    /** Mean anomaly at reference time in radians. */
    double m0;

    /** Mean motion difference from computed value in radians per second. */
    double deltaN;

    /**
     * Contains second-order harmonic perturbations.
     */
    @VintfStability
    parcelable SecondOrderHarmonicPerturbation {
        /** Amplitude of cosine harmonic correction term to angle of inclination in radians. */
        double cic;

        /** Amplitude of sine harmonic correction term to angle of inclination in radians. */
        double cis;

        /** Amplitude of cosine harmonic correction term to the orbit in meters. */
        double crc;

        /** Amplitude of sine harmonic correction term to the orbit in meters. */
        double crs;

        /** Amplitude of cosine harmonic correction term to the argument of latitude in radians. */
        double cuc;

        /** Amplitude of sine harmonic correction term to the argument of latitude in radians. */
        double cus;
    }

    /** Second-order harmonic perturbations. */
    SecondOrderHarmonicPerturbation secondOrderHarmonicPerturbation;
}
