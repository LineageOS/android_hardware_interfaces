/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.ranging;

import android.hardware.bluetooth.ranging.Nadm;

/**
 * Generic structure to return the ranging result
 */
@VintfStability
parcelable RangingResult {
    /**
     * Estimated distance in meters.
     */
    double resultMeters;
    /**
     * Potential distance estimate error (plus or minus) in meters, always positive.
     */
    double errorMeters;
    /**
     * Azimuth Angle measurement in degrees.
     *
     * Azimuth of remote device in horizontal coordinate system, this measured from azimuth north
     * and increasing eastward. When the remote device in azimuth north, this angle is 0, when the
     * remote device in azimuth south, this angle is 180.
     *
     * See: <a href="https://en.wikipedia.org/wiki/Horizontal_coordinate_system">Horizontal
     *  coordinate system</a>for the details
     *
     * On an Android device, azimuth north is defined as the angle perpendicular away from the
     * back of the device when holding it in portrait mode upright.
     *
     * The Azimuth north is defined as the direction in which the top edge of the device is
     * facing when it is placed flat.
     *
     */
    double azimuthDegrees;
    /**
     * Estimated error (plus or minus) of azimuth angle measurement in degrees, always positive.
     */
    double errorAzimuthDegrees;
    /**
     * Altitude Angle measurement in degrees.
     *
     * Altitude of remote device in horizontal coordinate system, this is the angle between the
     * remote device and the top edge of local device. When local device is placed flat, the angle
     * of the zenith is 90, the angle of the nadir is -90.
     *
     * See: https://en.wikipedia.org/wiki/Horizontal_coordinate_system
     */
    double altitudeDegrees;
    /**
     * Estimated error (plus or minus) of altitude angle measurement in degrees, always positive.
     */
    double errorAltitudeDegrees;
    /**
     * Estimated delay spread in meters of the measured channel. This is a measure of multipath
     * richness of the channel.
     */
    double delaySpreadMeters;
    /**
     * A normalized value from 0 (low confidence) to 100 (high confidence) representing the
     * confidence of estimated distance.
     */
    byte confidenceLevel;
    /**
     * A value representing the chance of being attacked for the measurement.
     */
    Nadm detectedAttackLevel;
    /**
     * Estimated velocity, in the direction of line between two devices, of the moving object in
     * meters/sec.
     */
    double velocityMetersPerSecond;
    /**
     * Parameter for vendors to place vendor-specific ranging results data.
     */
    @nullable byte[] vendorSpecificCsRangingResultsData;
}
