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
 * Used by EV charging properties to enumerate the current state of the battery charging.
 */
@VintfStability
@Backing(type="int")
enum EvChargeState {
    UNKNOWN = 0,
    CHARGING = 1,
    FULLY_CHARGED = 2,
    NOT_CHARGING = 3,
    /**
     * Vehicle not charging due to an error
     */
    ERROR = 4,
}
