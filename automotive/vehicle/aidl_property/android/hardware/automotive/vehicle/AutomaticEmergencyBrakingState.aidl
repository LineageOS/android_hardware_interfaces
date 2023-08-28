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
 * Used to enumerate the state of Automatic Emergency Braking (AEB).
 */
@VintfStability
@Backing(type="int")
enum AutomaticEmergencyBrakingState {

    /**
     * This state is used as an alternative to any AutomaticEmergencyBrakingState value that is not
     * defined in the platform. Ideally, implementations of
     * VehicleProperty#AUTOMATIC_EMERGENCY_BRAKING_STATE should not use this state. The framework
     * can use this field to remain backwards compatible if AutomaticEmergencyBrakingState is
     * extended to include additional states.
     */
    OTHER = 0,
    /**
     * AEB is enabled and monitoring safety, but brakes are not activated.
     */
    ENABLED = 1,
    /**
     * AEB is enabled and currently has the brakes applied for the vehicle.
     */
    ACTIVATED = 2,
    /**
     * Many AEB implementations allow the driver to override AEB. This means that the car has
     * determined it should brake, but a user decides to take over and do something else. This is
     * often done for safety reasons and to ensure that the driver can always take control of the
     * vehicle. This state should be set when the user is actively overriding the AEB system.
     */
    USER_OVERRIDE = 3,
}
