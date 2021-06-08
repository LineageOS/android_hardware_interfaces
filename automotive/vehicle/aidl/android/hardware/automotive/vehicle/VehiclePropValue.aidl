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

import android.hardware.automotive.vehicle.VehiclePropertyStatus;

/**
 * Encapsulates the property name and the associated value. It
 * is used across various API calls to set values, get values or to register for
 * events.
 */
// @VintfStability
parcelable VehiclePropValue {
    /** Time is elapsed nanoseconds since boot */
    long timestamp;

    /**
     * Area type(s) for non-global property it must be one of the value from
     * VehicleArea* enums or 0 for global properties.
     */
    int areaId;

    /** Property identifier */
    int prop;

    /** Status of the property */
    VehiclePropertyStatus status;

    /**
     * This is used for properties of types VehiclePropertyType#INT
     * and VehiclePropertyType#INT_VEC
     */
    int[] int32Values;

    /**
     * This is used for properties of types VehiclePropertyType#FLOAT
     * and VehiclePropertyType#FLOAT_VEC
     */
    float[] floatValues;

    /** This is used for properties of type VehiclePropertyType#INT64 */
    long[] int64Values;

    /** This is used for properties of type VehiclePropertyType#BYTES */
    byte[] byteValues;

    /** This is used for properties of type VehiclePropertyType#STRING */
    @utf8InCpp String stringValue;
}
