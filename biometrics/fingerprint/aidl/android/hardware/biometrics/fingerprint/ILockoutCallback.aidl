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
oneway interface ILockoutCallback {
    /**
     * Notifies the framework that the user has just entered the Error::LOCKOUT state. This must be
     * sent in the following scenarios:
     * 1) The user just attempted authentication and was rejected, resulting in a timed lockout.
     * 2) The framework just created a session for a (sensorId, userId) pair that has not been
     *    created since the HAL started (e.g. there is no active or idle session for this
     *    (sensorId, userId) pair.
     *
     * @param sensorId Sensor for which the user is locked out.
     * @param userId User for which the sensor is locked out.
     * @param durationMillis Remaining duration of the lockout.
     */
    void onLockoutTimed(in int sensorId, in int userId, in long durationMillis);

    /**
     * Notifies the framework that the user has just entered the Error::LOCKOUT_PERMANENT state.
     * This must be sent in the following scenarios:
     * 1) The user just attempted authentication and was rejected, resulting in a permanent lockout.
     * 2) The framework just created a session for a (sensorId, userId) pair that has not been
     *    created since the HAL started (e.g. there is no active or idle session for this
     *    (sensorId, userId) pair.
     *
     * @param sensorId Sensor for which the user is locked out.
     * @param userId User for which the sensor is locked out.
     */
    void onLockoutPermanent(in int sensorId, in int userId);

    /**
     * Notifies the framework that lockout has been cleared for this (sensorId, userId) pair. This
     * can happen in the following scenarios:
     * 1) A timed lockout has ended (e.g. original durationMillis specified in #onLockoutTimed
     *    has expired.
     * 2) See ISession#resetLockout.
     * 3) The (sensorId, userId) pair is not in any lockout state, and the user successfully
     *    authenticated with a fingerprint.
     *
     * @param sensorId Sensor for which the user's lockout is cleared.
     * @param userId User for the sensor's lockout is cleared.
     */
    void onLockoutCleared(in int sensorId, in int userId);
}

