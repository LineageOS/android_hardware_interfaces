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

@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable RawPropValues {
    /**
     * This is used for properties of types VehiclePropertyType#INT32,
     * VehiclePropertyType#BOOLEAN and VehiclePropertyType#INT32_VEC
     */
    int[] int32Values = {};

    /**
     * This is used for properties of types VehiclePropertyType#FLOAT
     * and VehiclePropertyType#FLOAT_VEC
     */
    float[] floatValues;

    /**
     * This is used for properties of type VehiclePropertyType#INT64 and
     * VehiclePropertyType#INT64_VEC
     */
    long[] int64Values;

    /** This is used for properties of type VehiclePropertyType#BYTES */
    byte[] byteValues;

    /** This is used for properties of type VehiclePropertyType#STRING */
    @utf8InCpp String stringValue;
}
