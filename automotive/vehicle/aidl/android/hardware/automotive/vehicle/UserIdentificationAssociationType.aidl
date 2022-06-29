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
 * Types of mechanisms used to identify an Android user.
 *
 * See USER_IDENTIFICATION_ASSOCIATION for more details and example.
 */
@VintfStability
@Backing(type="int")
enum UserIdentificationAssociationType {
    INVALID = 0,
    /**
     * Key used to unlock the car.
     */
    KEY_FOB = 1,
    /**
     * Custom mechanism defined by the OEM.
     */
    CUSTOM_1 = 101,
    /**
     * Custom mechanism defined by the OEM.
     */
    CUSTOM_2 = 102,
    /**
     * Custom mechanism defined by the OEM.
     */
    CUSTOM_3 = 103,
    /**
     * Custom mechanism defined by the OEM.
     */
    CUSTOM_4 = 104,
}
