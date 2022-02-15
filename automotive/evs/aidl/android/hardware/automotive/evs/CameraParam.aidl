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
 * EVS camera parameter
 */
@VintfStability
@Backing(type="int")
enum CameraParam {
    /**
     * The brightness of image frames
     */
    BRIGHTNESS,
    /**
     * The contrast of image frames
     */
    CONTRAST,
    /**
     * Automatic gain/exposure control
     */
    AUTOGAIN,
    /**
     * Gain control
     */
    GAIN,
    /**
     * Automatic Whitebalance
     */
    AUTO_WHITE_BALANCE,
    /**
     * Manual white balance setting as a color temperature in Kelvin.
     */
    WHITE_BALANCE_TEMPERATURE,
    /**
     * Image sharpness adjustment
     */
    SHARPNESS,
    /**
     * Auto Exposure Control modes; auto, manual, shutter priority, or
     * aperture priority.
     */
    AUTO_EXPOSURE,
    /**
     * Manual exposure time of the camera
     */
    ABSOLUTE_EXPOSURE,
    /**
     * Sets the focal point of the camera to the specified position.  This
     * parameter may not be effective when auto focus is enabled.
     */
    ABSOLUTE_FOCUS,
    /**
     * Enables continuous automatic focus adjustments.
     */
    AUTO_FOCUS,
    /**
     * Specifies the objective lens focal length as an absolute value.
     */
    ABSOLUTE_ZOOM,
}
