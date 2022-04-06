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

/**
 * A struct containing the characteristics of the reflecting plane that the satellite signal has
 * bounced from.
 *
 * The value is only valid if HAS_REFLECTING_PLANE flag is set. An invalid reflecting plane
 * means either reflection planes serving is not supported or the satellite signal has gone
 * through multiple reflections.
 *
 * @hide
 */
@VintfStability
parcelable ReflectingPlane {
    /** Represents latitude of the reflecting plane in degrees. */
    double latitudeDegrees;

    /** Represents longitude of the reflecting plane in degrees. */
    double longitudeDegrees;

    /**
     * Represents altitude of the reflecting point in the plane in meters above the WGS 84 reference
     * ellipsoid.
     */
    double altitudeMeters;

    /** Represents azimuth clockwise from north of the reflecting plane in degrees. */
    double reflectingPlaneAzimuthDegrees;
}
