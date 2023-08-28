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
 * Used to enumerate the state of Lane Keep Assist (LKA).
 */
@VintfStability
@Backing(type="int")
enum LaneKeepAssistState {

    /**
     * This state is used as an alternative for any LaneKeepAssistState value that is not defined in
     * the platform. Ideally, implementations of VehicleProperty#LANE_KEEP_ASSIST_STATE should not
     * use this state. The framework can use this field to remain backwards compatible if
     * LaneKeepAssistState is extended to include additional states.
     */
    OTHER = 0,
    /**
     * LKA is enabled and monitoring, but steering assist is not activated.
     */
    ENABLED = 1,
    /**
     * LKA is enabled and currently has steering assist applied for the vehicle. Steering assist is
     * steering toward the left direction, which generally means the steering wheel turns counter
     * clockwise. This is usually in response to the vehicle drifting to the right. Once steering
     * assist is completed, LKA must return to the ENABLED state.
     */
    ACTIVATED_STEER_LEFT = 2,
    /**
     * LKA is enabled and currently has steering assist applied for the vehicle. Steering assist is
     * steering toward the right direction, which generally means the steering wheel turns
     * clockwise. This is usually in response to the vehicle drifting to the left. Once steering
     * assist is completed, LKA must return to the ENABLED state.
     */
    ACTIVATED_STEER_RIGHT = 3,
    /**
     * Many LKA implementations allow the driver to override LKA. This means that the car has
     * determined it should take some action, but a user decides to take over and do something else.
     * This is often done for safety reasons and to ensure that the driver can always take control
     * of the vehicle. This state should be set when the user is actively overriding the LKA system.
     */
    USER_OVERRIDE = 4,
}
