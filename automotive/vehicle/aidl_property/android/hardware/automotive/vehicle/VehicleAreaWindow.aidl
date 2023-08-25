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
 * Various windshields/windows in the car.
 */
@VintfStability
@Backing(type="int")
enum VehicleAreaWindow {
    FRONT_WINDSHIELD = 0x00000001,
    REAR_WINDSHIELD = 0x00000002,
    ROW_1_LEFT = 0x00000010,
    ROW_1_RIGHT = 0x00000040,
    ROW_2_LEFT = 0x00000100,
    ROW_2_RIGHT = 0x00000400,
    ROW_3_LEFT = 0x00001000,
    ROW_3_RIGHT = 0x00004000,
    ROOF_TOP_1 = 0x00010000,
    ROOF_TOP_2 = 0x00020000,
}
