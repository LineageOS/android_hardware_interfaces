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

import android.hardware.automotive.vehicle.VehicleAreaConfig;
import android.hardware.automotive.vehicle.VehiclePropertyAccess;
import android.hardware.automotive.vehicle.VehiclePropertyChangeMode;

// @VintfStability
parcelable VehiclePropConfig {
    /** Property identifier */
    int prop;

    /**
     * Defines if the property is read or write or both.
     */
    VehiclePropertyAccess access = VehiclePropertyAccess.NONE;

    /**
     * Defines the change mode of the property.
     */
    VehiclePropertyChangeMode changeMode = VehiclePropertyChangeMode.STATIC;

    /**
     * Contains per-area configuration.
     */
    VehicleAreaConfig[] areaConfigs;

    /** Contains additional configuration parameters */
    int[] configArray;

    /**
     * Some properties may require additional information passed over this
     * string. Most properties do not need to set this.
     */
    @utf8InCpp String configString;

    /**
     * Min sample rate in Hz.
     * Must be defined for VehiclePropertyChangeMode::CONTINUOUS
     */
    float minSampleRate;

    /**
     * Must be defined for VehiclePropertyChangeMode::CONTINUOUS
     * Max sample rate in Hz.
     */
    float maxSampleRate;
}
