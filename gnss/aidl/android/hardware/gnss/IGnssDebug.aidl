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

package android.hardware.gnss;

import android.hardware.gnss.GnssConstellationType;
import android.hardware.gnss.SatellitePvt.SatelliteEphemerisSource;

/**
 * Extended interface for GNSS Debug support
 *
 * This information is used for debugging purpose, e.g., shown in a bugreport to
 * describe the chipset states including time, position, and satellite data.
 *
 * @hide
 */
@VintfStability
interface IGnssDebug {
    /** Satellite's ephemeris type */
    @VintfStability
    @Backing(type="int")
    enum SatelliteEphemerisType {
        EPHEMERIS = 0,
        ALMANAC_ONLY = 1,
        NOT_AVAILABLE = 2,
    }

    /** Satellite's ephemeris health */
    @VintfStability
    @Backing(type="int")
    enum SatelliteEphemerisHealth {
        GOOD = 0,
        BAD = 1,
        UNKNOWN = 2,
    }

    /**
     * Provides the current best known UTC time estimate.
     * If no fresh information is available, e.g. after a delete all,
     * then whatever the effective defaults are on the device must be
     * provided (e.g. Jan. 1, 2017, with an uncertainty of 5 years) expressed
     * in the specified units.
     */
    @VintfStability
    parcelable TimeDebug {
        /** UTC time estimate in milliseconds. */
        long timeEstimateMs;

        /** 68% time error estimate in nanoseconds. */
        float timeUncertaintyNs;

        /**
         * 68% error estimate in local clock drift,
         * in nanoseconds per second (also known as parts per billion - ppb.)
         */
        float frequencyUncertaintyNsPerSec;
    }

    @VintfStability
    parcelable PositionDebug {
        /**
         * Validity of the data in this struct. False only if no
         * latitude/longitude information is known.
         */
        boolean valid;

        /** Latitude expressed in degrees */
        double latitudeDegrees;

        /** Longitude expressed in degrees */
        double longitudeDegrees;

        /** Altitude above ellipsoid expressed in meters */
        float altitudeMeters;

        /** Represents horizontal speed in meters per second. */
        float speedMetersPerSec;

        /** Represents heading in degrees. */
        float bearingDegrees;

        /**
         * Estimated horizontal accuracy of position expressed in meters,
         * radial, 68% confidence.
         */
        double horizontalAccuracyMeters;

        /**
         * Estimated vertical accuracy of position expressed in meters, with
         * 68% confidence.
         */
        double verticalAccuracyMeters;

        /**
         * Estimated speed accuracy in meters per second with 68% confidence.
         */
        double speedAccuracyMetersPerSecond;

        /**
         * Estimated bearing accuracy degrees with 68% confidence.
         */
        double bearingAccuracyDegrees;

        /**
         * Time duration before this report that this position information was
         * valid.  This can, for example, be a previous injected location with
         * an age potentially thousands of seconds old, or
         * extrapolated to the current time (with appropriately increased
         * accuracy estimates), with a (near) zero age.
         */
        float ageSeconds;
    }

    @VintfStability
    parcelable SatelliteData {
        /** Satellite vehicle ID number */
        int svid;

        /** Defines the constellation type of the given SV. */
        GnssConstellationType constellation;

        /**
         * Defines the standard broadcast ephemeris or almanac availability for
         * the satellite.  To report status of predicted orbit and clock
         * information, see the serverPrediction fields below.
         */
        SatelliteEphemerisType ephemerisType;

        /** Defines the ephemeris source of the satellite. */
        SatelliteEphemerisSource ephemerisSource;

        /**
         * Defines whether the satellite is known healthy
         * (safe for use in location calculation.)
         */
        SatelliteEphemerisHealth ephemerisHealth;

        /**
         * Time duration from this report (current time), minus the
         * effective time of the ephemeris source (e.g. TOE, TOA.)
         * Set to 0 when ephemerisType is NOT_AVAILABLE.
         */
        float ephemerisAgeSeconds;

        /**
         * True if a server has provided a predicted orbit and clock model for
         * this satellite.
         */
        boolean serverPredictionIsAvailable;

        /**
         * Time duration from this report (current time) minus the time of the
         * start of the server predicted information.  For example, a 1 day
         * old prediction would be reported as 86400 seconds here.
         */
        float serverPredictionAgeSeconds;
    }

    /**
     * Provides a set of debug information that is filled by the GNSS chipset
     * when the method getDebugData() is invoked.
     */
    @VintfStability
    parcelable DebugData {
        /** Current best known position. */
        PositionDebug position;

        /** Current best know time estimate */
        TimeDebug time;

        /**
         * Provides a list of the available satellite data, for all
         * satellites and constellations the device can track,
         * including GnssConstellationType UNKNOWN.
         */
        List<SatelliteData> satelliteDataArray;
    }

    /**
     * This methods requests position, time and satellite ephemeris debug information
     * from the HAL.
     *
     * @return ret debugData information from GNSS Hal that contains the current best
     * known position, best known time estimate and a complete list of
     * constellations that the device can track.
     */
    DebugData getDebugData();
}
