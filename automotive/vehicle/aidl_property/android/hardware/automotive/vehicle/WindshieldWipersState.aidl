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
 * Used to enumerate the current state of VehicleProperty#WINDSHIELD_WIPERS_STATE.
 */
@VintfStability
@Backing(type="int")
enum WindshieldWipersState {

    /**
     * This state is used as an alternative for any WindshieldWipersState value that is not defined
     * in the platform. Ideally, implementations of VehicleProperty#WINDSHIELD_WIPERS_STATE should
     * not use this state. The framework can use this field to remain backwards compatible if
     * WindshieldWipersState is extended to include additional states.
     */
    OTHER = 0,
    /**
     * This state indicates the windshield wipers are currently off. If
     * VehicleProperty#WINDSHIELD_WIPERS_SWITCH is implemented, then it may be set to any of the
     * following modes: OFF or AUTO.
     */
    OFF = 1,
    /**
     * This state indicates the windshield wipers are currently on. If
     * VehicleProperty#WINDSHIELD_WIPERS_SWITCH is implemented, then it may be set to any of the
     * following modes: MIST, INTERMITTENT_LEVEL_*, CONTINUOUS_LEVEL_*, or AUTO.
     */
    ON = 2,
    /**
     * Windshield wipers are in the service mode.
     */
    SERVICE = 3,
}
