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
 * See {@code android.view.InputDevice#SOURCE_*} for more details.
 */
@VintfStability
@Backing(type="int")
enum VehicleHwMotionInputSource {
    /**
     * Unknown source
     */
    SOURCE_UNKNOWN = 0,
    /**
     * Source keyboard
     */
    SOURCE_KEYBOARD = 1,
    /**
     * Source dpad
     */
    SOURCE_DPAD = 2,
    /**
     * Source game pad
     */
    SOURCE_GAMEPAD = 3,
    /**
     * Source touch screen
     */
    SOURCE_TOUCHSCREEN = 4,
    /**
     * Source mouse
     */
    SOURCE_MOUSE = 5,
    /**
     * Source stylus
     */
    SOURCE_STYLUS = 6,
    /**
     * Source bluetooth stylus
     */
    SOURCE_BLUETOOTH_STYLUS = 7,
    /**
     * Source trackball
     */
    SOURCE_TRACKBALL = 8,
    /**
     * Source mouse relative
     */
    SOURCE_MOUSE_RELATIVE = 9,
    /**
     * Source touchpad
     */
    SOURCE_TOUCHPAD = 10,
    /**
     * Source touch navigation
     */
    SOURCE_TOUCH_NAVIGATION = 11,
    /**
     * Source rotary encoder
     */
    SOURCE_ROTARY_ENCODER = 12,
    /**
     * Source joystick
     */
    SOURCE_JOYSTICK = 13,
    /**
     * Source hdmi
     */
    SOURCE_HDMI = 14,
    /**
     * Source sensor
     */
    SOURCE_SENSOR = 15,
}
