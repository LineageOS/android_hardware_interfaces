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

import android.hardware.gnss.ElapsedRealtime;

/** Represents a location. @hide */
@VintfStability
parcelable GnssLocation {
    /** Bit mask to indicate GnssLocation has valid latitude and longitude. */
    const int HAS_LAT_LONG = 0x0001;
    /** Bit mask to indicate GnssLocation has valid altitude. */
    const int HAS_ALTITUDE = 0x0002;
    /** Bit mask to indicate GnssLocation has valid speed. */
    const int HAS_SPEED = 0x0004;
    /** Bit mask to indicate GnssLocation has valid bearing. */
    const int HAS_BEARING = 0x0008;
    /** Bit mask to indicate GnssLocation has valid horizontal accuracy. */
    const int HAS_HORIZONTAL_ACCURACY = 0x0010;
    /** Bit mask to indicate GnssLocation has valid vertical accuracy. */
    const int HAS_VERTICAL_ACCURACY = 0x0020;
    /** Bit mask to indicate GnssLocation has valid speed accuracy. */
    const int HAS_SPEED_ACCURACY = 0x0040;
    /** Bit mask to indicate GnssLocation has valid bearing accuracy. */
    const int HAS_BEARING_ACCURACY = 0x0080;

    /** A bit field of flags indicating the validity of the fields in this GnssLocation. */
    int gnssLocationFlags;

    /** Represents latitude in degrees. */
    double latitudeDegrees;

    /** Represents longitude in degrees. */
    double longitudeDegrees;

    /** Represents altitude in meters above the WGS 84 reference ellipsoid. */
    double altitudeMeters;

    /** Represents speed in meters per second. */
    double speedMetersPerSec;

    /** Represents heading in degrees. */
    double bearingDegrees;

    /**
     * Represents expected horizontal position accuracy, radial, in meters (68% confidence).
     */
    double horizontalAccuracyMeters;

    /**
     * Represents expected vertical position accuracy in meters (68% confidence).
     */
    double verticalAccuracyMeters;

    /**
     * Represents expected speed accuracy in meter per seconds (68% confidence).
     */
    double speedAccuracyMetersPerSecond;

    /**
     * Represents expected bearing accuracy in degrees (68% confidence).
     */
    double bearingAccuracyDegrees;

    /** Timestamp for the location fix in milliseconds since January 1, 1970. */
    long timestampMillis;

    /**
     * Timing information of the GNSS location synchronized with SystemClock.elapsedRealtimeNanos()
     * clock.
     *
     * This clock information can be obtained from SystemClock.elapsedRealtimeNanos(), when the GNSS
     * is attached straight to the AP/SOC. When it is attached to a separate module the timestamp
     * needs to be estimated by syncing the notion of time via PTP or some other mechanism.
     */
    ElapsedRealtime elapsedRealtime;
}
