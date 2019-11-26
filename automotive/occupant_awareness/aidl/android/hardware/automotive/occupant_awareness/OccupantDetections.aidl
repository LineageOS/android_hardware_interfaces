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

import android.hardware.automotive.occupant_awareness.OccupantDetection;

@VintfStability
parcelable OccupantDetections {
    /**
     * Timestamp that the underlying source image was captured, in milliseconds since Jan 1, 1970
     * (Unix time).
     */
    long timeStampMillis;
    /**
     * A vector of detections for all occupants in the vehicle. One OccupantDetection will be
     * generated per detected face.
     */
    OccupantDetection[] detections;
}

