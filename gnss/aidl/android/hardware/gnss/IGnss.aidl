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

import android.hardware.gnss.GnssLocation;
import android.hardware.gnss.IAGnss;
import android.hardware.gnss.IAGnssRil;
import android.hardware.gnss.IGnssAntennaInfo;
import android.hardware.gnss.IGnssBatching;
import android.hardware.gnss.IGnssCallback;
import android.hardware.gnss.IGnssConfiguration;
import android.hardware.gnss.IGnssDebug;
import android.hardware.gnss.IGnssGeofence;
import android.hardware.gnss.IGnssMeasurementInterface;
import android.hardware.gnss.IGnssNavigationMessageInterface;
import android.hardware.gnss.IGnssPowerIndication;
import android.hardware.gnss.IGnssPsds;
import android.hardware.gnss.measurement_corrections.IMeasurementCorrectionsInterface;
import android.hardware.gnss.visibility_control.IGnssVisibilityControl;

/**
 * Represents the standard GNSS (Global Navigation Satellite System) interface.
 *
 * @hide
 */
@VintfStability
interface IGnss {
    /**
     * All GNSS binder calls may return a ServiceSpecificException with the following error
     * codes.
     */
    const int ERROR_INVALID_ARGUMENT = 1;

    /** A callback has already been registered. */
    const int ERROR_ALREADY_INIT = 2;

    /** Any other error. */
    const int ERROR_GENERIC = 3;

    /** Requested operational mode for GNSS operation. */
    @VintfStability
    @Backing(type="int")
    enum GnssPositionMode {
        /** Mode for running GNSS standalone (no assistance). */
        STANDALONE = 0,
        /** AGNSS MS-Based mode. */
        MS_BASED = 1,
        /**
         * AGNSS MS-Assisted mode. This mode is not maintained by the platform anymore.
         * It is strongly recommended to use MS_BASED instead.
         */
        MS_ASSISTED = 2,
    }

    /** Requested recurrence mode for GNSS operation. */
    @VintfStability
    @Backing(type="int")
    enum GnssPositionRecurrence {
        /** Receive GNSS fixes on a recurring basis at a specified period. */
        RECURRENCE_PERIODIC = 0,
        /** Request a single shot GNSS fix. */
        RECURRENCE_SINGLE = 1,
    }

    /**
     * Flags used to specify which aiding data to delete when calling
     * deleteAidingData().
     */
    @VintfStability
    @Backing(type="int")
    enum GnssAidingData {
        EPHEMERIS = 0x0001,
        ALMANAC = 0x0002,
        POSITION = 0x0004,
        TIME = 0x0008,
        IONO = 0x0010,
        UTC = 0x0020,
        HEALTH = 0x0040,
        SVDIR = 0x0080,
        SVSTEER = 0x0100,
        SADATA = 0x0200,
        RTI = 0x0400,
        CELLDB_INFO = 0x8000,
        ALL = 0xFFFF
    }

    /**
     * Opens the interface and provides the callback routines to the implementation of this
     * interface.
     *
     * The framework calls this method to instruct the GPS engine to prepare for serving requests
     * from the framework. The GNSS HAL implementation must respond to all GNSS requests from the
     * framework upon successful return from this method until close() method is called to
     * close this interface.
     *
     * @param callback Callback interface for IGnss.
     */
    void setCallback(in IGnssCallback callback);

    /**
     * Closes the interface.
     *
     * The close() method is called by the framework to tell the GNSS HAL implementation to
     * clear the callback and not expect any GNSS requests in the immediate future - e.g. this may
     * be called when location is disabled by a user setting or low battery conditions. The GNSS HAL
     * implementation must immediately stop responding to any existing requests until the
     * setCallback() method is called again and the requests are re-initiated by the framework.
     *
     * After this method is called, the GNSS HAL implementation may choose to modify GNSS hardware
     * states to save power. It is expected that when setCallback() method is called again to
     * reopen this interface, to serve requests, there may be some minor delays in GNSS response
     * requests as hardware readiness states are restored, not to exceed those that occur on normal
     * device boot up.
     */
    void close();

    /**
     * This method returns the IGnssPsds interface.
     *
     * @return The IGnssPsds interface.
     */
    @nullable IGnssPsds getExtensionPsds();

    /**
     * This method returns the IGnssConfiguration interface.
     *
     * This method must return non-null.
     *
     * @return The IGnssConfiguration interface.
     */
    IGnssConfiguration getExtensionGnssConfiguration();

    /**
     * This method returns the IGnssMeasurementInterface interface.
     *
     * This method must return non-null.
     *
     * @return The IGnssMeasurementInterface interface.
     */
    IGnssMeasurementInterface getExtensionGnssMeasurement();

    /**
     * This method returns the IGnssPowerIndication interface.
     *
     * This method must return non-null.
     *
     * @return The IGnssPowerIndication interface.
     */
    IGnssPowerIndication getExtensionGnssPowerIndication();

    /**
     * This method returns the IGnssBatching interface.
     *
     * @return The IGnssBatching interface.
     */
    @nullable IGnssBatching getExtensionGnssBatching();

    /**
     * This method returns the IGnssGeofence interface.
     *
     * @return The IGnssGeofence interface.
     */
    @nullable IGnssGeofence getExtensionGnssGeofence();

    /**
     * This method returns the IGnssNavigationMessageInterface.
     *
     * @return The IGnssNavigationMessageInterface.
     */
    @nullable IGnssNavigationMessageInterface getExtensionGnssNavigationMessage();

    /**
     * This method returns the IAGnss interface.
     *
     * @return The IAGnss interface.
     */
    IAGnss getExtensionAGnss();

    /**
     * This method returns the IAGnssRil interface.
     *
     * @return The IAGnssRil interface.
     */
    IAGnssRil getExtensionAGnssRil();

    /**
     * This method returns the IGnssDebug interface.
     *
     * This method must return non-null.
     *
     * @return Handle to the IGnssDebug interface.
     */
    IGnssDebug getExtensionGnssDebug();

    /**
     * This method returns the IGnssVisibilityControl.
     *
     * @return Handle to the IGnssVisibilityControl.
     */
    IGnssVisibilityControl getExtensionGnssVisibilityControl();

    /**
     * Starts a location output stream using the IGnssCallback gnssLocationCb(), following the
     * settings from the most recent call to setPositionMode().
     *
     * This output must operate independently of any GNSS location batching operations,
     * see the IGnssBatching for details.
     */
    void start();

    /**
     * Stops the location output stream.
     */
    void stop();

    /**
     * Injects the current time.
     *
     * @param timeMs This is the UTC time received from the NTP server, its value is given in
     *     milliseconds since January 1, 1970.
     * @param timeReferenceMs The corresponding value of SystemClock.elapsedRealtime() from the
     *     device when the NTP response was received in milliseconds.
     * @param uncertaintyMs Uncertainty associated with the value represented by time. Represented
     *     in milliseconds.
     */
    void injectTime(in long timeMs, in long timeReferenceMs, in int uncertaintyMs);

    /**
     * Injects current location from another (typically network) location provider.
     *
     * @param location Current location from the location provider
     */
    void injectLocation(in GnssLocation location);

    /**
     * Injects current location from the best available location provider.
     *
     * Unlike injectLocation, this method may inject a recent GNSS location from the HAL
     * implementation, if that is the best available location known to the framework.
     *
     * @param location Location information from the best available location provider.
     */
    void injectBestLocation(in GnssLocation location);

    /**
     * Specifies that the next call to start will not use the information defined in the flags.
     * GnssAidingData value of GnssAidingData::ALL is passed for a cold start.
     *
     * @param aidingDataFlags Flags specifying the aiding data to be deleted.
     */
    void deleteAidingData(in GnssAidingData aidingDataFlags);

    /**
     * Options used in the setPositionMode() call for specifying the GNSS engine behavior.
     */
    @VintfStability
    parcelable PositionModeOptions {
        /**
         * Must be one of MS_BASED or STANDALONE. It is allowed by the platform (and it is
         * recommended) to fallback to MS_BASED if MS_ASSISTED is passed in, and MS_BASED is
         * supported.
         */
        GnssPositionMode mode;

        /* Recurrence GNSS position recurrence value, either periodic or single. */
        GnssPositionRecurrence recurrence;

        /* Represents the time between fixes in milliseconds. */
        int minIntervalMs;

        /* Represents the requested fix accuracy in meters. */
        int preferredAccuracyMeters;

        /* Represents the requested time to first fix in milliseconds. */
        int preferredTimeMs;

        /**
         * When true, and IGnss is the only client to the GNSS hardware, the GNSS hardware must make
         * strong tradeoffs to substantially restrict power use. Specifically, in the case of a
         * several second long minIntervalMs, the GNSS hardware must not, on average, run power
         * hungry operations like RF and signal searches for more than one second per interval, and
         * must make exactly one call to gnssSvStatusCb(), and either zero or one call to
         * GnssLocationCb() at each interval. When false, HAL must operate in the nominal mode and
         * is expected to make power and performance tradoffs such as duty-cycling when signal
         * conditions are good and more active searches to reacquire GNSS signals when no signals
         * are present. When there are additional clients using the GNSS hardware other than IGnss,
         * the GNSS hardware may operate in a higher power mode, on behalf of those clients.
         */
        boolean lowPowerMode;
    }

    /**
     * Sets the GnssPositionMode parameter, its associated recurrence value, the time between fixes,
     * requested fix accuracy, time to first fix.
     */
    void setPositionMode(in PositionModeOptions options);

    /*
     * This method returns the IGnssAntennaInfo.
     *
     * @return Handle to the IGnssAntennaInfo.
     */
    IGnssAntennaInfo getExtensionGnssAntennaInfo();

    /**
     * This method returns the IMeasurementCorrectionsInterface.
     *
     * @return Handle to the IMeasurementCorrectionsInterface.
     */
    @nullable IMeasurementCorrectionsInterface getExtensionMeasurementCorrections();

    /**
     * Starts a SvStatus output stream using the IGnssCallback gnssSvStatusCb().
     */
    void startSvStatus();

    /**
     * Stops the SvStatus output stream.
     */
    void stopSvStatus();

    /**
     * Starts an NMEA (National Marine Electronics Association) output stream using the
     * IGnssCallback gnssNmeaCb().
     */
    void startNmea();

    /**
     * Stops the NMEA output stream.
     */
    void stopNmea();
}
