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
 * Bit flags for fan direction
 */
@VintfStability
@Backing(type="int")
enum VehicleHvacFanDirection {
    UNKNOWN = 0x0,
    FACE = 0x1,
    FLOOR = 0x2,
    /**
     * FACE_AND_FLOOR = FACE | FLOOR
     */
    FACE_AND_FLOOR = 0x3,
    /**
     * DEFROST may also be described as the windshield fan direction.
     */
    DEFROST = 0x4,
    /**
     * DEFROST_AND_FLOOR = DEFROST | FLOOR
     */
    DEFROST_AND_FLOOR = 0x06,
}
