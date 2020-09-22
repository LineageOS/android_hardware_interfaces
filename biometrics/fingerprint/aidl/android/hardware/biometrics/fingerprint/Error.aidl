/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.biometrics.fingerprint;

@VintfStability
@Backing(type="byte")
enum Error {
    /**
     * Used for testing, and to keep subsequent numbering consistent with older HIDLs.
     */
    // NO_ERROR = 0,

    /**
     * A hardware error has occurred that cannot be resolved. For example, I2C failure or a broken
     * sensor.
     */
    HW_UNAVAILABLE = 1,

    /**
     * The implementation is unable to process the request. For example, invalid arguments were
     * supplied.
     */
    UNABLE_TO_PROCESS,

    /**
     * The current operation took too long to complete.
     */
    TIMEOUT,

    /**
     * No space available to store additional enrollments.
     */
    NO_SPACE,

    /**
     * The operation was canceled. See common::ICancellationSignal.
     */
    CANCELED,

    /**
     * The implementation was unable to remove an enrollment.
     * See ISession#removeEnrollments.
     */
    UNABLE_TO_REMOVE,

    /**
     * Reserved to maintain backwards compatibility. See ISessionCallback#onLockoutTimed instead.
     */
    // LOCKOUT = 7,

    /**
     * Used to enable vendor-specific error messages.
     */
    VENDOR = 8,

    /**
     * Reserved to maintain backwards compatibility. See ISessionCallback#onLockoutPermanent
     * instead.
     */
    // LOCKOUT_PERMANENT = 9,
}

