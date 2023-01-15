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
 * Defines which action the Android system should take in an INITIAL_USER_INFO request.
 */
@VintfStability
@Backing(type="int")
enum InitialUserInfoResponseAction {
    /**
     * Let the Android System decide what to do.
     *
     * For example, it might create a new user on first boot, and switch to the last
     * active user afterwards.
     */
    DEFAULT = 0,
    /**
     * Switch to an existing Android user.
     */
    SWITCH = 1,
    /**
     * Create a new Android user (and switch to it).
     */
    CREATE = 2,
}
