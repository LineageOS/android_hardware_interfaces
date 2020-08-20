/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.powerstats;

@VintfStability
@Backing(type="int")
enum PowerEntityType {
    /**
     * A subsystem is a self-contained compute unit. Some examples include
     * application processor, DSP, GPU.
     */
    SUBSYSTEM = 0,
    /**
     * A peripheral is an auxiliary device that connects to and works with a
     * compute unit. Some examples include simple sensors, camera, display.
     */
    PERIPHERAL = 1,
    /**
     * A power domain is a single subsystem or a collection of subsystems
     * that is controlled by a single voltage rail.
     */
    POWER_DOMAIN = 2,
}