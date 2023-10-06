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
 * Used by lights state properties to enumerate the current state of the lights.
 *
 * Most XXX_LIGHTS_STATE properties will only report ON and OFF states.  Only
 * the HEADLIGHTS_STATE property will report DAYTIME_RUNNING.
 */
@VintfStability
@Backing(type="int")
enum VehicleLightState {
    OFF = 0,
    ON = 1,
    DAYTIME_RUNNING = 2,
}
