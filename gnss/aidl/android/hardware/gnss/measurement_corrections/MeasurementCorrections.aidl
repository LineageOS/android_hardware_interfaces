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

package android.hardware.gnss.measurement_corrections;

import android.hardware.gnss.measurement_corrections.SingleSatCorrection;

/**
 * A struct containing a set of measurement corrections for all used GNSS satellites at the location
 * specified by latitudeDegrees, longitudeDegrees, altitudeMeters and at the time of week specified
 * toaGpsNanosecondsOfWeek
 *
 * @hide
 */
@VintfStability
parcelable MeasurementCorrections {
    /** Represents latitude in degrees at which the corrections are computed.. */
    double latitudeDegrees;

    /** Represents longitude in degrees at which the corrections are computed.. */
    double longitudeDegrees;

    /**
     * Represents altitude in meters above the WGS 84 reference ellipsoid at which the corrections
     * are computed.
     */
    double altitudeMeters;

    /**
     * Represents the horizontal uncertainty (63% to 68% confidence) in meters on the device
     * position at which the corrections are provided.
     *
     * This value is useful for example to judge how accurate the provided corrections are.
     */
    double horizontalPositionUncertaintyMeters;

    /**
     * Represents the vertical uncertainty (63% to 68% confidence) in meters on the device position
     * at which the corrections are provided.
     *
     * This value is useful for example to judge how accurate the provided corrections are.
     */
    double verticalPositionUncertaintyMeters;

    /** Time Of Applicability, GPS time of week in nanoseconds. */
    long toaGpsNanosecondsOfWeek;

    /**
     * A set of SingleSatCorrection each containing measurement corrections for a satellite in view
     */
    SingleSatCorrection[] satCorrections;

    /**
     * Boolean indicating if environment bearing is available.
     */
    boolean hasEnvironmentBearing;

    /**
     * Environment bearing in degrees clockwise from true North (0.0 to 360.0], in direction of
     * user motion. Environment bearing is provided when it is known with high probability that
     * velocity is aligned with an environment feature, such as a building or road.
     *
     * If user speed is zero, environmentBearingDegrees represents bearing of most recent speed
     * that was > 0.
     *
     * As position approaches another road, environmentBearingUncertaintyDegrees will grow, and at
     * some stage hasEnvironmentBearing = false.
     *
     * As position moves towards an open area, environmentBearingUncertaintyDegrees will grow, and
     * at some stage hasEnvironmentBearing = false.
     *
     * If the road is curved in the vicinity of the user location, then
     * environmentBearingUncertaintyDegrees will include the amount by which the road direction
     * changes in the area of position uncertainty.
     *
     * hasEnvironmentBearing should be checked to verify the environment bearing is available
     * before calling this method. The value is undefined if hasEnvironmentBearing is false.
     */
    float environmentBearingDegrees;

    /**
     * Environment bearing uncertainty [0 to 180]. It represents the standard deviation of the
     * physical structure in the circle of position uncertainty. hasEnvironmentBearing becomes false
     * as the uncertainty value passes a predefined threshold depending on the physical structure
     * around the user.
     *
     * hasEnvironmentBearing should be checked to verify the environment bearing is available
     * before calling this method. The value is undefined if hasEnvironmentBearing is false.
     */
    float environmentBearingUncertaintyDegrees;
}
