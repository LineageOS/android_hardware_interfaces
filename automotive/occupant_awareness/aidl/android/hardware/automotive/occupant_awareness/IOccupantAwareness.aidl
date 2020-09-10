/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hardware.automotive.occupant_awareness;

import android.hardware.automotive.occupant_awareness.OccupantAwarenessStatus;
import android.hardware.automotive.occupant_awareness.Role;
import android.hardware.automotive.occupant_awareness.IOccupantAwarenessClientCallback;
import android.hardware.automotive.occupant_awareness.OccupantDetections;

@VintfStability
interface IOccupantAwareness {
    /*
     * System not able to generate any occupancy awareness.
     */
    const int CAP_NONE = 0;
    /*
     * System is able to detect the presence of humans.
     */
    const int CAP_PRESENCE_DETECTION = 1 << 0;
    /*
     * System is able to detect the gaze of humans.
     */
    const int CAP_GAZE_DETECTION = 1 << 1;
    /*
     * System is able to compute the attention details of humans.
     */
    const int CAP_DRIVER_MONITORING_DETECTION = 1 << 2;

    /**
     * Starts the occupant awareness detection system. This is a non-blocking function call.
     * Once system is ready, state will be modified. State update can be inrquired using callback
     * or getState() function.
     * @return status is the current system state.
     */
    OccupantAwarenessStatus startDetection();

    /**
     * Stops the occupant awareness detection system. This is a non-blocking function call.
     * Once system is reset, state will be modified. State update can be inrquired using callback
     * or getState() function.
     * @return status is the current system state.
     */
    OccupantAwarenessStatus stopDetection();

    /**
     * Returns list of Awareness Capability supported for the given type of occupants.
     *
     * @param out Bitwise OR of supported capabilities (CAP_* mask).
     */
    int getCapabilityForRole(in Role occupantRole);

    /**
     * Inquires the current state of the occupant awareness system.
     * @param occupantRole specifies the role of occupants of interest.
     * @param detectionCapability specifies a single detection capability (CAP_* ) of interest.
     *
     * @return status is the current system state.
     */
    OccupantAwarenessStatus getState(in Role occupantRole, in int detectionCapability);

    /**
     * Registers a callback for data streaming. Only single callback is supported. setCallback()
     *        should replace existing callback with new callback.
     * @return: returns ok if successful.
     */
    void setCallback(in IOccupantAwarenessClientCallback callback);

    /**
     * Returns the most recent set of detections.
     * @param OccupantDetections output detections.
     * @return returns ok if successful.
     */
    void getLatestDetection(out OccupantDetections detections);
}
