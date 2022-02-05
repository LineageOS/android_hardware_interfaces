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

package android.hardware.camera.common;

/**
 * Common enumeration and structure definitions for all HALs under
 * android.hardware.camera
 *
 *
 * Status codes for camera HAL method service specific errors.
 *
 */
@VintfStability
@Backing(type="int")
enum Status {
    /**
     * Method call succeeded
     */
    OK = 0,
    /**
     * One of the arguments to the method call is invalid. For example,
     * the camera ID is unknown.
     */
    ILLEGAL_ARGUMENT = 1,
    /**
     * The specified camera device is already in use
     */
    CAMERA_IN_USE = 2,
    /**
     * The HAL cannot support more simultaneous cameras in use.
     */
    MAX_CAMERAS_IN_USE = 3,
    /**
     * This HAL does not support this method.
     */
    METHOD_NOT_SUPPORTED = 4,
    /**
     * The specified camera device does not support this operation.
     */
    OPERATION_NOT_SUPPORTED = 5,
    /**
     * This camera device is no longer connected or otherwise available for use
     */
    CAMERA_DISCONNECTED = 6,
    /**
     * The HAL has encountered an internal error and cannot complete the
     * request.
     */
    INTERNAL_ERROR = 7,
}
