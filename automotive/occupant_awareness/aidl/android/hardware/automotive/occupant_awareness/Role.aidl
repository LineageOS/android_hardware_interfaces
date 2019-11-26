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
enum Role {
    /*
     * All valid role(s) must have at least 1 bit set.
     */
    INVALID = 0,
    /*
     * System could not determine role for this occupant.
     */
    UNKNOWN = 1 << 0,
    /*
     * Occupants that the system detects as front seat passengers.
     */
    FRONT_PASSENGER = 1 << 1,
    /*
     * Occupants that the system detects as driver(s).
     */
    DRIVER = 1 << 2,
    /*
     * Occupants on left seat of row 2.
     */
    ROW_2_PASSENGER_LEFT = 1 << 3,
    /*
     * Occupants on center seat of row 2.
     */
    ROW_2_PASSENGER_CENTER = 1 << 4,
    /*
     * Occupants on right seat of row 2.
     */
    ROW_2_PASSENGER_RIGHT = 1 << 5,
    /*
     * Occupants on left seat of row 3.
     */
    ROW_3_PASSENGER_LEFT = 1 << 6,
    /*
     * Occupants on center seat of row 3.
     */
    ROW_3_PASSENGER_CENTER = 1 << 7,
    /*
     * Occupants on right seat of row 3.
     */
    ROW_3_PASSENGER_RIGHT = 1 << 8,

    /*
     * Occupants that the system detects as front seat occupant.
     * FRONT_OCCUPANTS = DRIVER | FRONT_PASSENGER
     */
    FRONT_OCCUPANTS = 1 << 1 | 1 << 2,
    /*
     * Occupants of row 2.
     * ROW_2_OCCUPANTS = ROW_2_PASSENGER_LEFT | ROW_2_PASSENGER_CENTER | ROW_2_PASSENGER_RIGHT
     */
    ROW_2_OCCUPANTS = 1 << 3 | 1 << 4 | 1 << 5,
    /*
     * Occupants of row 3.
     * ROW_3_OCCUPANTS = ROW_3_PASSENGER_LEFT | ROW_3_PASSENGER_CENTER | ROW_3_PASSENGER_RIGHT
     */
    ROW_3_OCCUPANTS = 1 << 6 | 1 << 7 | 1 << 8,
    /*
     * All the occupants in the vehicle.
     * ALL_OCCUPANTS = UNKNOWN | FRONT_OCCUPANTS | ROW_2_OCCUPANTS | ROW_3_OCCUPANTS
     */
    ALL_OCCUPANTS = 0x1FF,
}
