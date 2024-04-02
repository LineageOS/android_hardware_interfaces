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
@RustDerive(Clone=true)
parcelable VehiclePropConfig {
    /** Property identifier */
    int prop;

    /**
     * Defines if the property is read or write or both.
     *
     * If any VehicleAreaConfig.access is not set (i.e. VehicleAreaConfig.access ==
     * VehiclePropertyAccess.NONE) for this property, it will automatically be assumed that the
     * areaId access is the same as the VehiclePropConfig.access.
     *
     * VehiclePropConfig.access should be equal the maximal subset of the accesses set in its
     * areaConfigs, excluding those with access == VehiclePropertyAccess.NONE. For example, if a
     * VehiclePropConfig has some area configs with an access of VehiclePropertyAccess.READ and
     * others with an access of VehiclePropertyAccess.READ_WRITE, the VehiclePropConfig object's
     * access should be VehiclePropertyAccess.READ.
     *
     * In the scenario where the OEM actually wants to set VehicleAreaConfig.access =
     * VehiclePropertyAccess.NONE for a particular area config, the maximal subset rule should apply
     * with this area config included, making the VehiclePropConfig.access =
     * VehiclePropertyAccess.NONE.
     *
     * Currently we do not support scenarios where some areaIds are WRITE while others are
     * READ_WRITE. See the documentation for VehicleAreaConfig.access for more details.
     *
     * Examples:
     *   Suppose we have a property with two areaIds which we will call "LEFT" and "RIGHT". Here
     *   are some scenarios that can describe what the VehiclePropConfig.access value should be for
     *   this property.
     *   1. LEFT is READ and RIGHT is READ_WRITE. VehiclePropConfig.access must be READ as that is
     *      the maximal common access across all areaIds.
     *   2. LEFT is READ_WRITE and RIGHT is READ_WRITE. VehiclePropConfig.access must be READ_WRITE
     *      as that is the maximal common access across all areaIds.
     *   3. LEFT is WRITE and RIGHT is WRITE. VehiclePropConfig.access must be WRITE as that is the
     *      maximal common access across all areaIds.
     *   4. LEFT is READ_WRITE and RIGHT is not set (i.e. defaults to NONE)/is set to NONE, with the
     *      expectation that RIGHT should be populated with the default access mode of the property.
     *      VehiclePropConfig.access can be set to READ or READ_WRITE, whatever the OEM feels is the
     *      appropriate default access for the property.
     *   5. LEFT is READ and RIGHT is not set (i.e. defaults to NONE)/is set to NONE, with the
     *      expectation that RIGHT should be populated with the default access mode of the property.
     *      VehiclePropConfig.access must be set to READ because setting to READ_WRITE breaks the
     *      rule of having the global access being the maximal subset of the area config accesses.
     *      If the OEM wants RIGHT to be READ_WRITE in this scenario, the config should be rewritten
     *      such that LEFT is not set/is set to NONE and RIGHT is set to READ_WRITE with
     *      VehiclePropConfig.access set to READ.
     *   6. LEFT is READ_WRITE and RIGHT is set to NONE with the intention of RIGHT to specifically
     *      have no access. VehiclePropConfig.access must be NONE to support RIGHT maintaining its
     *      NONE access.
     *   7. LEFT is READ_WRITE and RIGHT is WRITE. This is unsupported behaviour and the config
     *      should not be defined this way.
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
