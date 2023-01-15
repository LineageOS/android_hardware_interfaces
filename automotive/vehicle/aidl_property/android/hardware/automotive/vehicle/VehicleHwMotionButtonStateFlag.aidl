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
 * See {@code android.view.MotionEvent#getButtonState()} for more details.
 */
@VintfStability
@Backing(type="int")
enum VehicleHwMotionButtonStateFlag {
    /**
     * Button state primary
     */
    BUTTON_PRIMARY = 0x0001,
    /**
     * Button state secondary
     */
    BUTTON_SECONDARY = 0x0002,
    /**
     * Button state tertiary
     */
    BUTTON_TERTIARY = 0x0004,
    /**
     * Button state back
     */
    BUTTON_BACK = 0x0008,
    /**
     * Button state forward
     */
    BUTTON_FORWARD = 0x0010,
    /**
     * Button state stylus primary
     */
    BUTTON_STYLUS_PRIMARY = 0x0020,
    /**
     * Button state stylus secondary
     */
    BUTTON_STYLUS_SECONDARY = 0x0040,
}
