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

import android.hardware.biometrics.common.ICancellationSignal;
import android.hardware.biometrics.face.Feature;
import android.hardware.biometrics.face.EnrollmentType;
import android.hardware.keymaster.HardwareAuthToken;
import android.hardware.common.NativeHandle;

/** * A session is a collection of immutable state (sensorId, userId), mutable state (SessionState),
 * methods available for the framework to call, and a callback (ISessionCallback) to notify the
 * framework about the events and results. A session is used to establish communication between
 * the framework and the HAL.
 */
@VintfStability
interface ISession {
   /**
     * generateChallenge:
     *
     * Begins a secure transaction request. Note that the challenge by itself is not useful. It only
     * becomes useful when wrapped in a verifiable message such as a HardwareAuthToken.
     *
     * Canonical example:
     *   1) User requests an operation, such as face enrollment.
     *   2) Face enrollment cannot happen until the user confirms their lockscreen credential
     *      (PIN/Pattern/Password).
     *   3) However, the biometric subsystem does not want just "any" proof of credential
     *      confirmation. It needs proof that the user explicitly authenticated credential in order
     *      to allow addition of biometric enrollments.
     * To secure this path, the following path is taken:
     *   1) Upon user requesting face enroll, the framework requests
     *      IFace#generateChallenge
     *   2) Framework sends the challenge to the credential subsystem, and upon credential
     *      confirmation, a HAT is created, containing the challenge in the "challenge" field.
     *   3) Framework sends the HAT to the HAL, e.g. ISession#enroll.
     *   4) Implementation verifies the authenticity and integrity of the HAT.
     *   5) Implementation now has confidence that the user entered their credential to allow
     *      biometric enrollment.
     *
     * Note that the interface allows multiple in-flight challenges. For example, invoking
     * generateChallenge(0, 0, timeoutSec) twice does not invalidate the first challenge. The
     * challenge is invalidated only when:
     *   1) The provided timeout expires, or
     *   2) IFace#revokeChallenge is invoked
     *
     * For example, the following is a possible table of valid challenges:
     * ----------------------------------------------
     * | SensorId | UserId | ValidUntil | Challenge |
     * |----------|--------|------------|-----------|
     * | 0        | 0      | <Time1>    | <Random1> |
     * | 0        | 0      | <Time2>    | <Random2> |
     * | 1        | 0      | <Time3>    | <Random3> |
     * | 0        | 10     | <Time4>    | <Random4> |
     * ----------------------------------------------
     *
     * @param cookie A unique number identifying this operation
     * @param timeoutSec Duration for which the challenge is valid for
     */
    void generateChallenge(in int cookie, in int timeoutSec);

    /**
     * revokeChallenge:
     *
     * Revokes a challenge that was previously generated. Note that if an invalid combination of
     * parameters is requested, the implementation must still notify the framework using the
     * provided callback.
     *
     * @param cookie A unique number identifying this operation
     * @param challenge Challenge that should be revoked.
     */
    void revokeChallenge(in int cookie, in long challenge);

    /**
     * getEnrollmentConfig:
     *
     * Returns the enrollment configuration depending on the provided enrollment type. Enrollment
     * configuration determines how many stages the enrollment will have and the requirements for
     * each of the stages.
     *
     * @param enrollmentType See the EnrollmentType enum.
     * @return A list of EnrollmentStageConfig that describes each enrollment stage.
     *
    List<EnrollmentStageConfig> getEnrollmentConfig(in EnrollmentType enrollmentType);

    /**
     * enroll:
     *
     * A request to add a face enrollment.
     *
     * Once the HAL is able to start processing the enrollment request, it must notify the framework
     * via ISessionCallback#onStateChanged with SessionState::ENROLLING.
     *
     * At any point during enrollment, if a non-recoverable error occurs, the HAL must notify the
     * framework via ISessionCallback#onError with the applicable enrollment-specific error, and
     * then send ISessionCallback#onStateChanged(cookie, SessionState::IDLING) if no subsequent
     * operation is in the queue.
     *
     * Before capturing face data, the implementation must first verify the authenticity and
     * integrity of the provided HardwareAuthToken. In addition, it must check that the challenge
     * within the provided HardwareAuthToken is valid. See IFace#generateChallenge. If any of
     * the above checks fail, the framework must be notified via ISessionCallback#onError and the
     * HAL must notify the framework when it returns to the idle state. See
     * Error::UNABLE_TO_PROCESS.
     *
     * During enrollment, the implementation may notify the framework via
     * ISessionCallback#onAcquired with messages that may be used to guide the user. This callback
     * can be invoked multiple times if necessary. Similarly, the framework may be notified of
     * enrollment progress changes via ISessionCallback#onEnrollmentProgress. Once the framework is
     * notified that there are 0 "remaining" steps, the framework may cache the "enrollmentId". See
     * ISessionCallback#onEnrollmentProgress for more info. The HAL must notify the framework once
     * it returns to the idle state.
     *
     * When a finger is successfully added and before the framework is notified of remaining=0, the
     * implementation MUST update and associate this (sensorId, userId) pair with a new new
     * entropy-encoded random identifier. See ISession#getAuthenticatorId for more information.
     *
     * @param cookie An identifier used to track subsystem operations related to this call path. The
     *               client must guarantee that it is unique per ISession.
     * @param hat See above documentation.
     * @param enrollmentType See the EnrollmentType enum.
     * @param features See the Feature enum.
     * @param previewSurface A surface provided by the framework if SensorProps#halControlsPreview is
     *                       set to true. The HAL must send the preview frames to previewSurface if
     *                       it's not null.
     * @return ICancellationSignal An object that can be used by the framework to cancel this
     * operation.
     */
    ICancellationSignal enroll(in int cookie, in HardwareAuthToken hat, in EnrollmentType type,
            in Feature[] features, in NativeHandle previewSurface);

    /**
     * authenticate:
     *
     * A request to start looking for faces to authenticate.
     *
     * Once the HAL is able to start processing the authentication request, it must notify framework
     * via ISessionCallback#onStateChanged with SessionState::AUTHENTICATING.
     *
     * At any point during authentication, if a non-recoverable error occurs, the HAL must notify
     * the framework via ISessionCallback#onError with the applicable authentication-specific error,
     * and then send ISessionCallback#onStateChanged(cookie, SessionState::IDLING) if no
     * subsequent operation is in the queue.
     *
     * During authentication, the implementation may notify the framework via
     * ISessionCallback#onAcquired with messages that may be used to guide the user. This callback
     * can be invoked multiple times if necessary.
     *
     * The HAL must notify the framework of accepts/rejects via ISessionCallback#onAuthentication*.
     *
     * The authentication lifecycle ends when either
     *   1) A face is accepted, and ISessionCallback#onAuthenticationSucceeded is invoked, or
     *   2) Any non-recoverable error occurs (such as lockout). See the full list of
     *      authentication-specific errors in the Error enum.
     *
     * Note that upon successful authentication, the lockout counter for this (sensorId, userId)
     * pair must be cleared.
     *
     * Note that upon successful authentication, ONLY sensors configured as SensorStrength::STRONG
     * are allowed to create and send a HardwareAuthToken to the framework. See the Android CDD for
     * more details. For SensorStrength::STRONG sensors, the HardwareAuthToken's "challenge" field
     * must be set with the operationId passed in during #authenticate. If the sensor is NOT
     * SensorStrength::STRONG, the HardwareAuthToken MUST be null.
     *
     * @param cookie An identifier used to track subsystem operations related to this call path. The
     *               client must guarantee that it is unique per ISession.
     * @param operationId For sensors configured as SensorStrength::STRONG, this must be used ONLY
     *                    upon successful authentication and wrapped in the HardwareAuthToken's
     *                    "challenge" field and sent to the framework via
     *                    ISessionCallback#onAuthenticated. The operationId is an opaque identifier
     *                    created from a separate secure subsystem such as, but not limited to
     *                    KeyStore/KeyMaster. The HardwareAuthToken can then be used as an
     *                    attestation for the provided operation. For example, this is used
     *                    to unlock biometric-bound auth-per-use keys (see
     *                    setUserAuthenticationParameters in KeyGenParameterSpec.Builder and
     *                    KeyProtection.Builder.
     * @return ICancellationSignal An object that can be used by the framework to cancel this
     * operation.
     */
    ICancellationSignal authenticate(in int cookie, in long operationId);

    /**
     * detectInteraction:
     *
     * A request to start looking for faces without performing matching. Must only be called if
     * SensorProps#supportsDetectInteraction is true. If invoked on implementations that do not
     * support this functionality, the HAL must respond with ISession#onError(UNABLE_TO_PROCESS, 0).
     *
     * Once the HAL is able to start processing this request, it must notify the framework via
     * ISessionCallback#onStateChanged with SessionState::DETECTING_INTERACTION.
     *
     * The framework will use this method in cases where determing user presence is required, but
     * identifying/authentication is not. For example, when the device is encrypted (first boot) or
     * in lockdown mode.
     *
     * At any point during detectInteraction, if a non-recoverable error occurs, the HAL must notify
     * the framework via ISessionCallback#onError with the applicable error, and then send
     * ISessionCallback#onStateChanged(cookie, SessionState::IDLING) if no subsequent operation is
     * in the queue.
     *
     * The implementation must only check for a face-like image was detected (e.g. to
     * minimize interactions due to non-face objects), and the lockout counter must not
     * be modified.
     *
     * Upon detecting any face, the implementation must invoke
     * ISessionCallback#onInteractionDetected.
     *
     * The lifecycle of this operation ends when either
     * 1) Any face is detected and the framework is notified via
     *    ISessionCallback#onInteractiondetected
     * 2) The operation was cancelled by the framework (see ICancellationSignal)
     * 3) The HAL ends the operation, for example when a subsequent operation pre-empts this one.
     *
     * Note that if the operation is canceled, the implementation must notify the framework via
     * ISessionCallback#onError with Error::CANCELED.
     *
     * @param cookie An identifier used to track subsystem operations related to this call path.
     *               The framework will guarantee that it is unique per ISession.
     * @return ICancellationSignal An object that can be used by the framework to cancel this
     * operation.
     */
    ICancellationSignal detectInteraction(in int cookie);

    /*
     * enumerateEnrollments:
     *
     * A request to enumerate (list) the enrollments for this (sensorId, userId) pair. The
     * framework typically uses this to ensure that its cache is in sync with the HAL.
     *
     * Once the HAL is able to start processing this request, it must notify the framework via
     * ISessionCallback#onStateChanged with SessionState::ENUMERATING_ENROLLMENTS.
     *
     * The implementation must then notify the framework with a list of enrollments applicable
     * for the current session via ISessionCallback#onEnrollmentsEnumerated.
     *
     * @param cookie An identifier used to track subsystem operations related to this call path.
     *               The framework will guarantee that it is unique per ISession.
     */
    void enumerateEnrollments(in int cookie);

    /**
     * removeEnrollments:
     *
     * A request to remove the enrollments for this (sensorId, userId) pair.
     *
     * Once the HAL is able to start processing this request, it must notify the framework via
     * ISessionCallback#onStateChanged with SessionState::REMOVING_ENROLLMENTS.
     *
     * After removing the enrollmentIds from everywhere necessary (filesystem, secure subsystems,
     * etc), the implementation must notify the framework via ISessionCallback#onEnrollmentsRemoved.
     *
     * @param cookie An identifier used to track subsystem operations related to this call path.
     *               The framework will guarantee that it is unique per ISession.
     */
    void removeEnrollments(in int cookie, in int[] enrollmentIds);

    /**
     * getFeatures:
     *
     * Returns a list of currently enabled features for the provided enrollmentId.
     *
     * If the enrollmentId is invalid, the HAL must invoke ISessionCallback#onError with
     * Error::UNABLE_TO_PROCESS and return to SessionState::IDLING if no subsequent work is in the
     * queue.
     *
     * Once the HAL is able to start processing this request, it must notify the framework by using
     * ISessionCallback#onStateChanged with SessionState::GETTING_FEATURES.
     *
     * The HAL must notify the framework about the result by calling
     * ISessionCallback#onFeaturesRetrieved.
     *
     * @param cookie An identifier used to track subsystem operations related to this call path. The
     *               client must guarantee that it is unique per ISession.
     * @param enrollmentId the ID of the enrollment for which the features are requested.
     */
    void getFeatures(in int cookie, in int enrollmentId);

    /**
     * setFeature:
     *
     * Enables or disables a feature for the given enrollmentId. Because certain features may
     * decrease security, the user must enter their password before this method is invoked
     * (see @param hat). The HAL must verify the hat before changing any feature state.
     *
     * If either the hat or enrollmentId is invalid, the HAL must invoke ISessionCallback#onError
     * with Error::UNABLE_TO_PROCESS and return to SessionState::IDLING if no subsequent work is in
     * the queue.
     *
     * Once the HAL is able to start processing this request, it must notify the framework by using
     * ISessionCallback#onStateChanged with SessionState::SETTING_FEATURE.
     *
     * After the feature is successfully set, the HAL must notify the framework by calling
     * ISessionCallback#onFeatureSet.
     *
     * @param cookie An identifier used to track subsystem operations related to this call path. The
     *               client must guarantee that it is unique per ISession.
     * @param hat HardwareAuthToken See above documentation.
     * @param enrollmentId the ID of the enrollment for which the feature update is requested.
     * @param feature The feature to be enabled or disabled.
     * @param enabled Whether the provided features should be enabled or disabled.
     */
    void setFeature(in int cookie, in HardwareAuthToken hat, in int enrollmentId,
            in Feature feature, boolean enabled);

    /**
     * getAuthenticatorId:
     *
     * MUST return 0 via ISessionCallback#onAuthenticatorIdRetrieved for sensors that are configured
     * as SensorStrength::WEAK or SensorStrength::CONVENIENCE.
     *
     * The following only applies to sensors that are configured as SensorStrength::STRONG.
     *
     * The authenticatorId is a (sensorId, user)-specific identifier which can be used during key
     * generation and key import to to associate a key (in KeyStore / KeyMaster) with the current
     * set of enrolled faces. For example, the following public Android APIs allow for keys
     * to be invalidated when the user adds a new enrollment after the key was created:
     * KeyGenParameterSpec.Builder.setInvalidatedByBiometricEnrollment and
     * KeyProtection.Builder.setInvalidatedByBiometricEnrollment.
     *
     * In addition, upon successful face authentication, the signed HAT that is returned to
     * the framework via ISessionCallback#onAuthenticated must contain this identifier in the
     * authenticatorId field.
     *
     * Returns an entropy-encoded random identifier associated with the current set of enrollments
     * via ISessionCallback#onAuthenticatorIdRetrieved. The authenticatorId
     *   1) MUST change whenever a new face is enrolled
     *   2) MUST return 0 if no faces are enrolled
     *   3) MUST not change if a face is deleted.
     *   4) MUST be an entropy-encoded random number
     *
     * @param cookie An identifier used to track subsystem operations related to this call path. The
     *               client must guarantee that it is unique per ISession.
     */
    void getAuthenticatorId(in int cookie);

    /**
     * invalidateAuthenticatorId:
     *
     * This method only applies to sensors that are configured as SensorStrength::STRONG. If invoked
     * by the framework for sensor of other strengths, the HAL should immediately invoke
     * ISessionCallback#onAuthenticatorIdInvalidated.
     *
     * The following only applies to sensors that are configured as SensorStrength::STRONG.
     *
     * When invoked by the framework, the implementation must perform the following sequence of
     * events:
     *   1) Update the authenticatorId with a new entropy-encoded random number
     *   2) Persist the new authenticatorId to non-ephemeral storage
     *   3) Notify the framework that the above is completed, via
     *      ISessionCallback#onAuthenticatorInvalidated
     *
     * A practical use case of invalidation would be when the user adds a new enrollment to a sensor
     * managed by a different HAL instance. The public android.security.keystore APIs bind keys to
     * "all biometrics" rather than "face-only" or "face-only" (see #getAuthenticatorId
     * for more details). As such, the framework would coordinate invalidation across multiple
     * biometric HALs as necessary.
     *
     * @param cookie An identifier used to track subsystem operations related to this call path. The
     *               client must guarantee that it is unique per ISession.
     */
    void invalidateAuthenticatorId(in int cookie);

    /**
     * resetLockout:
     *
     * Requests the implementation to clear the lockout counter. Upon receiving this request, the
     * implementation must perform the following:
     *   1) Verify the authenticity and integrity of the provided HAT
     *   2) Verify that the timestamp provided within the HAT is relatively recent (e.g. on the
     *      order of minutes, not hours).
     * If either of the checks fail, the HAL must invoke ISessionCallback#onError with
     * Error::UNABLE_TO_PROCESS and return to SessionState::IDLING if no subsequent work is in the
     * queue.
     *
     * Upon successful verification, the HAL must clear the lockout counter and notify the framework
     * via ISessionCallback#onLockoutCleared.
     *
     * Note that lockout is user AND sensor specific. In other words, there is a separate lockout
     * state for each (user, sensor) pair. For example, the following is a valid state on a
     * multi-sensor device:
     * ------------------------------------------------------------------
     * | SensorId | UserId | FailedAttempts | LockedOut | LockedUntil   |
     * |----------|--------|----------------|-----------|---------------|
     * | 0        | 0      | 1              | false     | x             |
     * | 1        | 0      | 5              | true      | <future_time> |
     * | 0        | 10     | 0              | false     | x             |
     * | 1        | 10     | 0              | false     | x             |
     * ------------------------------------------------------------------
     *
     * Lockout may be cleared in the following ways:
     *   1) ISession#resetLockout
     *   2) After a period of time, according to a rate-limiter.
     *
     * Note that the "FailedAttempts" counter must be cleared upon successful face
     * authentication. For example, if SensorId=0 UserId=0 FailedAttempts=1, and a successful
     * face authentication occurs, the counter for that (SensorId, UserId) pair must be reset
     * to 0.
     *
     * In addition, lockout states MUST persist after device reboots, HAL crashes, etc.
     *
     * See the Android CDD section 7.3.10 for the full set of lockout and rate-limiting
     * requirements.
     *
     * @param cookie An identifier used to track subsystem operations related to this call path. The
     *               client must guarantee that it is unique per ISession.
     * @param hat HardwareAuthToken See above documentation.
     */
    void resetLockout(in int cookie, in HardwareAuthToken hat);
}

