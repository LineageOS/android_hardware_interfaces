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

import android.hardware.automotive.occupant_awareness.VehicleRegion;
import android.hardware.automotive.occupant_awareness.ConfidenceLevel;

@VintfStability
parcelable GazeDetection {
    /*
     * Confidence level for the gaze detection.
     */
    ConfidenceLevel gazeConfidence;
    /*
     * Head position, in millimeters. The vehicle coordinate system is specified in the cabin space
     * configuration. headPosition is double[3] array.
     */
    double[] headPosition;
    /*
     * Unit vector for the head pose direction. The vehicle coordinate system is specified in the
     * cabin space configuration. headAngleUnitVector is double[3] array.
     */
    double[] headAngleUnitVector;
    /*
     * Unit vector for the gaze direction. The vehicle coordinate system is specified in the cabin
     * space configuration. gazeAngleUnitVector is double[3] array.
     */
    double[] gazeAngleUnitVector;
    /*
     * Current gaze target.
     */
    VehicleRegion gazeTarget;
    /*
     * Custom gaze target string. This only need to be populated when gazeTarget is CUSTOM_TARGET.
     * Vendors should use "com.vendor_name.target_name" format to avoid name collision with other
     * vendors.
     */
    String customGazeTarget;
    /*
     * Duration that the subject has been looking at the current gaze target in milliseconds.
     */
    long timeOnTargetMillis;
}
