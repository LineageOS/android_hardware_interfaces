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

import android.hardware.automotive.vehicle.RawPropValues;
import android.hardware.automotive.vehicle.StatusCode;

/**
 * One result returned from {@code getSupportedValuesLists} for one request.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
@RustDerive(Clone=true)
parcelable SupportedValuesListResult {
    /**
     * The status for result. If this is not OK, the operation failed for this
     * [propId, areaId].
     */
    StatusCode status = StatusCode.OK;
    /**
     * The supported values list.
     *
     * If the [propId, areaId] does not specify a supported values list, this
     * is {@code null}.
     */
    @nullable List<RawPropValues> supportedValuesList;
}
