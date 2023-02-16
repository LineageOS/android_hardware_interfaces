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
 * Used by emergency lane keep assist to enumerate state.
 *
 * This enum could be extended in future releases to include additional feature states.
 */
@VintfStability
@Backing(type="int")
enum EmergencyLaneKeepAssistState {
    /**
     * This state is used as an alternative for any EmergencyLaneKeepAssistState value that is not
     * defined in the platform. Ideally, implementations of
     * VehicleProperty#EMERGENCY_LANE_KEEP_ASSIST_STATE should not use this state. The framework can
     * use this field to remain backwards compatible if EmergencyLaneKeepAssistState is extended to
     * include additional states.
     */
    OTHER = 0,
    /**
     * ELKA is enabled and monitoring safety, but no safety event is detected and steering assist is
     * not activated.
     */
    ENABLED = 1,
    /**
     * ELKA is enabled and a safety event is detected. Vehicle is sending out a warning to the
     * driver indicating that there is a dangerous maneuver on the left side of the vehicle.
     */
    WARNING_LEFT = 2,
    /**
     * ELKA is enabled and a safety event is detected. Vehicle is sending out a warning to the
     * driver indicating that there is a dangerous maneuver on the right side of the vehicle.
     */
    WARNING_RIGHT = 3,
    /**
     * ELKA is enabled and currently has steering assist applied to the vehicle. Steering assist
     * nudges the vehicle towards the left, which generally means the steering wheel turns counter
     * clockwise. This is usually in response to the driver making an unsafe right lane change.
     */
    ACTIVATED_STEER_LEFT = 4,
    /**
     * ELKA is enabled and currently has steering assist applied to the vehicle. Steering assist
     * nudges the vehicle towards the right, which generally means the steering wheel turns
     * clockwise. This is usually in response to the driver making an unsafe left lane change.
     */
    ACTIVATED_STEER_RIGHT = 5,
    /**
     * Many safety feature implementations allow the driver to override said feature. This means
     * that the car has determined it should take some action, but a user decides to take over and
     * do something else. This is often done for safety reasons and to ensure that the driver can
     * always take control of the vehicle. This state should be set when the user is currently
     * overriding ELKA.
     */
    USER_OVERRIDE = 6,
}
