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

import android.hardware.biometrics.fingerprint.AcquiredInfo;
import android.hardware.biometrics.fingerprint.Error;
import android.hardware.keymaster.HardwareAuthToken;

@VintfStability
interface ISessionCallback {
    /**
     * Notifies the framework when a challenge is successfully generated.
     */
    void onChallengeGenerated(in long challenge);

    /**
     * Notifies the framework when a challenge has been revoked.
     */
    void onChallengeRevoked(in long challenge);

    /**
     * This method must only be used to notify the framework during the following operations:
     *   - ISession#enroll
     *   - ISession#authenticate
     *   - ISession#detectInteraction
     *
     * These messages may be used to provide user guidance multiple times per operation if
     * necessary.
     *
     * @param info See the AcquiredInfo enum.
     * @param vendorCode Only valid if info == AcquiredInfo::VENDOR. The vendorCode must be used to
     *                   index into the configuration
     *                   com.android.internal.R.array.fingerprint_acquired_vendor that's installed
     *                   on the vendor partition.
     */
    void onAcquired(in AcquiredInfo info, in int vendorCode);

    /**
     * This method must only be used to notify the framework during the following operations:
     *   - ISession#enroll
     *   - ISession#authenticate
     *   - ISession#detectInteraction
     *   - ISession#invalidateAuthenticatorId
     *   - ISession#resetLockout
     *
     * These messages may be used to notify the framework or user that a non-recoverable error
     * has occurred. The operation is finished, and the HAL can proceed with the next operation
     * or return to the idling state.
     *
     * Note that cancellation (see common::ICancellationSignal) must be followed with an
     * Error::CANCELED message.
     *
     * @param error See the Error enum.
     * @param vendorCode Only valid if error == Error::VENDOR. The vendorCode must be used to index
     *                   into the configuration
     *                   com.android.internal.R.fingerprint_error_vendor that's installed on the
     *                   vendor partition.
     */
    void onError(in Error error, in int vendorCode);

    /**
     * This method must only be used to notify the framework during the ISession#enroll operation.
     *
     * @param enrollmentId Unique stable identifier for the enrollment that's being added by this
     *                     ISession#enroll invocation.
     * @param remaining Remaining number of steps before enrollment is complete.
     */
    void onEnrollmentProgress(in int enrollmentId, int remaining);

    /**
     * This method must only be used to notify the framework during ISession#authenticate.
     *
     * Used to notify the framework upon successful authentication. Note that the authentication
     * lifecycle ends when either 1) a fingerprint is accepted, or 2) an error occurred. The
     * authentication lifecycle does NOT end when a fingerprint is rejected.
     *
     * @param enrollmentId Fingerprint that was accepted.
     * @param hat If the sensor is configured as SensorStrength::STRONG, a non-null attestation that
     *            a fingerprint was accepted. The HardwareAuthToken's "challenge" field must be set
     *            with the operationId passed in during ISession#authenticate. If the sensor is NOT
     *            SensorStrength::STRONG, the HardwareAuthToken MUST be null.
     */
    void onAuthenticationSucceeded(in int enrollmentId, in HardwareAuthToken hat);

    /**
     * This method must only be used to notify the framework during ISession#authenticate.
     *
     * Used to notify the framework upon rejected attempts. Note that the authentication
     * lifecycle ends when either 1) a fingerprint is accepted, or 2) an error occurred.
     * The authentication lifecycle does NOT end when a fingerprint is rejected.
     */
    void onAuthenticationFailed();

    /**
     * This method must only be used to notify the framework during ISession#authenticate.
     *
     * Authentication is locked out due to too many unsuccessful attempts. This is a rate-limiting
     * lockout, and authentication can be restarted after a period of time. See
     * ISession#resetLockout.
     *
     * @param sensorId Sensor for which the user is locked out.
     * @param userId User for which the sensor is locked out.
     * @param durationMillis Remaining duration of the lockout.
     */
    void onLockoutTimed(in long durationMillis);

    /**
     * This method must only be used to notify the framework during ISession#authenticate.
     *
     * Authentication is disabled until the user unlocks with their device credential
     * (PIN/Pattern/Password). See ISession#resetLockout.
     *
     * @param sensorId Sensor for which the user is locked out.
     * @param userId User for which the sensor is locked out.
     */
    void onLockoutPermanent();

    /**
     * Notifies the framework that lockout has been cleared for this (sensorId, userId) pair.
     *
     * Note that this method can be used to notify the framework during any state.
     *
     * Lockout can be cleared in the following scenarios:
     * 1) A timed lockout has ended (e.g. durationMillis specified in previous #onLockoutTimed
     *    has expired.
     * 2) See ISession#resetLockout.
     *
     * @param sensorId Sensor for which the user's lockout is cleared.
     * @param userId User for the sensor's lockout is cleared.
     */
    void onLockoutCleared();

    /**
     * This method must only be used to notify the framework during
     * ISession#detectInteraction
     *
     * Notifies the framework that user interaction occurred. See ISession#detectInteraction.
     */
    void onInteractionDetected();

    /**
     * This method must only be used to notify the framework during
     * ISession#enumerateEnrollments.
     *
     * Notifies the framework of the current enrollments. See ISession#enumerateEnrollments.
     *
     * @param enrollmentIds A list of enrollments for the session's (userId, sensorId) pair.
     */
    void onEnrollmentsEnumerated(in int[] enrollmentIds);

    /**
     * This method must only be used to notify the framework during
     * ISession#removeEnrollments.
     *
     * Notifies the framework that the specified enrollments are removed.
     *
     * @param enrollmentIds The enrollments that were removed.
     */
    void onEnrollmentsRemoved(in int[] enrollmentIds);

    /**
     * This method must only be used to notify the framework during
     * ISession#getAuthenticatorId.
     *
     * Notifies the framework with the authenticatorId corresponding to this session's
     * (userId, sensorId) pair.
     *
     * @param authenticatorId See the above documentation.
     */
    void onAuthenticatorIdRetrieved(in long authenticatorId);

    /**
     * This method must only be used to notify the framework during
     * ISession#invalidateAuthenticatorId.
     *
     * See ISession#invalidateAuthenticatorId for more information.
     *
     * @param newAuthenticatorId The new entropy-encoded random identifier associated with the
     *                           current set of enrollments.
     */
    void onAuthenticatorIdInvalidated(in long newAuthenticatorId);

    /**
     * This method notifes the client that this session has closed.
     * The client must not make any more calls to this session.
     */
    void onSessionClosed();
}
