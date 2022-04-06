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

import android.hardware.gnss.GnssLocation;

/**
 * The callback interface to report GNSS geofence events from the HAL.
 *
 * There are 3 states associated with a Geofence: Inside, Outside, Unknown.
 * There are 3 transitions: ENTERED, EXITED, UNCERTAIN.
 *
 * An example state diagram with confidence level: 95% and Unknown time limit
 * set as 30 secs is shown below. (confidence level and Unknown time limit are
 * explained latter).
 *                         ____________________________
 *                        |       Unknown (30 secs)   |
 *                         """"""""""""""""""""""""""""
 *                            ^ |                  |  ^
 *                   UNCERTAIN| |ENTERED     EXITED|  |UNCERTAIN
 *                            | v                  v  |
 *                        ________    EXITED     _________
 *                       | Inside | -----------> | Outside |
 *                       |        | <----------- |         |
 *                        """"""""    ENTERED    """""""""
 *
 * Inside state: We are 95% confident that the user is inside the geofence.
 * Outside state: We are 95% confident that the user is outside the geofence
 * Unknown state: Rest of the time.
 *
 * The Unknown state is better explained with an example:
 *
 *                            __________
 *                           |         c|
 *                           |  ___     |    _______
 *                           |  |a|     |   |   b   |
 *                           |  """     |    """""""
 *                           |          |
 *                            """"""""""
 * In the diagram above, "a" and "b" are 2 geofences and "c" is the accuracy
 * circle reported by the GNSS subsystem. Now with regard to "b", the system is
 * confident that the user is outside. But with regard to "a" is not confident
 * whether it is inside or outside the geofence. If the accuracy remains the
 * same for a sufficient period of time, the UNCERTAIN transition must be
 * triggered with the state set to Unknown. If the accuracy improves later, an
 * appropriate transition must be triggered.  This "sufficient period of time"
 * is defined by the parameter in the addGeofenceArea API.
 * In other words, Unknown state can be interpreted as a state in which the
 * GNSS subsystem isn't confident enough that the user is either inside or
 * outside the Geofence. It moves to Unknown state only after the expiry of the
 * timeout.
 *
 * The geofence callback needs to be triggered for the ENTERED and EXITED
 * transitions, when the GNSS system is confident that the user has entered
 * (Inside state) or exited (Outside state) the Geofence. An implementation
 * which uses a value of 95% as the confidence is recommended. The callback
 * must be triggered only for the transitions requested by the
 * addGeofenceArea method.
 *
 * Even though the diagram and explanation talks about states and transitions,
 * the callee is only interested in the transitions. The states are mentioned
 * here for illustrative purposes.
 *
 * Startup Scenario: When the device boots up, if an application adds geofences,
 * and then we get an accurate GNSS location fix, it needs to trigger the
 * appropriate (ENTERED or EXITED) transition for every Geofence it knows about.
 * By default, all the Geofences will be in the Unknown state.
 *
 * When the GNSS system is unavailable, gnssGeofenceStatusCb must be
 * called to inform the upper layers of the same. Similarly, when it becomes
 * available the callback must be called. This is a global state while the
 * UNKNOWN transition described above is per geofence.
 *
 * An important aspect to note is that users of this API (framework), will use
 * other subsystems like wifi, sensors, cell to handle Unknown case and
 * hopefully provide a definitive state transition to the third party
 * application. GNSS Geofence will just be a signal indicating what the GNSS
 * subsystem knows about the Geofence.
 *
 * @hide
 */
@VintfStability
interface IGnssGeofenceCallback {
    // Geofence transition status
    const int ENTERED = 1 << 0;
    const int EXITED = 1 << 1;
    const int UNCERTAIN = 1 << 2;

    // Geofence availability status
    const int UNAVAILABLE = 1 << 0;
    const int AVAILABLE = 1 << 1;

    // Geofence operation status
    const int OPERATION_SUCCESS = 0;
    const int ERROR_TOO_MANY_GEOFENCES = -100;
    const int ERROR_ID_EXISTS = -101;
    const int ERROR_ID_UNKNOWN = -102;
    const int ERROR_INVALID_TRANSITION = -103;
    const int ERROR_GENERIC = -149;

    /**
     * The callback associated with the geofence transition.
     *
     * The callback must only be called when the caller is interested in that particular transition.
     * For instance, if the caller is interested only in ENTERED transition, then the callback must
     * not be called with the EXITED transition.
     *
     * IMPORTANT: If a transition is triggered resulting in this callback, the GNSS subsystem will
     * wake up the application processor, if it is in suspend state.
     *
     * @param geofenceId The id associated with the addGeofenceArea.
     * @param location The current GNSS location.
     * @param transition Can be one of ENTERED, EXITED or UNCERTAIN.
     * @param timestamp Timestamp (in UTC milliseconds) when the transition was detected.
     */
    void gnssGeofenceTransitionCb(in int geofenceId, in GnssLocation location, in int transition,
            in long timestampMillis);

    /**
     * The callback associated with the availability of the GNSS system for geofencing monitoring.
     * If the GNSS system determines that it cannot monitor geofences because of lack of reliability
     * or unavailability of the GNSS signals, it will call this callback with UNAVAILABLE parameter.
     *
     * @param status - UNAVAILABLE or AVAILABLE.
     * @param lastLocation - Last known location.
     */
    void gnssGeofenceStatusCb(in int availability, in GnssLocation lastLocation);

    /**
     * The callback associated with the addGeofence call.
     *
     * @param geofenceId Id of the geofence.
     * @param status Returns OPERATION_SUCCESS if the geofence add was successful,
     *               returns ERROR_TOO_MANY_GEOFENCES if the geofence limit has been reached,
     *               returns ERROR_ID_EXISTS if geofence with id already exists,
     *               returns ERROR_INVALID_TRANSITION if the monitorTransition contains an invalid
     *               transition, and
     *               returns ERROR_GENERIC for other errors.
     */
    void gnssGeofenceAddCb(in int geofenceId, in int status);

    /**
     * The callback associated with the removeGeofence call.
     *
     * @param geofenceId Id of the geofence.
     * @param status Returns OPERATION_SUCCESS if successful,
     *               returns ERROR_ID_UNKNOWN for invalid id and
     *               returns ERROR_GENERIC for others.
     */
    void gnssGeofenceRemoveCb(in int geofenceId, in int status);

    /**
     * The callback associated with the pauseGeofence call.
     *
     * @param geofenceId Id of the geofence.
     * @param status Returns OPERATION_SUCCESS if success,
     *               returns ERROR_ID_UNKNOWN for invalid id,
     *               returns ERROR_INVALID_TRANSITION when monitorTransitions is invalid, and
     *               returns ERROR_GENERIC for other err errors.
     */
    void gnssGeofencePauseCb(in int geofenceId, in int status);

    /**
     * The callback associated with the resumeGeofence call.
     *
     * @param geofenceId - Id of the geofence.
     * @param status Returns OPERATION_SUCCESS if successful,
     *               returns ERROR_ID_UNKNOWN for invalid id, and
     *               returns ERROR_GENERIC for others.
     */
    void gnssGeofenceResumeCb(in int geofenceId, in int status);
}
