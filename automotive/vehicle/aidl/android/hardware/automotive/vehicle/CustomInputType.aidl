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
 * Input code values for HW_CUSTOM_INPUT.
 */
@VintfStability
@Backing(type="int")
enum CustomInputType {
    /**
     * Ten optional functions to be used in case OEM don't need more than 10 input code values.
     *
     * OEMs are free to use any signed 32 bits number to represent the input code value.
     * The following function keys are only for convenience and any other integer values are
     * also allowed.
     */
    CUSTOM_EVENT_F1 = 1001,
    CUSTOM_EVENT_F2 = 1002,
    CUSTOM_EVENT_F3 = 1003,
    CUSTOM_EVENT_F4 = 1004,
    CUSTOM_EVENT_F5 = 1005,
    CUSTOM_EVENT_F6 = 1006,
    CUSTOM_EVENT_F7 = 1007,
    CUSTOM_EVENT_F8 = 1008,
    CUSTOM_EVENT_F9 = 1009,
    CUSTOM_EVENT_F10 = 1010,
}
