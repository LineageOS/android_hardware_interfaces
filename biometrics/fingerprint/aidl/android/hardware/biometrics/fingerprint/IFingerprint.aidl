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

import android.hardware.biometrics.fingerprint.IGenerateChallengeCallback;
import android.hardware.biometrics.fingerprint.ILockoutCallback;
import android.hardware.biometrics.fingerprint.IRevokeChallengeCallback;
import android.hardware.biometrics.fingerprint.ISession;
import android.hardware.biometrics.fingerprint.ISessionCallback;
import android.hardware.biometrics.fingerprint.SensorProps;

@VintfStability
interface IFingerprint {
    /**
     * getSensorProps:
     *
     * @return A list of properties for all sensors that an instance of the
     * HAL supports.
     */
    SensorProps[] getSensorProps();

    /**
     * createSession:
     *
     * Creates a session which can then be used by the framework to perform
     * operations such as enroll, authenticate, etc for the given sensorId
     * and userId.
     *
     * A physical sensor identified by sensorId typically supports only a
     * single in-flight session at a time. As such, if a session is currently
     * in a state other than SessionState::IDLING, the HAL MUST finish or
     * cancel the current operation and return to SessionState::IDLING before
     * the new session is created. For example:
     *   1) If a session for sensorId=0, userId=0
     *      is currently in a cancellable state (see ICancellationSignal) such
     *      as SessionState::AUTHENTICATING and the framework requests a new
     *      session for sensorId=0, userId=10, the HAL must end the current
     *      session with Error::CANCELED, invoke
     *      ISessionCallback#onStateChanged with SessionState::IDLING, and
     *      then return a new session for sensorId=0, userId=10.
     *   2) If a session for sensorId=0, userId=0 is currently in a
     *      non-cancellable state such as SessionState::REMOVING_ENROLLMENTS,
     *      and the framework requests a new session for sensorId=0, userId=10,
     *      the HAL must finish the current operation before invoking
     *      ISessionCallback#onStateChanged with SessionState::IDLING, and
     *      return a new session for sensorId=0, userId=10.
     *
     * Implementations must store user-specific state or metadata in
     * /data/vendor_de/<user>/fpdata as specified by the SeLinux policy. This
     * directory is created/removed by vold (see vold_prepare_subdirs.cpp).
     * Implementations may store additional user-specific data, such as
     * embeddings or templates in StrongBox.
     *
     * @param sensorId The sensor with which this session is being created.
     * @param userId The userId with which this session is being created.
     * @param cb Used to notify the framework.
     * @return A new session
     */
    ISession createSession(in int sensorId, in int userId, in ISessionCallback cb);

    /**
     * setLockoutCallback:
     *
     * Sets a callback to notify the framework lockout changes. Note
     * that lockout is user AND sensor specific. In other words, there is a
     * separate lockout state for each (user, sensor) pair. For example, the
     * following is a valid state on a multi-sensor device:
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
     * In addition, lockout states MUST persist after device reboots, HAL
     * crashes, etc.
     *
     * See the Android CDD section 7.3.10 for the full set of lockout and
     * rate-limiting requirements.
     *
     * @param cb Used to notify the framework of lockout changes.
     */
    void setLockoutCallback(in ILockoutCallback cb);

    /**
     * generateChallenge:
     *
     * Begins a secure transaction request. Note that the challenge by itself
     * is not useful. It only becomes useful when wrapped in a verifiable
     * message such as a HardwareAuthToken.
     *
     * Canonical example:
     *   1) User requests an operation, such as fingerprint enrollment.
     *   2) Fingerprint enrollment cannot happen until the user confirms
     *      their lockscreen credential (PIN/Pattern/Password).
     *   3) However, the biometric subsystem does not want just "any"
     *      proof of credential confirmation. It needs proof that the
     *      user explicitly authenticated credential in order to allow
     *      addition of biometric enrollments.
     * To secure this path, the following path is taken:
     *   1) Upon user requesting fingerprint enroll, the framework requests
     *      IFingerprint#generateChallenge
     *   2) Framework sends the challenge to the credential subsystem, and upon
     *      credential confirmation, a HAT is created, containing the challenge
     *      in the "challenge" field.
     *   3) Framework sends the HAT to the HAL, e.g. ISession#enroll.
     *   4) Implementation verifies the authenticity and integrity of the HAT.
     *   5) Implementation now has confidence that the user entered their
     *      credential to allow biometric enrollment.
     *
     * Note that the interface allows multiple in-flight challenges. For
     * example, invoking generateChallenge(0, 0, timeoutSec, cb) twice
     * does not invalidate the first challenge. The challenge is invalidated
     * only when:
     *   1) The provided timeout expires, or
     *   2) IFingerprint#revokeChallenge is invoked
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
     * @param sensorId Sensor to associate the challenge with
     * @param userId User to associate the challenge with
     * @param timeoutSec Duration for which the challenge is valid for
     * @param cb Callback to notify the framework
     */
    void generateChallenge(in int sensorId, in int userId, in int timeoutSec, in IGenerateChallengeCallback cb);

    /**
     * revokeChallenge:
     *
     * Revokes a challenge that was previously generated. Note that if an
     * invalid combination of parameters is requested, the implementation
     * must still notify the framework using the provided callback.
     *
     * @param sensorId Sensor that the revocation should apply to.
     * @param userId User that the revocation should apply to.
     * @param challenge Challenge that should be revoked.
     * @param cb Used to notify the framework.
     */
    void revokeChallenge(in int sensorId, in int userId, in long challenge, in IRevokeChallengeCallback cb);
}
