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
enum VehicleArea {
    GLOBAL = 0x01000000,
    /** WINDOW maps to enum VehicleAreaWindow */
    WINDOW = 0x03000000,
    /** MIRROR maps to enum VehicleAreaMirror */
    MIRROR = 0x04000000,
    /** SEAT maps to enum VehicleAreaSeat */
    SEAT = 0x05000000,
    /** DOOR maps to enum VehicleAreaDoor */
    DOOR = 0x06000000,
    /** WHEEL maps to enum VehicleAreaWheel */
    WHEEL = 0x07000000,

    MASK = 0x0f000000,
}
