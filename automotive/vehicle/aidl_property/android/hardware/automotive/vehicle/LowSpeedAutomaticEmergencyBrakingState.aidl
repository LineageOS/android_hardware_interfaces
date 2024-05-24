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
 * Used to enumerate the state of Low Speed Automatic Emergency Braking.
 */
@VintfStability
@Backing(type="int")
enum LowSpeedAutomaticEmergencyBrakingState {

    /**
     * This state is used as an alternative to any LowSpeedAutomaticEmergencyBrakingState value that
     * is not defined in the platform. Ideally, implementations of
     * VehicleProperty#LOW_SPEED_AUTOMATIC_EMERGENCY_BRAKING_STATE should not use this state. The
     * framework can use this field to remain backwards compatible if
     * LowSpeedAutomaticEmergencyBrakingState is extended to include additional states.
     */
    OTHER = 0,
    /**
     * Low Speed Automatic Emergency Braking is enabled and monitoring safety, but brakes are not
     * activated.
     */
    ENABLED = 1,
    /**
     * Low Speed Automatic Emergency Braking is enabled and currently has the brakes applied for the
     * vehicle.
     */
    ACTIVATED = 2,
    /**
     * Many Low Speed Automatic Emergency Braking implementations allow the driver to override Low
     * Speed Automatic Emergency Braking. This means that the car has determined it should brake,
     * but a user decides to take over and do something else. This is often done for safety reasons
     * and to ensure that the driver can always take control of the vehicle. This state should be
     * set when the user is actively overriding the low speed automatic emergency braking system.
     */
    USER_OVERRIDE = 3,
}
