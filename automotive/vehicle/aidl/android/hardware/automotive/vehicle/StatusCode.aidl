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
 * Error codes used in vehicle HAL interface. System defined error codes will have the range from
 * 0x0000 to 0xffff and vendor error codes will have the range from 0x0001 to 0xffff. The error code
 * is formatted as [VENDOR_ERROR] << 16 | [SYSTEM_ERROR]. A vendor error code of 0 indicates vendor
 * code not set.
 */
@VintfStability
@Backing(type="int")
enum StatusCode {
    OK = 0,
    /**
     * Caller should try again.
     *
     * This code must be returned when an ephemeral error happens and a retry
     * will likely succeed. E.g., when the device is currently booting up
     * and the property is not ready yet.
     */
    TRY_AGAIN = 1,
    /**
     * Invalid argument provided.
     */
    INVALID_ARG = 2,
    /**
     * The property is currently unavailable and will be unavailable unless
     * some other state changes.
     *
     * This code must be returned when device that associated with the vehicle
     * property is not available. For example, when client tries to set HVAC
     * temperature when the whole HVAC unit is turned OFF.
     *
     * The difference between this and TRY_AGAIN is that if NOT_AVAILABLE is
     * returned for a property, it will remain NOT_AVAILABLE unless some other
     * state changes. This means a retry will likely still return NOT_AVAILABLE.
     * However, for TRY_AGAIN error, a retry will likely return OK.
     *
     * When subscribing to a property that is currently unavailable for getting.
     * VHAL must return OK even if getting/setting must return NOT_AVAILABLE.
     * VHAL must not generate property change event when the property is not
     * available for getting.
     */
    NOT_AVAILABLE = 3,
    /**
     * Access denied
     */
    ACCESS_DENIED = 4,
    /**
     * Something unexpected has happened in Vehicle HAL
     */
    INTERNAL_ERROR = 5,

    /**
     * The following error codes were added in version 2 of this interface.
     */

    /**
     * For features that are not available because the underlying feature is
     * disabled.
     */
    NOT_AVAILABLE_DISABLED = 6,
    /**
     * For features that are not available because the vehicle speed is too low.
     */
    NOT_AVAILABLE_SPEED_LOW = 7,
    /**
     * For features that are not available because the vehicle speed is too
     * high.
     */
    NOT_AVAILABLE_SPEED_HIGH = 8,
    /**
     * For features that are not available because of bad camera or sensor
     * visibility. Examples might be bird poop blocking the camera or a bumper
     * cover blocking an ultrasonic sensor.
     */
    NOT_AVAILABLE_POOR_VISIBILITY = 9,
    /**
     * The feature cannot be accessed due to safety reasons. Eg. System could be
     * in a faulty state, an object or person could be blocking the requested
     * operation such as closing a trunk door, etc.
     */
    NOT_AVAILABLE_SAFETY = 10,
}
