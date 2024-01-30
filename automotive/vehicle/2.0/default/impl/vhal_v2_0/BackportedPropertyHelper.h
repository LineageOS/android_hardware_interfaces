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

// This file contains backported system property definitions and backported enums.

#pragma once

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {
namespace backportedproperty {

/**
 * Characterization of inputs used for computing location.
 *
 * This property must indicate what (if any) data and sensor inputs are considered by the system
 * when computing the vehicle's location that is shared with Android through the GNSS HAL.
 *
 * The value must return a collection of bit flags. The bit flags are defined in
 * LocationCharacterization. The value must also include exactly one of DEAD_RECKONED or
 * RAW_GNSS_ONLY among its collection of bit flags.
 *
 * When this property is not supported, it is assumed that no additional sensor inputs are fused
 * into the GNSS updates provided through the GNSS HAL. That is unless otherwise specified
 * through the GNSS HAL interfaces.
 *
 * @change_mode VehiclePropertyChangeMode.STATIC
 * @access VehiclePropertyAccess.READ
 */
constexpr int32_t LOCATION_CHARACTERIZATION = 0x31400C10;

/**
 * Used by LOCATION_CHARACTERIZATION to enumerate the supported bit flags.
 *
 * These flags are used to indicate to what transformations are performed on the
 * GNSS data before the location data is sent, so that location processing
 * algorithms can take into account prior fusion.
 *
 * This enum can be extended in future releases to include additional bit flags.
 */
enum class LocationCharacterization : int32_t {
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
};

}  // namespace backportedproperty
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
