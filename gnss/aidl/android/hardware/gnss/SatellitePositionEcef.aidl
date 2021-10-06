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

/**
 * Contains estimates of the satellite position fields in ECEF coordinate frame.
 *
 * The satellite position must be defined at the time of transmission of the
 * signal receivedSvTimeNs.
 */
@VintfStability
parcelable SatellitePositionEcef {
    /** Satellite position X in WGS84 ECEF (meters). */
    double posXMeters;

    /** Satellite position Y in WGS84 ECEF (meters). */
    double posYMeters;

    /** Satellite position Z in WGS84 ECEF (meters). */
    double posZMeters;

    /**
     * The Signal in Space User Range Error (URE) (meters).
     *
     * It covers satellite position and clock errors projected to the pseudorange measurements.
     */
    double ureMeters;
}
