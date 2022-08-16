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

import android.hardware.automotive.vehicle.SwitchUserMessageType;
import android.hardware.automotive.vehicle.SwitchUserStatus;

/**
 * Defines the result of a SwitchUserRequest.
 *
 * NOTE: this struct is not used in the HAL properties directly, it must be converted to
 * VehiclePropValue.RawValue through libraries provided by the default Vehicle HAL implementation.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable SwitchUserResponse {
    /**
     * Id of the request being responded.
     */
    int requestId;
    /**
     * Type of message.
     */
    SwitchUserMessageType messageType = SwitchUserMessageType.UNKNOWN;
    /**
     * Status of the request.
     */
    SwitchUserStatus status = SwitchUserStatus.SUCCESS;
    /**
     * HAL-specific error message.
     *
     * This argument is optional, and when defined, it's passed "as-is" to the caller. It could be
     * used to show custom error messages to the end user.
     */
    @utf8InCpp String errorMessage;
}
