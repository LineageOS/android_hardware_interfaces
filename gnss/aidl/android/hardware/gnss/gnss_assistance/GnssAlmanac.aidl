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
 * Contains almanac parameters for GPS, QZSS, Galileo, Beidou.
 *
 * For Beidou, this is defined in BDS-SIS-ICD-B1I-3.0 section 5.2.4.15.
 * For GPS, this is defined in IS-GPS-200 section 20.3.3.5.1.2.
 * For QZSS, this is defined in IS-QZSS-PNT section 4.1.2.6.
 * For Galileo, this is defined in Galileo-OS-SIS-ICD-v2.1 section 5.1.10.
 *
 * @hide
 */
@VintfStability
parcelable GnssAlmanac {
    /**
     * Almanac issue date in milliseconds (UTC).
     *
     * This is unused for GPS/QZSS/Baidou.
     */
    long issueDateMs;

    /**
     * Almanac issue of data.
     * This is defined Galileo-OS-SIS-ICD-v2.1 section 5.1.10.
     * This is unused for GPS/QZSS/Baidou.
     */
    int iod;

    /**
     * Almanac reference week number.
     *
     * For GPS and QZSS, this is GPS week number (modulo 1024).
     * For Beidou, this is Baidou week number (modulo 8192).
     * For Galileo, this is modulo 4 representation of the Galileo week number.
     */
    int weekNumber;

    /** Almanac reference time in seconds. */
    int toaSeconds;

    /**
     * Contains almanac parameters for GPS, QZSS, Galileo, Beidou.
     *
     * For Beidou, this is defined in BDS-SIS-ICD-B1I-3.0 section 5.2.4.15.
     * For GPS, this is defined in IS-GPS-200 section 20.3.3.5.1.2.
     * For QZSS, this is defined in IS-QZSS-PNT section 4.1.2.6.
     * For Galileo, this is defined in Galileo-OS-SIS-ICD-v2.1 section 5.1.10.
     */
    @VintfStability
    parcelable GnssSatelliteAlmanac {
        /** The PRN number of the GNSS satellite. */
        int svid;

        /**
         * Satellite health information.
         *
         * For GPS, this is satellite subframe 4 and 5, page 25 6-bit health code as defined in
         * IS-GPS-200 Table 20-VIII expressed in integer form.
         *
         * For QZSS, this is the 5-bit health code as defined in IS-QZSS-PNT, Table 4.1.2-5-2
         * expressed in integer form.
         *
         * For Beidou, this is 1-bit health information. (0=healthy, 1=unhealthy).
         *
         * For Galileo, this is 6-bit health, bit 0 and 1 is for E5a, bit 2 and 3 is for E5b, bit
         * 4 and 5 is for E1b.
         */
        int svHealth;

        /** Eccentricity. */
        double eccentricity;

        /**
         * Inclination in semi-circles.
         *
         * For GPS and Galileo, this is the difference between the inclination angle at reference
         * time and the nominal inclination in semi-circles.
         *
         * For Beidou and QZSS, this is the inclination angle at reference time in semi-circles.
         */
        double inclination;

        /** Argument of perigee in semi-circles. */
        double omega;

        /** Longitude of ascending node of orbital plane at weekly epoch in semi-circles. */
        double omega0;

        /** Rate of right ascension in semi-circles per second. */
        double omegaDot;

        /**
         * Square root of semi-major axis in square root of meters.
         *
         * For Galileo, this is the difference with respect to the square root of the nominal
         * semi-major axis in square root of meters.
         */
        double rootA;

        /** Mean anomaly at reference time in semi-circles. */
        double m0;

        /** Satellite clock time bias correction coefficient in seconds. */
        double af0;

        /** Satellite clock time drift correction coefficient in seconds per second. */
        double af1;
    }

    /** Array of GnssSatelliteAlmanac. */
    GnssSatelliteAlmanac[] satelliteAlmanac;
}
