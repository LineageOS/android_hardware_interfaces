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
 * Contains estimates of the satellite velocity fields in the ECEF coordinate frame.
 *
 * The satellite velocity must be defined at the time of transmission of the
 * signal receivedSvTimeNs.
 */
@VintfStability
parcelable SatelliteVelocityEcef {
    /** Satellite velocity X in WGS84 ECEF (meters per second). */
    double velXMps;

    /** Satellite velocity Y in WGS84 ECEF (meters per second). */
    double velYMps;

    /** Satellite velocity Z in WGS84 ECEF (meters per second). */
    double velZMps;

    /**
     * The Signal in Space User Range Error Rate (URE Rate) (meters per second).
     *
     * It covers satellite velocity error and Satellite clock drift
     * projected to the pseudorange rate measurements.
     */
    double ureRateMps;
}
