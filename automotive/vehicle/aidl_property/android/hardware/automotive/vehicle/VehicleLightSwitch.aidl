/*
 * Copyright (C) 2021 The Android Open Source Project
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
 * Used by lights switch properties to enumerate user selected switch setting.
 *
 * XXX_LIGHTS_SWITCH properties report the switch settings that the user
 * selects.  The switch setting may be decoupled from the state reported if the
 * user selects AUTOMATIC.
 */
@VintfStability
@Backing(type="int")
enum VehicleLightSwitch {
    OFF = 0,
    ON = 1,
    /**
     * Daytime running lights mode.  Most cars automatically use DRL but some
     * cars allow the user to activate them manually.
     */
    DAYTIME_RUNNING = 2,
    /**
     * Allows the vehicle ECU to set the lights automatically
     */
    AUTOMATIC = 0x100,
}
