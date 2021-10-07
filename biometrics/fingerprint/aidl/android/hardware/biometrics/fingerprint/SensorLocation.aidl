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

package android.hardware.biometrics.fingerprint;

@VintfStability
parcelable SensorLocation {
    /**
     * The display to which the following measurements are relative to. This must correspond to the
     * android.hardware.DisplayManager#getDisplay Android API.
     *
     * A few examples:
     *   1) A capacitive rear fingerprint sensor would specify the display to which it is behind.
     *   2) An under-display fingerprint sensor would specify the display on which the sensor is
     *      located.
     *   3) A foldable device would specify multiple locations and have a SensorLocation entry
     *      for each display from which the sensor is accessible from.
     */
    int displayId;

    /**
     * The location of the center of the sensor if applicable. For example, sensors of
     * FingerprintSensorType::UNDER_DISPLAY_* would report this value as the distance in pixels,
     * measured from the left edge of the screen.
     */
    int sensorLocationX;

    /**
     * The location of the center of the sensor if applicable. For example, sensors of
     * FingerprintSensorType::UNDER_DISPLAY_* would report this value as the distance in pixels,
     * measured from the top edge of the screen.
     */
    int sensorLocationY;

    /**
     * The radius of the sensor if applicable. For example, sensors of
     * FingerprintSensorType::UNDER_DISPLAY_* would report this value as the radius of the sensor,
     * in pixels.
     */
    int sensorRadius;
}
