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

import android.hardware.automotive.occupant_awareness.ConfidenceLevel;

@VintfStability
parcelable DriverMonitoringDetection {
    /*
     * Confidence of the computed attention data.
     */
    ConfidenceLevel confidenceScore;
    /*
     * Is the driver currently looking on-road?
     */
    boolean isLookingOnRoad;
    /*
     * Duration the driver has been looking on or off road, in milliseconds.
     */
    long gazeDurationMillis;
}

