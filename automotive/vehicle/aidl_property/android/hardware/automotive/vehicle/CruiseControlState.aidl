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
 * Used to enumerate the current state of Cruise Control (CC).
 *
 * This enum could be extended in future releases to include additional feature states.
 */
@VintfStability
@Backing(type="int")
enum CruiseControlState {
    /**
     * This state is used as an alternative for any CruiseControlState value that is not defined in
     * the platform. Ideally, implementations of VehicleProperty#CRUISE_CONTROL_STATE should not use
     * this state. The framework can use this field to remain backwards compatible if
     * CruiseControlState is extended to include additional states.
     */
    OTHER = 0,
    /**
     * CC is enabled but the ADAS system is not actively controlling the vehicle's speed.
     */
    ENABLED = 1,
    /**
     * CC is enabled and activated, so the ADAS system is actively controlling the vehicle's speed.
     */
    ACTIVATED = 2,
    /**
     * Most CC implementations allow the driver to override CC. This means that the car has
     * determined it should maintain a certain speed and/or maintain a certain distance from a
     * leading vehicle, but the driver decides to take over and do something else. This is often
     * done for safety reasons and to ensure that the driver can always take control of the vehicle.
     * This state should be set when the user is actively overriding the CC system.
     */
    USER_OVERRIDE = 3,
    /**
     * Suspended state indicates CC is enabled and was activated, but now is suspended. This could
     * be caused by the user tapping the brakes while CC is ACTIVATED or the user using the
     * VehicleProperty#CRUISE_CONTROL_COMMAND to suspend CC. Once CC is suspended, the CC system
     * gives control of the vehicle back to the driver, but saves the target speed and/or target
     * time gap settings in case CC is resumed. This state can also be used when adaptive/predictive
     * CC slows to a stop and needs a user signal to start again.
     */
    SUSPENDED = 4,
    /**
     * When CC is in the ACTIVATED state but may potentially need to deactivate because of external
     * conditions (e.g. roads curvature is too extreme, the driver does not have their hands on the
     * steering wheel for a long period of time, or the driver is not paying attention), then the
     * ADAS system will notify the driver of a potential need to deactivate and give control back to
     * the driver.
     */
    FORCED_DEACTIVATION_WARNING = 5,
}
