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

@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable VehiclePropConfig {
    /** Property identifier */
    int prop;

    /**
     * Defines if the property is read or write or both.
     *
     * If populating VehicleAreaConfig.access fields for this property, this field should not be
     * populated. If the OEM decides to populate this field, none of the VehicleAreaConfig.access
     * fields should be populated.
     */
    VehiclePropertyAccess access = VehiclePropertyAccess.NONE;

    /**
     * Defines the change mode of the property.
     */
    VehiclePropertyChangeMode changeMode = VehiclePropertyChangeMode.STATIC;

    /**
     * Contains per-areaId configuration.
     *
     * [Definition] area: An area represents a unique element of a VehicleArea. For instance, if the
     *   VehicleArea is WINDOW, then an example area is FRONT_WINDSHIELD.
     *
     * [Definition] area ID: An area ID is a combination of one or more areas, and is created by
     *   bitwise "OR"ing the areas together. Areas from different VehicleArea values may not be
     *   mixed in a single area ID. For example, a VehicleAreaWindow area cannot be combined with a
     *   VehicleAreaSeat area in an area ID.
     *
     * For VehicleArea#GLOBAL properties, they must map only to a single area ID of 0.
     *
     * Rules for mapping a non VehicleArea#GLOBAL property to area IDs:
     *  - A property must be mapped to a set of area IDs that are impacted when the property value
     *    changes.
     *  - An area cannot be part of multiple area IDs, it must only be part of a single area ID.
     *  - When the property value changes in one of the areas in an area ID, then it must
     *    automatically change in all other areas in the area ID.
     *  - The property value must be independently controllable in any two different area IDs.
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
