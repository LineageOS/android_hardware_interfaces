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

import android.hardware.gnss.gnss_assistance.GpsSatelliteEphemeris.GpsL2Params;
import android.hardware.gnss.gnss_assistance.GpsSatelliteEphemeris.GpsSatelliteClockModel;
import android.hardware.gnss.gnss_assistance.GpsSatelliteEphemeris.GpsSatelliteHealth;
import android.hardware.gnss.gnss_assistance.KeplerianOrbitModel;
import android.hardware.gnss.gnss_assistance.SatelliteEphemerisTime;

/**
 * Contains ephemeris parameters specific to QZSS satellites.
 * This is defined in IS-QZSS-PNT, section 4.1.2.
 *
 * @hide
 */
@VintfStability
parcelable QzssSatelliteEphemeris {
    /** Satellite PRN. */
    int prn;

    /** L2 parameters. */
    GpsL2Params gpsL2Params;

    /** Clock model. */
    GpsSatelliteClockModel satelliteClockModel;

    /** Orbit model. */
    KeplerianOrbitModel satelliteOrbitModel;

    /** Satellite health. */
    GpsSatelliteHealth satelliteHealth;

    /** Ephemeris time. */
    SatelliteEphemerisTime satelliteEphemerisTime;
}
