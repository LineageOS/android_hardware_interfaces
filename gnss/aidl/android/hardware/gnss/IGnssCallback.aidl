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

import android.hardware.gnss.GnssConstellationType;
import android.hardware.gnss.GnssLocation;
import android.hardware.gnss.GnssSignalType;
import android.hardware.gnss.IGnssConfiguration;
import android.hardware.gnss.IGnssPsds;

/**
 * This interface is required for the HAL to communicate certain information
 * like status and location info back to the framework, the framework implements
 * the interfaces and passes a handle to the HAL.
 *
 * @hide
 */
@VintfStability
interface IGnssCallback {
    /**
     * Capability bit mask indicating that GNSS supports scheduling fixes for RECURRENCE_PERIODIC
     * mode.
     *
     * If this is not set, then the framework will use 1000ms for minInterval and will call start()
     * and stop() to schedule the GNSS.
     */
    const int CAPABILITY_SCHEDULING = 1 << 0;

    /** Capability bit mask indicating that GNSS supports MS-Based AGNSS mode */
    const int CAPABILITY_MSB = 1 << 1;

    /** Capability bit mask indicating that GNSS supports MS-Assisted AGNSS mode */
    const int CAPABILITY_MSA = 1 << 2;

    /** Capability bit mask indicating that GNSS supports single-shot fixes */
    const int CAPABILITY_SINGLE_SHOT = 1 << 3;

    /**
     * Capability bit indicating that the platform should periodically inject
     * time to GNSS in addition to on-demand and occasional time updates.
     *
     * <p>Note:<em>The naming of "on demand" should be "periodic" instead.  This
     * is the result of a historic implementation bug, b/73893222.</em>
     */
    const int CAPABILITY_ON_DEMAND_TIME = 1 << 4;

    /** Capability bit mask indicating that GNSS supports Geofencing  */
    const int CAPABILITY_GEOFENCING = 1 << 5;

    /** Capability bit mask indicating that GNSS supports Measurements. */
    const int CAPABILITY_MEASUREMENTS = 1 << 6;

    /** Capability bit mask indicating that GNSS supports Navigation Messages */
    const int CAPABILITY_NAV_MESSAGES = 1 << 7;

    /** Capability bit mask indicating that GNSS supports low power mode */
    const int CAPABILITY_LOW_POWER_MODE = 1 << 8;

    /** Capability bit mask indicating that GNSS supports blocklisting satellites */
    const int CAPABILITY_SATELLITE_BLOCKLIST = 1 << 9;

    /** Capability bit mask indicating that GNSS supports measurement corrections */
    const int CAPABILITY_MEASUREMENT_CORRECTIONS = 1 << 10;

    /** Capability bit mask indicating that GNSS supports antenna info */
    const int CAPABILITY_ANTENNA_INFO = 1 << 11;

    /** Capability bit mask indicating that GNSS supports correlation vector */
    const int CAPABILITY_CORRELATION_VECTOR = 1 << 12;

    /** Capability bit mask indicating that GNSS supports satellite PVT */
    const int CAPABILITY_SATELLITE_PVT = 1 << 13;

    /** Capability bit mask indicating that GNSS supports measurement corrections for driving */
    const int CAPABILITY_MEASUREMENT_CORRECTIONS_FOR_DRIVING = 1 << 14;

    /** Capability bit mask indicating that GNSS supports accumulated delta range */
    const int CAPABILITY_ACCUMULATED_DELTA_RANGE = 1 << 15;

    /**
     * Callback to inform framework of the GNSS HAL implementation's capabilities.
     *
     * @param capabilities Capability parameter is a bit field of the Capability bit masks.
     */
    void gnssSetCapabilitiesCb(in int capabilities);

    /** GNSS status event values. */
    @VintfStability
    @Backing(type="int")
    enum GnssStatusValue {
        /** GNSS status unknown. */
        NONE = 0,
        /** GNSS has begun navigating. */
        SESSION_BEGIN = 1,
        /** GNSS has stopped navigating. */
        SESSION_END = 2,
        /** GNSS has powered on but is not navigating. */
        ENGINE_ON = 3,
        /** GNSS is powered off. */
        ENGINE_OFF = 4
    }

    /**
     * Flags that indicate information about the satellite
     */
    @VintfStability
    @Backing(type="int")
    enum GnssSvFlags {
        NONE = 0,
        HAS_EPHEMERIS_DATA = 1 << 0,
        HAS_ALMANAC_DATA = 1 << 1,
        USED_IN_FIX = 1 << 2,
        HAS_CARRIER_FREQUENCY = 1 << 3,
    }

    @VintfStability
    parcelable GnssSvInfo {
        /**
         * Pseudo-random or satellite ID number for the satellite, a.k.a. Space Vehicle (SV), or
         * FCN/OSN number for Glonass. The distinction is made by looking at constellation field.
         * Values must be in the range of:
         *
         * - GNSS:    1-32
         * - SBAS:    120-151, 183-192
         * - GLONASS: 1-24, the orbital slot number (OSN), if known.  Or, if not:
         *            93-106, the frequency channel number (FCN) (-7 to +6) offset by
         *            + 100
         *            i.e. report an FCN of -7 as 93, FCN of 0 as 100, and FCN of +6
         *            as 106.
         * - QZSS:    193-200
         * - Galileo: 1-36
         * - Beidou:  1-37
         * - IRNSS:   1-14
         */
        int svid;

        /**
         * Defines the constellation of the given SV.
         */
        GnssConstellationType constellation;

        /**
         * Carrier-to-noise density in dB-Hz, typically in the range [0, 63].
         * It contains the measured C/N0 value for the signal at the antenna port.
         *
         * This is a mandatory field.
         */
        float cN0Dbhz;

        /**
         * Baseband Carrier-to-noise density in dB-Hz, typically in the range [0, 63]. It contains
         * the measured C/N0 value for the signal measured at the baseband.
         *
         * This is typically a few dB weaker than the value estimated for C/N0 at the antenna port,
         * which is reported in cN0DbHz.
         *
         * If a signal has separate components (e.g. Pilot and Data channels) and the receiver only
         * processes one of the components, then the reported basebandCN0DbHz reflects only the
         * component that is processed.
         *
         * This field is mandatory. Like cN0DbHz, it may be reported as 0 for satellites being
         * reported that may be searched for, but not yet tracked.
         */
        float basebandCN0DbHz;

        /** Elevation of SV in degrees. */
        float elevationDegrees;

        /** Azimuth of SV in degrees. */
        float azimuthDegrees;

        /**
         * Carrier frequency of the signal tracked, for example it can be the
         * GPS central frequency for L1 = 1575.45 MHz, or L2 = 1227.60 MHz, L5 =
         * 1176.45 MHz, varying GLO channels, etc. If the field is zero, it is
         * the primary common use central frequency, e.g. L1 = 1575.45 MHz for
         * GPS.
         *
         * For an L1, L5 receiver tracking a satellite on L1 and L5 at the same
         * time, two GnssSvInfo structs must be reported for this same
         * satellite, in one of the structs, all the values related
         * to L1 must be filled, and in the other all of the values related to
         * L5 must be filled.
         *
         * If the data is available, svFlag must contain HAS_CARRIER_FREQUENCY.
         */
        long carrierFrequencyHz;

        /** A bit field of the GnssSvFlags. */
        int svFlag;
    }

    /**
     * Called to communicate the status of the GNSS engine.
     *
     * @param status Status information from HAL.
     */
    void gnssStatusCb(in GnssStatusValue status);

    /**
     * Callback for the HAL to pass a vector of GnssSvInfo back to the client.
     *
     * If only GnssMeasurement is registered, the SvStatus reporting interval must be
     * the same as the measurement interval, i.e., the interval the measurement
     * engine runs at. If only location is registered, the SvStatus interval must
     * be the same as the location interval. If both GnssMeasurement and location
     * are registered, then the SvStatus interval is the same as the lesser interval
     * between the two.
     *
     * @param svInfo SV status information from HAL.
     */
    void gnssSvStatusCb(in GnssSvInfo[] svInfoList);

    /**
     * Called when a GNSS location is available.
     *
     * @param location Location information from HAL.
     */
    void gnssLocationCb(in GnssLocation location);

    /**
     * Callback for reporting NMEA sentences. Called when NMEA data is available.
     *
     * @param timestamp Marks the instance of reporting.
     * @param nmea Follows standard NMEA 0183. Each sentence begins with a '$'
     * and ends with a carriage return/line feed sequence and can be no longer
     * than 80 characters of visible text (plus the line terminators). The data
     * is contained within this single line with data items separated by commas.
     * The data itself is just ascii text and may extend over multiple sentences
     * in certain specialized instances but is normally fully contained in one
     * variable length sentence. The data may vary in the amount of precision
     * contained in the message. For example time might be indicated to decimal
     * parts of a second or location may be shown with 3 or even 4 digits after
     * the decimal point. Programs that read the data must only use the commas
     * to determine the field boundaries and not depend on column positions.
     * There is a provision for a checksum at the end of each sentence which may
     * or may not be checked by the unit that reads the data. The checksum field
     * consists of a '*' and two hex digits representing an 8 bit exclusive OR
     * of all characters between, but not including, the '$' and '*'.
     */
    void gnssNmeaCb(in long timestamp, in @utf8InCpp String nmea);

    /**
     * Callback utility for acquiring the GNSS wakelock. This can be used to prevent
     * the CPU from suspending while handling GNSS events.
     */
    void gnssAcquireWakelockCb();

    /** Callback utility for releasing the GNSS wakelock. */
    void gnssReleaseWakelockCb();

    /**
     * Provides information about how new the underlying GPS/GNSS hardware and software is.
     */
    @VintfStability
    parcelable GnssSystemInfo {
        /**
         * The year in which the last update was made to the underlying hardware/firmware used to
         * capture GNSS signals, e.g. 2016.
         */
        int yearOfHw;

        /**
         * The name of the GNSS HAL implementation model and version name.
         *
         * This is a user-visible string that identifies the model and version of the GNSS HAL.
         * For example "ABC Co., Baseband Part 1234, RF Part 567, Software version 3.14.159"
         *
         * For privacy reasons, this string must not contain any device-specific serial number or
         * other identifier that uniquely identifies an individual device.
         */
        @utf8InCpp String name;
    }

    /**
     * Callback to inform the framework of the GNSS system information.
     *
     * This must be called in response to IGnss::setCallback
     *
     * @param info GnssSystemInfo about the GPS/GNSS hardware.
     */
    void gnssSetSystemInfoCb(in GnssSystemInfo info);

    /** Callback for requesting NTP time */
    void gnssRequestTimeCb();

    /**
     * Callback for requesting Location.
     *
     * HAL implementation must call this when it wants the framework to provide locations to assist
     * with GNSS HAL operation, for example, to assist with time to first fix, error recovery, or to
     * supplement GNSS location for other clients of the GNSS HAL.
     *
     * If a request is made with independentFromGnss set to true, the framework must avoid
     * providing locations derived from GNSS locations (such as "fused" location), to help improve
     * information independence for situations such as error recovery.
     *
     * In response to this method call, GNSS HAL can expect zero, one, or more calls to
     * IGnss::injectLocation or IGnss::injectBestLocation, dependent on availability of location
     * from other sources, which may happen at some arbitrary delay. Generally speaking, HAL
     * implementations must be able to handle calls to IGnss::injectLocation or
     * IGnss::injectBestLocation at any time.
     *
     * @param independentFromGnss True if requesting a location that is independent from GNSS.
     * @param isUserEmergency True if the location request is for delivery of this location to an
     *        emergency services endpoint, during a user-initiated emergency session (e.g.
     *        during-call to E911, or up to 5 minutes after end-of-call or text to E911).
     */
    void gnssRequestLocationCb(in boolean independentFromGnss, in boolean isUserEmergency);

    /**
     * Callback to inform the framework of the list of GnssSignalTypes the GNSS HAL implementation
     * supports.
     *
     * These capabilities are static at runtime, i.e., they represent the signal types the
     * GNSS implementation supports without considering the temporary disabled signal types such as
     * the blocklisted satellites/constellations or the constellations disabled by regional
     * restrictions.
     *
     * @param gnssSignalTypes a list of GnssSignalTypes specifying the constellations, carrier
     *     frequencies, and the code types the GNSS HAL implementation supports.
     */
    void gnssSetSignalTypeCapabilitiesCb(in GnssSignalType[] gnssSignalTypes);
}
