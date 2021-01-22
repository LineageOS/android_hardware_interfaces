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

package android.hardware.biometrics.face;

@VintfStability
@Backing(type="byte")
enum Error {
    /**
     * Reserved for testing and to keep subsequent numbering consistent with
     * older interfaces.
     *
     * NO_ERROR = 0,
     */

    /**
     * A hardware error has occurred that cannot be resolved. Try again later.
     */
    HW_UNAVAILABLE = 1,

    /**
     * The current enroll or authenticate operation could not be completed,
     * e.g. the sensor was unable to process the current image or the HAT was
     * invalid.
     */
    UNABLE_TO_PROCESS = 2,

    /**
     * The current operation took too long to complete. This is intended to
     * prevent programs from blocking the face HAL indefinitely. The timeout is
     * framework and sensor-specific, but is generally on the order of 30
     * seconds.
     *
     * The timeout is a device-specific time meant to optimize power. For
     * example after 30 seconds of searching for a face it can be use to
     * indicate that the implementation is no longer looking and the framework
     * should restart the operation on the next user interaction.
     */
    TIMEOUT = 3,

    /**
     * The current operation could not be completed because there is not enough
     * storage space remaining to do so.
     */
    NO_SPACE = 4,

    /**
     * The current operation has been cancelled. This may happen if a new
     * request (authenticate, remove, enumerate, enroll) is initiated while
     * an on-going operation is in progress, or if cancel() was called.
     */
    CANCELED = 5,

    /**
     * The current remove operation could not be completed; the face template
     * provided could not be removed.
     */
    UNABLE_TO_REMOVE = 6,

    /**
     * Reserved to maintain backwards compatibility. See
     * ISessionCallback#onLockoutTimed instead.
     *
     * LOCKOUT = 7,
     */

    /**
     * Used to enable a vendor-specific error message.
     */
    VENDOR = 8,

    /**
     * Reserved to maintain backwards compatibility. See
     * ISessionCallback#onLockoutPermanent instead.
     *
     * LOCKOUT_PERMANENT = 9
     */
}

