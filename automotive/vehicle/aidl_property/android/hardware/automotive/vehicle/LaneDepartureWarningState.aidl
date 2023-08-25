/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.automotive.vehicle;

/**
 * Used to enumerate the state of Lane Departure Warning (LDW).
 */
@VintfStability
@Backing(type="int")
enum LaneDepartureWarningState {

    /**
     * This state is used as an alternative for any LaneDepartureWarningState value that is not
     * defined in the platform. Ideally, implementations of
     * VehicleProperty#LANE_DEPARTURE_WARNING_STATE should not use this state. The framework
     * can use this field to remain backwards compatible if LaneDepartureWarningState is
     * extended to include additional states.
     */
    OTHER = 0,
    /**
     * LDW is enabled and monitoring, but the vehicle is centered in the lane.
     */
    NO_WARNING = 1,
    /**
     * LDW is enabled, detects the vehicle is approaching or crossing lane lines on the left side
     * of the vehicle, and is currently warning the user.
     */
    WARNING_LEFT = 2,
    /**
     * LDW is enabled, detects the vehicle is approaching or crossing lane lines on the right side
     * of the vehicle, and is currently warning the user.
     */
    WARNING_RIGHT = 3,
}
