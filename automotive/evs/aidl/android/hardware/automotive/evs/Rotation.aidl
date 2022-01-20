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

package android.hardware.automotive.evs;

/**
 * Rotation:
 *
 * The required counterclockwise rotation of EVS camera stream and display.
 */
@VintfStability
@Backing(type="int")
enum Rotation {
    /** No rotation */
    ROTATION_0 = 0,
    /** Rotate by 90 degree counterclockwise */
    ROTATION_90 = 1,
    /** Rotate by 180 degree counterclockwise */
    ROTATION_180 = 2,
    /** Rotate by 270 degree counterclockwise */
    ROTATION_270 = 3

}
