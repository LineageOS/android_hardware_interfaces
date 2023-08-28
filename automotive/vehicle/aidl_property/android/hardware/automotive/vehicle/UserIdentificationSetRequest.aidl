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

import android.hardware.automotive.vehicle.UserIdentificationSetAssociation;
import android.hardware.automotive.vehicle.UserInfo;

/**
 * Defines the format of a set() call to USER_IDENTIFICATION_ASSOCIATION.
 *
 * NOTE: this struct is not used in the HAL properties directly, it must be converted to
 * VehiclePropValue.RawValue through libraries provided by the default Vehicle HAL implementation.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable UserIdentificationSetRequest {
    /**
     * Id of the request being responded.
     */
    int requestId;
    /**
     * Information about the current foreground Android user.
     */
    UserInfo userInfo;
    /**
     * Number of association being set.
     */
    int numberAssociations;
    /**
     * Associations being set.
     */
    UserIdentificationSetAssociation[] associations;
}
