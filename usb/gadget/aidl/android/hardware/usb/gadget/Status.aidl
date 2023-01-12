/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.usb.gadget;

@VintfStability
@Backing(type="int")
enum Status {
    SUCCESS = 0,
    /**
     * Error value when the HAL operation fails for reasons not listed here.
     */
    ERROR = 1,
    /**
     * USB configuration applied successfully.
     */
    FUNCTIONS_APPLIED = 2,
    /**
     * USB confgiuration failed to apply.
     */
    FUNCTIONS_NOT_APPLIED = 3,
    /**
     * USB configuration not supported.
     */
    CONFIGURATION_NOT_SUPPORTED = 4,
}
