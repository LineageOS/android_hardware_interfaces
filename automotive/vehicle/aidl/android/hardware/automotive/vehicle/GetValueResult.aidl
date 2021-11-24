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

import android.hardware.automotive.vehicle.StatusCode;
import android.hardware.automotive.vehicle.VehiclePropValue;

@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable GetValueResult {
    // The ID for the request this response is for.
    long requestId;
    // The status for the getValue operation. If this is not OK, the getValue
    // fails and prop should be ignored.
    StatusCode status = StatusCode.OK;
    // Contains the property value if status is OK. Must be null if status is
    // not OK.
    @nullable VehiclePropValue prop;
}
