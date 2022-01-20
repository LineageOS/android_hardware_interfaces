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

package android.hardware.automotive.evs;

/**
 * Error codes used in EVS HAL interface.
 */
@VintfStability
@Backing(type="int")
enum EvsResult {
    OK = 0,
    /**
     * Given arguments are invalid
     */
    INVALID_ARG,
    /**
     * Requested stream is already running
     */
    STREAM_ALREADY_RUNNING,
    /**
     * Buffer is not available; e.g. failed to allocate
     */
    BUFFER_NOT_AVAILABLE,
    /**
     * Ownership has been expired or stolen by other clients
     */
    OWNERSHIP_LOST,
    /**
     * A dependent service fails to handle a request
     */
    UNDERLYING_SERVICE_ERROR,
    /**
     * Permission denied
     */
    PERMISSION_DENIED,
    /**
     * Either the camera or the display is not available
     */
    RESOURCE_NOT_AVAILABLE,
    /**
     * Device or resource busy
     */
    RESOURCE_BUSY,
    /**
     * A method is not implemented yet
     */
    NOT_IMPLEMENTED,
    /**
     * Requested functionality is not supported
     */
    NOT_SUPPORTED,
}
