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

import android.hardware.gnss.IGnssGeofenceCallback;

/**
 * Extended interface for GNSS Geofence support.
 *
 * @hide
 */
@VintfStability
interface IGnssGeofence {
    /**
     * Opens the geofence interface and provides the callback routines to the HAL.
     *
     * @param callback Handle to the IGnssGeofenceCallback interface.
     */
    void setCallback(in IGnssGeofenceCallback callback);

    /**
     * Add a geofence area. This api currently supports circular geofences.
     *
     * @param geofenceId The id for the geofence. If a geofence with this id already exists, an
     * error value (ERROR_ID_EXISTS) must be returned.
     * @param latitudeDegrees The latitude(in degrees) for the geofence lastTransition.
     * @param longitudeDegrees The longitude(in degrees) for the geofence lastTransition.
     * @param radiusMeters The radius(in meters) for the geofence lastTransition.
     * @param lastTransition The current state of the geofence. It can be one of the transition
     * states (ENTERED, EXITED, UNCERTAIN) as defined in IGnssGeofenceCallback. For example, if
     * the system already knows that the user is inside the geofence, this will be set to ENTERED.
     * In most cases, it will be UNCERTAIN.
     * @param monitorTransitions A bitfield of ENTERED, EXITED and UNCERTAIN. It represents which
     * transitions to monitor.
     * @param notificationResponsivenessMs - Defines the best-effort description of how soon must
     * the callback be called when the transition associated with the Geofence is triggered. For
     * instance, if set to 1000 milliseconds with ENTERED, the callback must be called 1000
     * milliseconds within entering the geofence. This parameter is defined in milliseconds.
     * NOTE: This is not to be confused with the rate that the GNSS is polled at. It is acceptable
     * to dynamically vary the rate of sampling the GNSS for power-saving reasons; thus the rate of
     * sampling may be faster or slower than this.
     * @param unknownTimerMs - The time limit in millisecondsafter which the UNCERTAIN transition
     * must be triggered.
     */
    void addGeofence(in int geofenceId, in double latitudeDegrees, in double longitudeDegrees,
            in double radiusMeters, in int lastTransition, in int monitorTransitions,
            in int notificationResponsivenessMs, in int unknownTimerMs);

    /**
     * Pause monitoring a particular geofence.
     *
     * @param geofenceId The id for the geofence.
     */
    void pauseGeofence(in int geofenceId);

    /**
     * Resume monitoring a particular geofence.
     *
     * @param geofenceId - The id for the geofence.
     * @param monitorTransitions Specifies which transitions to monitor. It can be a bitwise OR of
     * ENTERED, EXITED and UNCERTAIN. This supersedes the value associated provided in the
     * addGeofence call.
     */
    void resumeGeofence(in int geofenceId, in int monitorTransitions);

    /**
     * Remove a geofence area. After the function returns, no notifications must be sent.
     *
     * @param geofenceId The id of the geofence.
     */
    void removeGeofence(in int geofenceId);
}
