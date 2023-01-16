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
 * Defines when a INITIAL_USER_INFO request was made.
 */
@VintfStability
@Backing(type="int")
enum InitialUserInfoRequestType {
    UNKNOWN = 0,
    /**
     * At the first time Android was booted (or after a factory reset).
     */
    FIRST_BOOT = 1,
    /**
     * At the first time Android was booted after the system was updated.
     */
    FIRST_BOOT_AFTER_OTA = 2,
    /**
     * When Android was booted "from scratch".
     */
    COLD_BOOT = 3,
    /**
     * When Android was resumed after the system was suspended to memory.
     */
    RESUME = 4,
}
