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
 * Used to enumerate the state of Lane Centering Assist (LCA).
 */
@VintfStability
@Backing(type="int")
enum LaneCenteringAssistState {

    /**
     * This state is used as an alternative for any LaneCenteringAssistState value that is not
     * defined in the platform. Ideally, implementations of
     * VehicleProperty#LANE_CENTERING_ASSIST_STATE should not use this state. The framework
     * can use this field to remain backwards compatible if LaneCenteringAssistState is
     * extended to include additional states.
     */
    OTHER = 0,
    /**
     * LCA is enabled but the ADAS system has not received an activation signal from the driver.
     * Therefore, LCA is not steering the car and waits for the driver to send a
     * LaneCenteringAssistCommand#ACTIVATE command.
     */
    ENABLED = 1,
    /**
     * LCA is enabled and the driver has sent an activation command to the LCA system, but the
     * system has not started actively steering the vehicle. This may happen when LCA needs time to
     * detect valid lane lines. The activation command can be sent through the
     * VehicleProperty#LANE_CENTERING_ASSIST_COMMAND vehicle property or through a system external
     * to Android. Once LCA is actively steering the vehicle, the state must be updated to
     * ACTIVATED. If the feature is not able to activate, then the cause can be communicated through
     * the ErrorState values and then return to the ENABLED state.
     */
    ACTIVATION_REQUESTED = 2,
    /**
     * LCA is enabled and actively steering the car to keep it centered in its lane.
     */
    ACTIVATED = 3,
    /**
     * Many LCA implementations allow the driver to override LCA. This means that the car has
     * determined it should go a certain direction to keep the car centered in the lane, but a user
     * decides to take over and do something else. This is often done for safety reasons and to
     * ensure that the driver can always take control of the vehicle. This state should be set when
     * the user is actively overriding the LCA system.
     */
    USER_OVERRIDE = 4,
    /**
     * When LCA is in the ACTIVATED state but it will potentially need to deactivate because of
     * external conditions (e.g. roads curvature is too extreme, the driver does not have their
     * hands on the steering wheel for a long period of time, or the driver is not paying
     * attention), then the ADAS system will notify the driver of a potential need to deactivate and
     * give control back to the driver.
     */
    FORCED_DEACTIVATION_WARNING = 5,
}
