/*
 * Copyright (C) 2024 The Android Open Source Project
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
 * Whether the [propId, areaId] has min/max supported value or supported values
 * list specified.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
@RustDerive(Clone=true)
parcelable HasSupportedValueInfo {
    /**
     * Whether [propId, areaId] has min supported value specified.
     *
     * If this is {@code true}, the hardware specifies a min supported value.
     * If {@code MinMaxSupportedValueResult}'s {@code status} is
     * {@code StatusCode.OK}, its {@code minSupportedValue} must not be
     * {@code null}.
     *
     * If this is {@code false}, {@code minSupportedValue} must be {@code null}.
     *
     * Unless otherwise specified, this field is set to {@code false} for any
     * properties whose type is not int32, int64 or float.
     *
     * For certain properties, e.g. {@code EV_BRAKE_REGENERATION_LEVEL}, this
     * must always be {@code true}. Check {@code VehicleProperty}
     * documentation.
     */
    boolean hasMinSupportedValue;

    /**
     * Whether [propId, areaId] has max supported value specified.
     *
     * If this is {@code true}, the hardware specifies a max supported value.
     * If {@code MinMaxSupportedValueResult}'s {@code status} is
     * {@code StatusCode.OK}, its {@code maxSupportedValue} must not be
     * {@code null}.
     *
     * If this is {@code false}, {@code maxSupportedValue} must be {@code null}.
     *
     * Unless otherwise specified, this field is set to {@code false} for any
     * properties whose type is not int32, int64 or float.
     *
     * For certain properties, e.g. {@code EV_BRAKE_REGENERATION_LEVEL}, this
     * must always be {@code true}. Check {@code VehicleProperty}
     * documentation.
     */
    boolean hasMaxSupportedValue;

    /**
     * Whether [propId, areaId] has supported values list specified.
     *
     * If this is {@code true}, it means the hardware specifies supported
     * values for this property.
     * If {@code SupportedValueListResult}'s {@code status} is
     * {@code StatusCode.OK}, its {@code supportedValuesList} must not be
     * {@code null}.
     *
     * If this is {@code false}, {@code supportedValuesList} must always be
     * {@code null}.
     *
     * The supported value is the superset for both the input value for writable
     * property and the output value for readable property.
     *
     * For certain properties, e.g. {@code GEAR_SELECTION}, this must always be
     * {@code true}. Check {@code VehicleProperty} documentation.
     */
    boolean hasSupportedValuesList;
}
