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
import android.hardware.biometrics.fingerprint.SessionState;
import android.hardware.keymaster.HardwareAuthToken;

@VintfStability
interface ISessionCallback {
    void onStateChanged(in int cookie, in SessionState state);

    void onAcquired(in AcquiredInfo info, in int vendorCode);

    void onError(in Error error, in int vendorCode);

    void onEnrollmentProgress(in int enrollmentId, int remaining, int vendorCode);

    /**
     * Used to notify the framework upon successful authentication. Note that the authentication
     * lifecycle ends when either 1) a fingerprint is accepted, or 2) an error such as
     * Error::LOCKOUT occurred. The authentication lifecycle does NOT end when a fingerprint is
     * rejected.
     *
     * @param enrollmentId Fingerprint that was accepted.
     * @param hat If the sensor is configured as SensorStrength::STRONG, a non-null attestation that
     *            a fingerprint was accepted. The HardwareAuthToken's "challenge" field must be set
     *            with the operationId passed in during ISession#authenticate. If the sensor is NOT
     *            SensorStrength::STRONG, the HardwareAuthToken MUST be null.
     */
    void onAuthenticationSucceeded(in int enrollmentId, in HardwareAuthToken hat);

    /**
     * Used to notify the framework upon rejected attempts. Note that the authentication
     * lifecycle ends when either 1) a fingerprint is accepted, or 2) an error such as
     * Error::LOCKOUT occurred. The authentication lifecycle does NOT end when a fingerprint is
     * rejected.
     */
    void onAuthenticationFailed();

    void onInteractionDetected();

    void onEnrollmentsEnumerated(in int[] enrollmentIds);

    void onEnrollmentsRemoved(in int[] enrollmentIds);

    /**
     * A callback invoked when ISession#getAuthenticatorId is invoked.
     */
    void onAuthenticatorIdRetrieved(in long authenticatorId);

    /**
     * A callback invoked when ISession#invalidateAuthenticatorId has completed.
     */
    void onAuthenticatorIdInvalidated();
}
