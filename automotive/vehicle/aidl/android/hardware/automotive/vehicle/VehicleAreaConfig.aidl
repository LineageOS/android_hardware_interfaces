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

import android.hardware.automotive.vehicle.VehiclePropertyAccess;

@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable VehicleAreaConfig {
    /**
     * Area id is always 0 for VehicleArea#GLOBAL properties.
     */
    int areaId;

    /**
     * If the property has @data_enum, leave the range to zero.
     *
     * Range will be ignored in the following cases:
     *    - The VehiclePropertyType is not INT32, INT64 or FLOAT.
     *    - Both of min value and max value are zero.
     */

    int minInt32Value;
    int maxInt32Value;

    long minInt64Value;
    long maxInt64Value;

    float minFloatValue;
    float maxFloatValue;

    /**
     * If the property has a @data_enum, then it is possible to specify a supported subset of the
     * @data_enum. If the property has a @data_enum and supportedEnumValues is null, then it is
     * assumed all @data_enum values are supported unless specified through another mechanism.
     */
    @nullable long[] supportedEnumValues;

    /**
     * Defines if the area ID for this property is READ, WRITE or READ_WRITE. This only applies if
     * the property is defined in the framework as a READ_WRITE property. Access (if set) should be
     * equal to, or a superset of, the VehiclePropConfig.access of the property.
     *
     * For example, if a property is defined as READ_WRITE, but the OEM wants to specify certain
     * area Ids as READ-only, the corresponding areaIds should have an access set to READ, while the
     * others must be set to READ_WRITE. We do not support setting specific area Ids to WRITE-only
     * when the property is READ-WRITE.
     *
     * Exclusively one of VehiclePropConfig and the VehicleAreaConfigs should be specified for a
     * single property. If VehiclePropConfig.access is populated, none of the
     * VehicleAreaConfig.access values should be populated. If VehicleAreaConfig.access values are
     * populated, VehiclePropConfig.access must not be populated.
     *
     * VehicleAreaConfigs should not be partially populated with access. If the OEM wants to specify
     * access for one area Id, all other configs should be populated with their access levels as
     * well.
     */
    VehiclePropertyAccess access = VehiclePropertyAccess.NONE;

    /**
     * Whether variable update rate is supported.
     *
     * This applies for continuous property only.
     *
     * It is HIGHLY RECOMMENDED to support variable update rate for all non-heartbeat continuous
     * properties for better performance unless the property is large.
     *
     * If variable update rate is supported and 'enableVariableUpdateRate' is true in subscribe
     * options, VHAL must only sends property update event when the property's value changes
     * (a.k.a treat continuous as an on-change property).
     *
     * E.g. if the client is subscribing at 5hz at time 0. If the property's value is 0 initially
     * and becomes 1 after 1 second.

     * If variable update rate is not enabled, VHAL clients will receive 5 property change events
     * with value 0 and 5 events with value 1 after 2 seconds.
     *
     * If variable update rate is enabled, VHAL clients will receive 1 property change event
     * with value 1 at time 1s. VHAL may/may not send a property event for the initial value (e.g.
     * a property change event with value 0 at time 0s). VHAL client must not rely on the first
     * property event, and must use getValues to fetch the initial value. In fact, car service is
     * using getValues to fetch the initial value, convert it to a property event and deliver to
     * car service clients.
     *
     * NOTE: If this is true, car service may cache the property update event for filtering purpose,
     * so this should be false if the property is large (e.g. a byte array of 1k in size).
     */
    boolean supportVariableUpdateRate;
}
