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
import android.hardware.automotive.occupant_awareness.OccupantDetections;

@VintfStability
interface IOccupantAwarenessClientCallback {
    /**
     * A callback invoked when the system status changes.
     *
     * @param detectionFlags The detection subsystem(s) whose status has changed.
     * @param status The new system status.
     */
    oneway void onSystemStatusChanged(in int detectionFlags, in OccupantAwarenessStatus status);

    /**
     * A callback invoked when a new set of detections are available.
     *
     * @param detections Occupant detections.
     */
    oneway void onDetectionEvent(in OccupantDetections detections);
}
