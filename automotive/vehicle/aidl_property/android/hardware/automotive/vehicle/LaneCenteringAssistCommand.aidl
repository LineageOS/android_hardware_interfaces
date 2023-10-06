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
 * Used by Lane Centering Assist (LCA) to enumerate commands.
 */
@VintfStability
@Backing(type="int")
enum LaneCenteringAssistCommand {
    /**
     * When VehicleProperty#LANE_CENTERING_ASSIST_STATE = LaneCenteringAssistState#ENABLED, this
     * command sends a request to activate steering control that keeps the vehicle centered in its
     * lane. While waiting for the LCA System to take control of the vehicle,
     * VehicleProperty#LANE_CENTERING_ASSIST_STATE must be in the
     * LaneCenteringAssistState#ACTIVATION_REQUESTED state. Once the vehicle takes control of
     * steering, then VehicleProperty#LANE_CENTERING_ASSIST_STATE must be in the
     * LaneCenteringAssistState#ACTIVATED state. Otherwise, an error can be communicated through an
     * ErrorState value.
     */
    ACTIVATE = 1,
    /**
     * When VehicleProperty#LANE_CENTERING_ASSIST_STATE is set to
     * LaneCenteringAssistState#ACTIVATION_REQUESTED or LaneCenteringAssistState#ACTIVATED, this
     * command deactivates steering control and the driver should take full control of the vehicle.
     * If this command succeeds, VehicleProperty#LANE_CENTERING_ASSIST_STATE must be updated to
     * LaneCenteringAssistState#ENABLED.
     */
    DEACTIVATE = 2,
}
