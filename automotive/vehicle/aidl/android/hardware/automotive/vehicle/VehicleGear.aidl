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
 * Various gears which can be selected by user and chosen in system.
 */
@VintfStability
@Backing(type="int")
enum VehicleGear {
    GEAR_UNKNOWN = 0x0000,
    GEAR_NEUTRAL = 0x0001,
    GEAR_REVERSE = 0x0002,
    GEAR_PARK = 0x0004,
    GEAR_DRIVE = 0x0008,
    GEAR_1 = 0x0010,
    GEAR_2 = 0x0020,
    GEAR_3 = 0x0040,
    GEAR_4 = 0x0080,
    GEAR_5 = 0x0100,
    GEAR_6 = 0x0200,
    GEAR_7 = 0x0400,
    GEAR_8 = 0x0800,
    GEAR_9 = 0x1000,
}
