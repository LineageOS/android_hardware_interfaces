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

@VintfStability
@Backing(type="int")
enum VehicleDisplay {
    /**
     * The primary Android display (for example, center console)
     */
    MAIN = 0,
    /**
     * Instrument cluster display. This may exist only for driver
     */
    INSTRUMENT_CLUSTER = 1,

    /**
     * Head Up Display. This may exist only for driver
     */
    HUD = 2,
    /**
     * Dedicated display for showing IME for {@code MAIN}
     */
    INPUT = 3,
    /**
     * Auxiliary display which can provide additional screen for {@code MAIN} display
     */
    AUXILIARY = 4,
}
