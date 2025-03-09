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

import android.hardware.gnss.gnss_assistance.BeidouSatelliteEphemeris;
import android.hardware.gnss.gnss_assistance.GalileoIonosphericModel;
import android.hardware.gnss.gnss_assistance.GalileoSatelliteEphemeris;
import android.hardware.gnss.gnss_assistance.GlonassAlmanac;
import android.hardware.gnss.gnss_assistance.GlonassSatelliteEphemeris;
import android.hardware.gnss.gnss_assistance.GnssAlmanac;
import android.hardware.gnss.gnss_assistance.GpsSatelliteEphemeris;
import android.hardware.gnss.gnss_assistance.IonosphericCorrection;
import android.hardware.gnss.gnss_assistance.KlobucharIonosphericModel;
import android.hardware.gnss.gnss_assistance.LeapSecondsModel;
import android.hardware.gnss.gnss_assistance.QzssSatelliteEphemeris;
import android.hardware.gnss.gnss_assistance.RealTimeIntegrityModel;
import android.hardware.gnss.gnss_assistance.TimeModel;
import android.hardware.gnss.gnss_assistance.UtcModel;

/**
 * Contains GNSS assistance.
 *
 * @hide
 */
@VintfStability
parcelable GnssAssistance {
    /** GNSS corrections for satellites. */
    @VintfStability
    parcelable GnssSatelliteCorrections {
        /**
         * Pseudo-random or satellite ID number for the satellite, a.k.a. Space Vehicle (SV), or
         * OSN number for Glonass. The distinction is made by looking at the constellation field.
         * Values must be in the range of:
         *
         * - GNSS:    1-32
         * - GLONASS: 1-25
         * - QZSS:    183-206
         * - Galileo: 1-36
         * - Beidou:  1-63
         */
        int svid;

        /** Ionospheric corrections */
        IonosphericCorrection[] inonosphericCorrections;
    }

    /** Contains GPS assistance. */
    @VintfStability
    parcelable GpsAssistance {
        /** The GPS almanac. */
        GnssAlmanac almanac;

        /** The Klobuchar ionospheric model. */
        KlobucharIonosphericModel ionosphericModel;

        /** The UTC model. */
        UtcModel utcModel;

        /** The leap seconds model. */
        LeapSecondsModel leapSecondsModel;

        /** The array of time models. */
        TimeModel[] timeModels;

        /** The array of GPS ephemeris. */
        GpsSatelliteEphemeris[] satelliteEphemeris;

        /** The array of real time integrity models. */
        RealTimeIntegrityModel[] realTimeIntegrityModels;

        /** The array of GPS satellite corrections. */
        GnssSatelliteCorrections[] satelliteCorrections;
    }

    /** Contains Galileo assistance. */
    @VintfStability
    parcelable GalileoAssistance {
        /** The Galileo almanac. */
        GnssAlmanac almanac;

        /** The Galileo ionospheric model. */
        GalileoIonosphericModel ionosphericModel;

        /** The UTC model. */
        UtcModel utcModel;

        /** The leap seconds model. */
        LeapSecondsModel leapSecondsModel;

        /** The array of time models. */
        TimeModel[] timeModels;

        /** The array of Galileo ephemeris. */
        GalileoSatelliteEphemeris[] satelliteEphemeris;

        /** The array of real time integrity models. */
        RealTimeIntegrityModel[] realTimeIntegrityModels;

        /** The array of Galileo satellite corrections. */
        GnssSatelliteCorrections[] satelliteCorrections;
    }

    /** Contains Glonass assistance. */
    @VintfStability
    parcelable GlonassAssistance {
        /** The Glonass almanac. */
        GlonassAlmanac almanac;

        /** The UTC model. */
        UtcModel utcModel;

        /** The array of time models. */
        TimeModel[] timeModels;

        /** The array of Glonass ephemeris. */
        GlonassSatelliteEphemeris[] satelliteEphemeris;

        /** The array of Glonass satellite corrections. */
        GnssSatelliteCorrections[] satelliteCorrections;
    }

    /** Contains QZSS assistance. */
    @VintfStability
    parcelable QzssAssistance {
        /** The QZSS almanac. */
        GnssAlmanac almanac;

        /** The Klobuchar ionospheric model. */
        KlobucharIonosphericModel ionosphericModel;

        /** The UTC model. */
        UtcModel utcModel;

        /** The leap seconds model. */
        LeapSecondsModel leapSecondsModel;

        /** The array of time models. */
        TimeModel[] timeModels;

        /** The array of QZSS ephemeris. */
        QzssSatelliteEphemeris[] satelliteEphemeris;

        /** The array of real time integrity models. */
        RealTimeIntegrityModel[] realTimeIntegrityModels;

        /** The array of QZSS satellite corrections. */
        GnssSatelliteCorrections[] satelliteCorrections;
    }

    /** Contains Beidou assistance. */
    @VintfStability
    parcelable BeidouAssistance {
        /** The Beidou almanac. */
        GnssAlmanac almanac;

        /** The Klobuchar ionospheric model. */
        KlobucharIonosphericModel ionosphericModel;

        /** The UTC model. */
        UtcModel utcModel;

        /** The leap seconds model. */
        LeapSecondsModel leapSecondsModel;

        /** The array of time models. */
        TimeModel[] timeModels;

        /** The array of Beidou ephemeris. */
        BeidouSatelliteEphemeris[] satelliteEphemeris;

        /** The array of real time integrity models. */
        RealTimeIntegrityModel[] realTimeIntegrityModels;

        /** The array of Beidou satellite corrections. */
        GnssSatelliteCorrections[] satelliteCorrections;
    }

    /** GPS assistance. */
    GpsAssistance gpsAssistance;

    /** Glonass assistance. */
    GlonassAssistance glonassAssistance;

    /** Galileo assistance. */
    GalileoAssistance galileoAssistance;

    /** Beidou assistance. */
    BeidouAssistance beidouAssistance;

    /** QZSS assistance. */
    QzssAssistance qzssAssistance;
}
