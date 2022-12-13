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
package android.hardware.automotive.can;

/**
 * Possible error codes (or OK) for ICanController.
 */
@VintfStability
@Backing(type="int")
enum Result {
    OK,

    /**
     * General error class, if others are not applicable.
     */
    UNKNOWN_ERROR,

    /**
     * Up request was called out of order (i.e. trying to up the interface
     * twice).
     */
    INVALID_STATE,

    /** Interface type is not supported. */
    NOT_SUPPORTED,

    /**
     * Provided interface ID (index, name, device path) doesn't exist or there
     * is no device with a given serial number.
     */
    BAD_INTERFACE_ID,

    /** Provided bit rate is not supported by the hardware. */
    BAD_BITRATE,

    /**
     * Provided bus name ({@see BusConfig#name}) has invalid format or doesn't exist.
     */
    BAD_BUS_NAME,

    /**
     * The interface for the bus you are trying to interact with is currently
     * down. As opposed to INVALID_STATE, this serves to warn the caller
     * _before_ they attempt an invalid operation.
     */
    INTERFACE_DOWN,
}
