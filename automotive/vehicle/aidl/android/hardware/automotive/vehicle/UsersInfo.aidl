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

import android.hardware.automotive.vehicle.UserInfo;

/**
 * Information about all Android users.
 *
 * NOTE: this struct is not used in the HAL properties directly, it's part of other structs, which
 * in turn are converted to a VehiclePropValue.RawValue through libraries provided by the default
 * Vehicle HAL implementation.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable UsersInfo {
    /**
     * The current foreground user.
     */
    UserInfo currentUser;
    /**
     * Number of existing users; includes the current user, recently removed users (with DISABLED
     * flag), and profile users (with PROFILE flag).
     */
    int numberUsers;
    /**
     * List of existing users; includes the current user, recently removed users (with DISABLED
     * flag), and profile users (with PROFILE flag).
     */
    UserInfo[] existingUsers;
}
