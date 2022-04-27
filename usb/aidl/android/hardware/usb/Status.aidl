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

package android.hardware.usb;

@VintfStability
@Backing(type="int")
enum Status {
    SUCCESS = 0,
    /**
     * error value when the HAL operation fails for reasons not listed here.
     */
    ERROR = 1,
    /**
     * error value returned when input argument is invalid.
     */
    INVALID_ARGUMENT = 2,
    /**
     * error value returned when role string is unrecognized.
     */
    UNRECOGNIZED_ROLE = 3,
    /**
     * Error value returned when the operation is not supported.
     */
    NOT_SUPPORTED = 4,
}
