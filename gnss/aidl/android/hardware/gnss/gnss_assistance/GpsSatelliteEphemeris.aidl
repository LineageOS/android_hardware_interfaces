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

import android.hardware.gnss.gnss_assistance.KeplerianOrbitModel;
import android.hardware.gnss.gnss_assistance.SatelliteEphemerisTime;

/**
 * Contains ephemeris parameters specific to GPS satellites.
 * This is defined in IS-GPS-200, section 20.3.3.3.
 *
 * @hide
 */
@VintfStability
parcelable GpsSatelliteEphemeris {
    /** Satellite PRN */
    int prn;

    /* Contains information about L2 params. */
    @VintfStability
    parcelable GpsL2Params {
        /** Code(s) on L2 Channel. */
        int l2Code;

        /** Data Flag for L2 P-Code. */
        int l2Flag;
    }

    /** L2 parameters. */
    GpsL2Params gpsL2Params;

    /** Contains the set of parameters needed for GPS satellite clock correction. */
    @VintfStability
    parcelable GpsSatelliteClockModel {
        /**
         * Time of the clock in seconds since GPS epoch.
         *
         * Represents the 'Epoch' field within the 'SV/EPOCH/SV CLK' record of GNSS
         * navigation message file in RINEX 3.05 Table A6.
         */
        long timeOfClockSeconds;

        /** SV clock bias in seconds. */
        double af0;

        /** SV clock drift in seconds per second. */
        double af1;

        /** Clock drift rate in seconds per second squared. */
        double af2;

        /** Group delay differential in seconds. */
        double tgd;

        /** Issue of data, clock. */
        int iodc;
    }

    /** Clock model. */
    GpsSatelliteClockModel satelliteClockModel;

    /** Orbit model. */
    KeplerianOrbitModel satelliteOrbitModel;

    /**
     * Contains information about GPS health. The information is tied to
     * Legacy Navigation (LNAV) data, not Civil Navigation (CNAV) data.
     */
    @VintfStability
    parcelable GpsSatelliteHealth {
        /**
         * Represents "SV health" in the "BROADCAST ORBIT - 6"
         * record of RINEX 3.05. Table A6, pp.68.
         */
        int svHealth;

        /**
         * Represents "SV accuracy" in meters in the "BROADCAST ORBIT - 6"
         * record of RINEX 3.05. Table A6, pp.68.
         */
        double svAccur;

        /**
         * Represents the "Fit Interval" in hours in the "BROADCAST ORBIT - 7"
         * record of RINEX 3.05. Table A6, pp.69.
         */
        double fitInt;
    }

    /** Satellite health. */
    GpsSatelliteHealth satelliteHealth;

    /** Ephemeris time. */
    SatelliteEphemerisTime satelliteEphemerisTime;
}
