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

package android.hardware.health.storage;

/**
 * Status values for HAL methods.
 */
@VintfStability
@Backing(type="int")
enum Result {
    /**
     * Execution of the method is successful.
     */
    SUCCESS = 0,
    /**
     * An IO error is encountered when the HAL communicates with the device.
     */
    IO_ERROR,
    /**
     * An unknown error is encountered.
     */
    UNKNOWN_ERROR,
}
