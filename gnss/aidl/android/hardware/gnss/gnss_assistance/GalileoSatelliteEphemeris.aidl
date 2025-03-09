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
 * Contains ephemeris parameters specific to Galileo satellites.
 *
 * @hide
 */
@VintfStability
parcelable GalileoSatelliteEphemeris {
    /**
     * Contains the set of parameters needed for Galileo satellite clock correction.
     * This is defined in Galileo-OS-SIS-ICD 5.1.3.
     */
    @VintfStability
    parcelable GalileoSatelliteClockModel {
        /*
         * States the type of satellite clock.
         */
        @VintfStability
        @Backing(type="int")
        enum SatelliteClockType {
            UNDEFINED = 0,
            GALILEO_FNAV_CLOCK = 1,
            GALILEO_INAV_CLOCK = 2
        }

        /**
         * Time of the clock in seconds since Galileo epoch.
         *
         * Represents the 'Epoch' field within the 'SV/EPOCH/SV CLK' record of GNSS
         * navigation message file in RINEX 3.05 Table A8 (Galileo).
         */
        long timeOfClockSeconds;

        /** SV clock bias correction coefficient in seconds. */
        double af0;

        /** SV clock drift correction coefficient in seconds per second. */
        double af1;

        /** SV clock drift rate correction coefficient in seconds per second squared. */
        double af2;

        /**
         * Broadcast group delay in seconds.
         * This is defined in Galileo-OS-SIS-ICD 5.1.5.
         */
        double bgdSeconds;

        /**
         * Signal in space accuracy in meters.
         * This is defined in Galileo-OS-SIS-ICD 5.1.12.
         */
        double sisaMeters;

        /** Type of satellite clock .*/
        SatelliteClockType satelliteClockType;
    }

    /**
     * Contains satellite health.
     * This is defined in Galileo-OS-SIS-ICD 5.1.9.3.
     */
    @VintfStability
    parcelable GalileoSvHealth {
        /** E1-B data validity status. */
        int dataValidityStatusE1b;

        /** E1-B/C signal health status. */
        int signalHealthStatusE1b;

        /** E5a data validity status. */
        int dataValidityStatusE5a;

        /** E5a signal health status. */
        int signalHealthStatusE5a;

        /** E5b data validity status. */
        int dataValidityStatusE5b;

        /** E5b signal health status. */
        int signalHealthStatusE5b;
    }

    /** Satellite code number. */
    int satelliteCodeNumber;

    /** Array of satellite clock model. */
    GalileoSatelliteClockModel[] satelliteClockModel;

    /** Satellite orbit model. */
    KeplerianOrbitModel satelliteOrbitModel;

    /** Satellite health. */
    GalileoSvHealth svHealth;

    /** Satellite ephemeris time. */
    SatelliteEphemerisTime satelliteEphemerisTime;
}
