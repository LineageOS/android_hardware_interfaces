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
}
