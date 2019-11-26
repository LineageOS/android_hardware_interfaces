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

import android.hardware.automotive.occupant_awareness.Role;
import android.hardware.automotive.occupant_awareness.PresenceDetection;
import android.hardware.automotive.occupant_awareness.GazeDetection;
import android.hardware.automotive.occupant_awareness.DriverMonitoringDetection;

/*
 * A complete detection for a single occupant in the vehicle. Includes data about the subject's
 * presence in the vehicle, gaze and attention.
 */
 @VintfStability
parcelable OccupantDetection {
    /*
     * Role of the occupant (e.g., driver, passenger).
     */
    Role role;
    /*
     * Occupant presence state for a single occupant.
     * If the vector is empty, no data could be generated.
     */
    PresenceDetection[] presenceData;
    /*
     * Gaze data for a single occupant.
     * If the vector is empty, no data could be generated.
     */
    GazeDetection[] gazeData;
    /*
     * Attention data for a single occupant.
     * If the vector is empty, no data could be generated.
     */
    DriverMonitoringDetection[] attentionData;
}

