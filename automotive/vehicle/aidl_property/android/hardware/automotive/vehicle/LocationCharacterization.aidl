/*
 * Copyright (C) 2023 The Android Open Source Project
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
 * Used by LOCATION_CHARACTERIZATION to enumerate the supported bit flags.
 *
 * These flags are used to indicate to what transformations are performed on the
 * GNSS data before the location data is sent, so that location processing
 * algorithms can take into account prior fusion.
 *
 * This enum can be extended in future releases to include additional bit flags.
 */
@VintfStability
@Backing(type="int")
enum LocationCharacterization {
    /**
     * Prior location samples have been used to refine the raw GNSS data (e.g. a
     * Kalman Filter).
     */
    PRIOR_LOCATIONS = 0x1,
    /**
     * Gyroscope data has been used to refine the raw GNSS data.
     */
    GYROSCOPE_FUSION = 0x2,
    /**
     * Accelerometer data has been used to refine the raw GNSS data.
     */
    ACCELEROMETER_FUSION = 0x4,
    /**
     * Compass data has been used to refine the raw GNSS data.
     */
    COMPASS_FUSION = 0x8,
    /**
     * Wheel speed has been used to refine the raw GNSS data.
     */
    WHEEL_SPEED_FUSION = 0x10,
    /**
     * Steering angle has been used to refine the raw GNSS data.
     */
    STEERING_ANGLE_FUSION = 0x20,
    /**
     * Car speed has been used to refine the raw GNSS data.
     */
    CAR_SPEED_FUSION = 0x40,
    /**
     * Some effort is made to dead-reckon location. In particular, this means that
     * relative changes in location have meaning when no GNSS satellite is
     * available.
     */
    DEAD_RECKONED = 0x80,
    /**
     * Location is based on GNSS satellite signals without sufficient fusion of
     * other sensors for complete dead reckoning. This flag should be set when
     * relative changes to location cannot be relied on when no GNSS satellite is
     * available.
     */
    RAW_GNSS_ONLY = 0x100,
}
