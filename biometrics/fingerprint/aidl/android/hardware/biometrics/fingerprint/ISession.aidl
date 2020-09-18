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

/**
 * Operations that can be performed for unique sessions retrieved via IFingerprint#createSession.
 * Methods defined within this interface can be split into the following categories:
 *   1) Methods associated with a state (see the SessionState enum). State-based operations are
 *      handled by the HAL in FIFO order.
 *   1a) Cancellable state-based operations. If a cancellable operation is in-progress and the
 *       framework requests a subsequent state-based operation, the implementation should finish
 *       the operation via ISessionCallback#onError with Error::CANCELED.
 *   1b) Non-cancellable state-based operations. These operations should fully complete before the
 *       next state-based operation can be started.
 *   2) Methods without a state. These methods may be invoked by the framework depending on its
 *      use case. For example on devices with sensors of FingerprintSensorType::UNDER_DISPLAY_*,
 *      ISession#onFingerDown may be invoked while the HAL is in SessionState::ENROLLING,
 *      SessionState::AUTHENTICATING, or SessionState::DETECTING_INTERACTION.
 *
 * If the HAL has multiple operations in its queue, it is not required to notify the framework
 * of SessionState::IDLING between each operation. However, it must notify the framework when all
 * work is completed. See ISessionCallback#onStateChanged. For example, the following is a valid
 * sequence of ISessionCallback#onStateChanged invocations: SessionState::IDLING -->
 * SessionState::ENROLLING --> SessionState::ENUMERATING_ENROLLMENTS --> SessionState::IDLING.
 */
@VintfStability
interface ISession {
    /**
     * Methods applicable to any fingerprint type.
     */

    /**
     * enroll:
     *
     * A request to add a fingerprint enrollment.
     *
     * Once the HAL is able to start processing the enrollment request, it must
     * notify the framework via ISessionCallback#onStateChanged with
     * SessionState::ENROLLING.
     *
     * At any point during enrollment, if a non-recoverable error occurs,
     * the HAL must notify the framework via ISessionCallback#onError with
     * the applicable enrollment-specific error, and then send
     * ISessionCallback#onStateChanged(cookie, SessionState::IDLING) if no
     * subsequent operation is in the queue.
     *
     * Before capturing fingerprint data, the implementation must first
     * verify the authenticity and integrity of the provided HardwareAuthToken.
     * In addition, it must check that the challenge within the provided
     * HardwareAuthToken is valid. See IFingerprint#generateChallenge.
     * If any of the above checks fail, the framework must be notified
     * via ISessionCallback#onError and the HAL must notify the framework when
     * it returns to the idle state. See Error::UNABLE_TO_PROCESS.
     *
     * During enrollment, the implementation may notify the framework
     * via ISessionCallback#onAcquired with messages that may be used to guide
     * the user. This callback can be invoked multiple times if necessary.
     * Similarly, the framework may be notified of enrollment progress changes
     * via ISessionCallback#onEnrollmentProgress. Once the framework is notified
     * that there are 0 "remaining" steps, the framework may cache the
     * "enrollmentId". See ISessionCallback#onEnrollmentProgress for more info.
     * The HAL must notify the framework once it returns to the idle state.
     *
     * When a finger is successfully added and before the framework is notified
     * of remaining=0, the implementation MUST update and associate this
     * (sensorId, user) pair with a new new entropy-encoded random identifier.
     * See ISession#getAuthenticatorId for more information.
     *
     * @param cookie An identifier used to track subsystem operations related
     *               to this call path. The client must guarantee that it is
     *               unique per ISession.
     * @param hat See above documentation.
     */
    ICancellationSignal enroll(in int cookie, in HardwareAuthToken hat);

    /**
     * authenticate:
     *
     * A request to start looking for fingerprints to authenticate.
     *
     * Once the HAL is able to start processing the authentication request, it must
     * notify framework via ISessionCallback#onStateChanged with
     * SessionState::AUTHENTICATING.
     *
     * At any point during authentication, if a non-recoverable error occurs,
     * the HAL must notify the framework via ISessionCallback#onError with
     * the applicable authentication-specific error, and then send
     * ISessionCallback#onStateChanged(cookie, SessionState::IDLING) if no
     * subsequent operation is in the queue.
     *
     * During authentication, the implementation may notify the framework
     * via ISessionCallback#onAcquired with messages that may be used to guide
     * the user. This callback can be invoked multiple times if necessary.
     *
     * The HAL must notify the framework of accepts/rejects via
     * ISessionCallback#onAuthentication*.
     *
     * The authentication lifecycle ends when either
     *   1) A fingerprint is accepted, and ISessionCallback#onAuthenticationSucceeded
     *      is invoked, or
     *   2) Any non-recoverable error occurs (such as lockout). See the full
     *      list of authentication-specific errors in the Error enum.
     *
     * Note that it is now the HAL's responsibility to keep track of lockout
     * states. See IFingerprint#setLockoutCallback and ISession#resetLockout.
     *
     * Note that upon successful authentication, ONLY sensors configured as
     * SensorStrength::STRONG are allowed to create and send a
     * HardwareAuthToken to the framework. See the Android CDD for more
     * details. For SensorStrength::STRONG sensors, the HardwareAuthToken's
     * "challenge" field must be set with the operationId passed in during
     * #authenticate. If the sensor is NOT SensorStrength::STRONG, the
     * HardwareAuthToken MUST be null.
     *
     * @param cookie An identifier used to track subsystem operations related
     *               to this call path. The client must guarantee that it is
     *               unique per ISession.
     * @param operationId For sensors configured as SensorStrength::STRONG,
     *                    this must be used ONLY upon successful authentication
     *                    and wrapped in the HardwareAuthToken's "challenge"
     *                    field and sent to the framework via
     *                    ISessionCallback#onAuthenticated. The operationId is
     *                    an opaque identifier created from a separate secure
     *                    subsystem such as, but not limited to KeyStore/KeyMaster.
     *                    The HardwareAuthToken can then be used as an attestation
     *                    for the provided operation. For example, this is used
     *                    to unlock biometric-bound auth-per-use keys (see
     *                    setUserAuthenticationParameters in
     *                    KeyGenParameterSpec.Builder and KeyProtection.Builder.
     */
    ICancellationSignal authenticate(in int cookie, in long operationId);

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
     * The authenticatorId is a (sensorId, user)-specific identifier which
     * can be used during key generation and key import to to associate a
     * key (in KeyStore / KeyMaster) with the current set of enrolled
     * fingerprints. For example, the following public Android APIs
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
     *               to this call path. The client must guarantee that it is
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
     *   1) Verify the authenticity and integrity of the provided HAT. If this
     *      check fails, the HAL must invoke ISessionCallback#onError with
     *      Error::UNABLE_TO_PROCESS and return to
     *      SessionState::IDLING if no subsequent work is in the queue.
     *   2) Verify that the timestamp provided within the HAT is relatively
     *      recent (e.g. on the order of minutes, not hours). If this check fails,
     *      the HAL must invoke ISessionCallback#onError with
     *      Error::UNABLE_TO_PROCESS and return to SessionState::IDLING
     *      if no subsequent work is in the queue.
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
     *               to this call path. The client must guarantee that it is
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
     * If either of the checks fail, the HAL must invoke ISessionCallback#onError
     * with Error::UNABLE_TO_PROCESS and return to SessionState::IDLING
     * if no subsequent work is in the queue.
     *
     * Upon successful verification, the HAL must clear the lockout counter
     * and notify the framework via ILockoutCallback#onLockoutChanged(sensorId, userId, 0).
     *
     * @param cookie An identifier used to track subsystem operations related
     *               to this call path. The client must guarantee that it is
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

