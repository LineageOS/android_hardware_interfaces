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
 * Used to enumerate the current type of Cruise Control (CC).
 *
 * This enum could be extended in future releases to include additional feature states.
 */
@VintfStability
@Backing(type="int")
enum CruiseControlType {
    /**
     * This state is used as an alternative for any CruiseControlType value that is not defined in
     * the platform. Ideally, implementations of VehicleProperty#CRUISE_CONTROL_TYPE should not use
     * this state. The framework can use this field to remain backwards compatible if
     * CruiseControlType is extended to include additional types.
     */
    OTHER = 0,
    /**
     * Standard cruise control is when a system in the vehicle automatically maintains a set speed
     * without the driver having to keep their foot on the accelerator. This version of cruise
     * control does not include automatic acceleration and deceleration to maintain a set time gap
     * from a vehicle ahead.
     */
    STANDARD = 1,
    /**
     * Adaptive cruise control is when a system in the vehicle automatically accelerates and
     * decelerates to maintain a set speed and/or a set time gap from a vehicle ahead.
     */
    ADAPTIVE = 2,
    /**
     * Predictive cruise control is a version of adaptive cruise control that also considers road
     * topography, road curvature, speed limit and traffic signs, etc. to actively adjust braking,
     * acceleration, gear shifting, etc. for the vehicle. This feature is often used to optimize
     * fuel consumption.
     */
    PREDICTIVE = 3,
}
