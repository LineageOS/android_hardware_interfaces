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

import android.hardware.automotive.vehicle.InitialUserInfoResponseAction;
import android.hardware.automotive.vehicle.UserInfo;

/**
 * Defines the format of a HAL response to a INITIAL_USER_INFO request.
 *
 * NOTE: this struct is not used in the HAL properties directly, it must be converted to
 * VehiclePropValue.RawValue through libraries provided by the default Vehicle HAL implementation.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable InitialUserInfoResponse {
    /**
     * Id of the request being responded.
     */
    int requestId;
    /**
     * which action the Android system should take.
     */
    InitialUserInfoResponseAction action = InitialUserInfoResponseAction.DEFAULT;
    /**
     * Information about the user that should be switched to or created.
     */
    UserInfo userToSwitchOrCreate;
    /**
     * System locales of the initial user (value will be passed as-is to
     * android.provider.Settings.System.SYSTEM_LOCALES)
     */
    @utf8InCpp String userLocales;
    /**
     * Name of the user that should be created.
     */
    @utf8InCpp String userNameToCreate;
}
