/*
 * Copyright (C) 2022 The Android Open Source Project
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
 * Used by EV_STOPPING_MODE to enumerate the current state of the stopping mode.
 *
 * This enum may be extended to include more states in the future.
 */
@VintfStability
@Backing(type="int")
enum EvStoppingMode {
    /**
     * Other EV stopping mode. Ideally, this should never be used.
     */
    OTHER = 0,
    /**
     * Vehicle slowly moves forward when the brake pedal is released.
     */
    CREEP = 1,
    /**
     * Vehicle rolls freely when the brake pedal is released (similar to neutral gear).
     */
    ROLL = 2,
    /**
     * Vehicle stops and holds its position when the brake pedal is released.
     */
    HOLD = 3,
}
