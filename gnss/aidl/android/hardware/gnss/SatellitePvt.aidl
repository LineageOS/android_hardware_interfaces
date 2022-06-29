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

import android.hardware.gnss.SatelliteClockInfo;
import android.hardware.gnss.SatellitePositionEcef;
import android.hardware.gnss.SatelliteVelocityEcef;

/**
 * Contains estimates of the satellite position, velocity and time in the
 * ECEF coordinate frame.
 *
 * @hide
 */
@VintfStability
parcelable SatellitePvt {
    /**
     * Bit mask indicating valid satellite position, velocity and clock info fields are
     * stored in the SatellitePvt.
     */
    const int HAS_POSITION_VELOCITY_CLOCK_INFO = 1 << 0;

    /**
     * Bit mask indicating a valid iono delay field is stored in the SatellitePvt.
     */
    const int HAS_IONO = 1 << 1;

    /**
     * Bit mask indicating a valid tropo delay field is stored in the SatellitePvt.
     */
    const int HAS_TROPO = 1 << 2;

    /**
     * A bitfield of flags indicating the validity of the fields in this SatellitePvt.
     * The bit masks are defined in the constants with prefix HAS_*
     *
     * Fields for which there is no corresponding flag must be filled in with a valid value.
     * For convenience, these are marked as mandatory.
     *
     * Others fields may have invalid information in them, if not marked as valid by the
     * corresponding bit in flags.
     */
    int flags;

    /**
     * Satellite position in WGS84 ECEF. See comments of
     * SatellitePositionEcef for units.
     */
    SatellitePositionEcef satPosEcef;

    /**
     * Satellite velocity in WGS84 ECEF. See comments of
     * SatelliteVelocityEcef for units.
     */
    SatelliteVelocityEcef satVelEcef;

    /** Satellite clock bias and drift info. */
    SatelliteClockInfo satClockInfo;

    /** Ionospheric delay in meters. */
    double ionoDelayMeters;

    /** Tropospheric delay in meters. */
    double tropoDelayMeters;

    /**
     * Time of Clock in seconds.
     *
     * This value is defined in seconds since GPS epoch, regardless of the constellation.
     *
     * The value must not be encoded as in GPS ICD200 documentation.
     */
    long timeOfClockSeconds;

    /**
     * Issue of Data, Clock.
     *
     * This is defined in GPS ICD200 documentation
     * (e.g., https://www.gps.gov/technical/icwg/IS-GPS-200H.pdf).
     *
     * The field must be set to 0 if it is not supported.
     */
    int issueOfDataClock;

    /**
     * Time of Ephemeris in seconds.
     *
     * This value is defined in seconds since GPS epoch, regardless of the constellation.
     *
     * The value must not be encoded as in GPS ICD200 documentation.
     */
    long timeOfEphemerisSeconds;

    /**
     * Issue of Data, Ephemeris.
     *
     * This is defined in GPS ICD200 documentation
     * (e.g., https://www.gps.gov/technical/icwg/IS-GPS-200H.pdf).
     *
     * The field must be set to 0 if it is not supported.
     */
    int issueOfDataEphemeris;

    /** Satellite's ephemeris source */
    @VintfStability
    @Backing(type="int")
    enum SatelliteEphemerisSource {
        // Demodulated from broadcast signals
        DEMODULATED = 0,
        // Server provided Normal type ephemeris data, which is similar to broadcast ephemeris in
        // longevity (e.g. SUPL) - lasting for few hours and providing satellite orbit and clock
        // with accuracy of 1 - 2 meters.
        SERVER_NORMAL = 1,
        // Server provided Long-Term type ephemeris data, which lasts for many hours to several days
        // and often provides satellite orbit and clock accuracy of 2 - 20 meters.
        SERVER_LONG_TERM = 2,
        // Other source
        OTHER = 3,
    }

    /**
     * Source of the ephemeris.
     */
    SatelliteEphemerisSource ephemerisSource = SatelliteEphemerisSource.OTHER;
}
