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

import android.hardware.biometrics.common.ICancellationSignal;
import android.hardware.keymaster.HardwareAuthToken;

@VintfStability
interface ISession {
    /**
     * Methods applicable to any fingerprint type.
     */

    ICancellationSignal enroll(in int cookie, in HardwareAuthToken hat);

    ICancellationSignal authenticate(in int cookie, in long keystoreOperationId);

    ICancellationSignal detectInteraction(in int cookie);

    void enumerateEnrollments(in int cookie);

    void removeEnrollments(in int cookie, in int[] enrollmentIds);

    /**
     * getAuthenticatorId:
     *
     * MUST return 0 via ISessionCallback#onAuthenticatorIdRetrieved for
     * sensors that are configured as SensorStrength::WEAK or
     * SensorStrength::CONVENIENCE.
     *
     * The following only applies to sensors that are configured as
     * SensorStrength::STRONG.
     *
     * The authenticatorId is used during key generation and key import to to
     * associate a key (in KeyStore / KeyMaster) with the current set of
     * enrolled fingerprints. For example, the following public Android APIs
     * allow for keys to be invalidated when the user adds a new enrollment
     * after the key was created:
     * KeyGenParameterSpec.Builder.setInvalidatedByBiometricEnrollment and
     * KeyProtection.Builder.setInvalidatedByBiometricEnrollment.
     *
     * In addition, upon successful fingerprint authentication, the signed HAT
     * that is returned to the framework via ISessionCallback#onAuthenticated
     * must contain this identifier in the authenticatorId field.
     *
     * Returns an entropy-encoded random identifier associated with the current
     * set of enrollments via ISessionCallback#onAuthenticatorIdRetrieved. The
     * authenticatorId
     *   1) MUST change whenever a new fingerprint is enrolled
     *   2) MUST return 0 if no fingerprints are enrolled
     *   3) MUST not change if a fingerprint is deleted.
     *   4) MUST be an entropy-encoded random number
     *
     * @param cookie An identifier used to track subsystem operations related
     *               to this call path. The framework will guarantee that it is
     *               unique per ISession.
     */
    void getAuthenticatorId(in int cookie);

    /**
     * invalidateAuthenticatorId:
     *
     * This method only applies to sensors that are configured as
     * SensorStrength::STRONG. If invoked erroneously by the framework for
     * sensor of other strengths, the HAL should immediately invoke
     * ISessionCallback#onAuthenticatorIdInvalidated.
     *
     * The following only applies to sensors that are configured as
     * SensorStrength::STRONG.
     *
     * When invoked by the framework, the implementation must perform the
     * following sequence of events:
     *   1) Verify the authenticity and integrity of the provided HAT
     *   2) Verify that the timestamp provided within the HAT is relatively
     *      recent (e.g. on the order of minutes, not hours).
     *   3) Update the authenticatorId with a new entropy-encoded random number
     *   4) Persist the new authenticatorId to non-ephemeral storage
     *   5) Notify the framework that the above is completed, via
     *      ISessionCallback#onAuthenticatorInvalidated
     *
     * A practical use case of invalidation would be when the user adds a new
     * enrollment to a sensor managed by a different HAL instance. The
     * public android.security.keystore APIs bind keys to "all biometrics"
     * rather than "fingerprint-only" or "face-only" (see #getAuthenticatorId
     * for more details). As such, the framework would coordinate invalidation
     * across multiple biometric HALs as necessary.
     *
     * @param cookie An identifier used to track subsystem operations related
     *               to this call path. The framework will guarantee that it is
     *               unique per ISession.
     * @param hat HardwareAuthToken that must be validated before proceeding
     *            with this operation.
     */
    void invalidateAuthenticatorId(in int cookie, in HardwareAuthToken hat);

    /**
     * resetLockout:
     *
     * Requests the implementation to clear the lockout counter. Upon receiving
     * this request, the implementation must perform the following:
     *   1) Verify the authenticity and integrity of the provided HAT
     *   2) Verify that the timestamp provided within the HAT is relatively
     *      recent (e.g. on the order of minutes, not hours).
     *
     * Upon successful verification, the HAL must notify the framework via
     * ILockoutCallback#onLockoutChanged(sensorId, userId, 0).
     *
     * If verification was uncessful, the HAL must notify the framework via
     * ILockoutCallback#onLockoutChanged(sensorId, userId, remaining_time).
     *
     * @param cookie An identifier used to track subsystem operations related
     *               to this call path. The framework will guarantee that it is
     *               unique per ISession.
     * @param hat HardwareAuthToken See above documentation.
     */
    void resetLockout(in int cookie, in HardwareAuthToken hat);


    /**
     * Methods for notifying the under-display fingerprint sensor about external events.
     */

    void onPointerDown(in int pointerId, in int x, in int y, in float minor, in float major);

    void onPointerUp(in int pointerId);

    void onUiReady();
}

