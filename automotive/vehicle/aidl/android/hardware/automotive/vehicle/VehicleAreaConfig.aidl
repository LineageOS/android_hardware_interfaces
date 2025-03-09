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

import android.hardware.automotive.vehicle.HasSupportedValueInfo;
import android.hardware.automotive.vehicle.VehiclePropertyAccess;

@VintfStability
@JavaDerive(equals=true, toString=true)
@RustDerive(Clone=true)
parcelable VehicleAreaConfig {
    /**
     * Area id is always 0 for VehicleArea#GLOBAL properties.
     */
    int areaId;

    /**
     * @deprecated client should use {@code getMinMaxSupportedValue} instead.
     *
     * Only applicable for {@code INT32} type property. Ignored for other types.
     *
     * The optional minimum value at boot time.
     *
     * For backward compatibility,
     * if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true},
     * and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true},
     * this must be equal to the min supported value
     * ({@code MinMaxSupportedValueResult.minSupportedValue}) at boot time.
     *
     * If no minimum or maximum value is available at boot time, both
     * {@code minInt32Value} and {@code maxInt32Value} must be set to 0.
     *
     * If either one is not 0, then we assume min and max both take effect.
     */
    int minInt32Value;

    /**
     * @deprecated client should use {@code getMinMaxSupportedValue} instead.
     *
     * Only applicable for {@code INT32} type property. Ignored for other types.
     *
     * The optional maximum value at boot time.
     *
     * For backward compatibility,
     * if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true},
     * and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true},
     * this must be equal to the max supported value
     * ({@code MinMaxSupportedValueResult.maxSupportedValue}) at boot time.
     *
     * If no minimum or maximum value is available at boot time, both
     * {@code minInt32Value} and {@code maxInt32Value} must be set to 0.
     *
     * If either one is not 0, then we assume min and max both take effect.
     */
    int maxInt32Value;

    /**
     * @deprecated client should use {@code getMinMaxSupportedValue} instead.
     *
     * Only applicable for {@code INT64} type property. Ignored for other types.
     *
     * The optional minimum value at boot time.
     *
     * For backward compatibility,
     * if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true},
     * and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true},
     * this must be equal to the min supported value
     * ({@code MinMaxSupportedValueResult.minSupportedValue}) at boot time.
     *
     * If no minimum or maximum value is available at boot time, both
     * {@code minInt64Value} and {@code maxInt64Value} must be set to 0.
     *
     * If either one is not 0, then we assume min and max both take effect.
     */
    long minInt64Value;

    /**
     * @deprecated client should use {@code getMinMaxSupportedValue} instead.
     *
     * Only applicable for {@code INT64} type property. Ignored for other types.
     *
     * The optional maximum value at boot time.
     *
     * For backward compatibility,
     * if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true},
     * and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true},
     * this must be equal to the max supported value
     * ({@code MinMaxSupportedValueResult.maxSupportedValue}) at boot time.
     *
     * If no minimum or maximum value is available at boot time, both
     * {@code minInt64Value} and {@code maxInt64Value} must be set to 0.
     *
     * If either one is not 0, then we assume min and max both take effect.
     */
    long maxInt64Value;

    /**
     * @deprecated client should use {@code getMinMaxSupportedValue} instead.
     *
     * Only applicable for {@code FLOAT} type property. Ignored for other types.
     *
     * The optional minimum value at boot time.
     *
     * For backward compatibility,
     * if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true},
     * and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true},
     * this must be equal to the min supported value
     * ({@code MinMaxSupportedValueResult.minSupportedValue}) at boot time.
     *
     * If no minimum or maximum value is available at boot time, both
     * {@code minFloatValue} and {@code maxFloatValue} must be set to 0.
     *
     * If either one is not 0, then we assume min and max both take effect.
     */
    float minFloatValue;

    /**
     * @deprecated client should use {@code getMinMaxSupportedValue} instead.
     *
     * Only applicable for {@code FLOAT} type property. Ignored for other types.
     *
     * The optional maximum value at boot time.
     *
     * For backward compatibility,
     * if {@code HasSupportedValueInfo.hasMinSupportedValue} is {@code true},
     * and {@code HasSupportedValueInfo.hasMaxSupportedValue} is {@code true},
     * this must be equal to the max supported value
     * ({@code MinMaxSupportedValueResult.maxSupportedValue}) at boot time.
     *
     * If no minimum or maximum value is available at boot time, both
     * {@code minFloatValue} and {@code maxFloatValue} must be set to 0.
     *
     * If either one is not 0, then we assume min and max both take effect.
     */
    float maxFloatValue;

    /**
     * @deprecated client should use {@code getMinMaxSupportedValue} instead.
     *
     * Only applicable for property with {@code @data_enum} annotation. Ignored
     * for other properties.
     *
     * Optional supported subset of supported values at boot time.
     *
     * If the property has a @data_enum and supportedEnumValues is {@code null},
     * then it is assumed all @data_enum values are supported unless specified
     * through another mechanism.
     *
     * For backward compatibility, if
     * {@code HasSupportedValueInfo.hasSupportedValuesList} is {@code true}
     * and this property has {@code data_enum} annotation,
     * this must be set to the same as
     * {@code SupportedValuesListResult.supportedValuesList} at boot time.
     */
    @nullable long[] supportedEnumValues;

    /**
     * Defines if the area ID for this property is READ, WRITE or READ_WRITE. This only applies if
     * the property is defined in the framework as a READ_WRITE property. Access (if set) should be
     * equal to, or a superset of, the VehiclePropConfig.access of the property. If access is not
     * set for this VehicleAreaConfig (i.e. access == VehiclePropertyAccess.NONE), then it will
     * automatically be assumed that the areaId access is the same as the VehiclePropConfig.access
     * of the property.
     *
     * For example, if a property is defined as READ_WRITE, but the OEM wants to specify certain
     * area Ids as READ-only, the corresponding areaIds should have an access set to READ, while the
     * others must be set to READ_WRITE. We do not support setting specific area Ids to WRITE-only
     * when the property is READ-WRITE. If any one area config has access
     * VehiclePropertyAccess::WRITE, then all VehicleAreaConfig.access values and
     * VehiclePropConfig.access must be set to WRITE for the property.
     *
     * VehiclePropConfig.access should be equal the maximal subset of the accesses set in
     * VehiclePropConfig.areaConfigs, excluding those with access == VehiclePropertyAccess.NONE. For
     * example, if a VehiclePropConfig has some area configs with an access of
     * VehiclePropertyAccess.READ and others with an access of VehiclePropertyAccess.READ_WRITE, the
     * VehiclePropConfig object's access should be VehiclePropertyAccess.READ.
     *
     * In the scenario where the OEM actually wants to set VehicleAreaConfig.access =
     * VehiclePropertyAccess.NONE, the maximal subset rule should apply with this area config
     * included, making the VehiclePropConfig.access = VehiclePropertyAccess.NONE.
     *
     * See VehiclePropConfig.access for more information.
     */
    VehiclePropertyAccess access = VehiclePropertyAccess.NONE;

    /**
     * Whether variable update rate is supported.
     *
     * This is always {@code false} for VHAL implementation < V3.
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

    /**
     * For VHAL implementation >= V4, this must not be {@code null}. This specifies whether
     * this property may have min/max supported value or supported values list.
     *
     * For VHAL implementation < V4, this is always {@code null} and is not accessed.
     */
    @nullable HasSupportedValueInfo hasSupportedValueInfo;
}
