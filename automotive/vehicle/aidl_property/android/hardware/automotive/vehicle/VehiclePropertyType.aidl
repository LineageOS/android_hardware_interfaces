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

/**
 * Enumerates supported data type for VehicleProperty.
 *
 * Used to create property ID in VehicleProperty enum.
 */
@VintfStability
@Backing(type="int")
enum VehiclePropertyType {
    STRING = 0x00100000,
    /**
     * Boolean values should be specified through the int32Values in RawPropValues.
     * int32Value = {0} represents false and int32Value = {1} represents true.
     */
    BOOLEAN = 0x00200000,
    INT32 = 0x00400000,
    INT32_VEC = 0x00410000,
    INT64 = 0x00500000,
    INT64_VEC = 0x00510000,
    FLOAT = 0x00600000,
    FLOAT_VEC = 0x00610000,
    BYTES = 0x00700000,

    /**
     * Any combination of scalar or vector types. The exact format must be
     * provided in the description of the property.
     *
     * For vendor MIXED type properties, configArray needs to be formatted in this
     * structure.
     * configArray[0], 1 indicates the property has a String value
     * configArray[1], 1 indicates the property has a Boolean value .
     * configArray[2], 1 indicates the property has an Integer value.
     * configArray[3], the number indicates the size of Integer[] in the property.
     * configArray[4], 1 indicates the property has a Long value.
     * configArray[5], the number indicates the size of Long[] in the property.
     * configArray[6], 1 indicates the property has a Float value.
     * configArray[7], the number indicates the size of Float[] in the property.
     * configArray[8], the number indicates the size of byte[] in the property.
     * For example:
     * {@code configArray = {1, 1, 1, 3, 0, 0, 0, 0, 0}} indicates the property has
     * a String value, a Boolean value, an Integer value and an array with 3 integers.
     */
    MIXED = 0x00e00000,

    MASK = 0x00ff0000,
}
