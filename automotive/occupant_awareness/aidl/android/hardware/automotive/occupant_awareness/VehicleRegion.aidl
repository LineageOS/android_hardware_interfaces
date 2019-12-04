/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hardware.automotive.occupant_awareness;

@VintfStability
@Backing(type="int")
enum VehicleRegion {
    /*
     * List of targets in the car.
     */
    UNKNOWN = 0,
    INSTRUMENT_CLUSTER = 1,
    REAR_VIEW_MIRROR = 2,
    LEFT_SIDE_MIRROR = 3,
    RIGHT_SIDE_MIRROR = 4,
    FORWARD_ROADWAY = 5,
    LEFT_ROADWAY = 6,
    RIGHT_ROADWAY = 7,
    HEAD_UNIT_DISPLAY = 8,
    /*
     * Vendors can use this value along with customGazeTarget string to uniquely identify their
     * custom region.
     */
    CUSTOM_TARGET = 200,
}
