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
 * Information about a specific Android user.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable UserInfo {
    /**
     * System user.
     *
     * On automotive, that user is always running, although never on foreground (except during
     * boot or exceptional circumstances).
     */
    const int USER_FLAG_SYSTEM = 0x01;
    /**
     * Guest users have restrictions.
     */
    const int USER_FLAG_GUEST = 0x02;
    /**
     * Ephemeral users have non-persistent state.
     */
    const int USER_FLAG_EPHEMERAL = 0x04;
    /**
     * Admin users have additional privileges such as permission to create other users.
     */
    const int USER_FLAG_ADMIN = 0x08;
    /**
     * Disabled users are marked for deletion.
     */
    const int USER_FLAG_DISABLED = 0x10;
    /**
     * Profile user is a profile of another user.
     */
    const int USER_FLAG_PROFILE = 0x20;
    /*
     * The user ID.
     */
    int userId = 0;
    /*
     * Bitmask for the user flags defined above (USER_FLAG_*).
     */
    int flags;
}
