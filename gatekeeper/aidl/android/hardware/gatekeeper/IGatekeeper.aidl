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

package android.hardware.gatekeeper;

import android.hardware.gatekeeper.GatekeeperEnrollResponse;
import android.hardware.gatekeeper.GatekeeperVerifyResponse;

@VintfStability
@SensitiveData
interface IGatekeeper {
    /**
     * enroll and verify binder calls may return a ServiceSpecificException
     * with the following error codes.
     */
    /* Success, but upper layers should re-enroll the verified password due to a version change. */
    const int STATUS_REENROLL = 1;
    /* operation is successful */
    const int STATUS_OK = 0;
    /* operation failed. */
    const int ERROR_GENERAL_FAILURE = -1;
    /* operation should  be retried after timeout. */
    const int ERROR_RETRY_TIMEOUT = -2;
    /* operation is not implemented. */
    const int ERROR_NOT_IMPLEMENTED = -3;

    /**
     * Deletes all the enrolled_password_handles for all uid's. Once called,
     * no users must be enrolled on the device.
     * This is an optional method.
     *
     * Service status return:
     *
     * OK if all the users are deleted successfully.
     * ERROR_GENERAL_FAILURE on failure.
     * ERROR_NOT_IMPLEMENTED if not implemented.
     */
    void deleteAllUsers();

    /**
     * Deletes the enrolledPasswordHandle associated with the uid. Once deleted
     * the user cannot be verified anymore.
     * This is an optional method.
     *
     * Service status return:
     *
     * OK if user is deleted successfully.
     * ERROR_GENERAL_FAILURE on failure.
     * ERROR_NOT_IMPLEMENTED if not implemented.
     *
     * @param uid The Android user identifier
     */
    void deleteUser(in int uid);

    /**
     * Enrolls desiredPassword, which may be derived from a user selected pin
     * or password, with the private key used only for enrolling authentication
     * factor data.
     *
     * If there was already a password enrolled, current password handle must be
     * passed in currentPasswordHandle, and current password must be passed in
     * currentPassword. Valid currentPassword must verify() against
     * currentPasswordHandle.
     *
     * Service status return:
     *
     * OK if password is enrolled successfully.
     * ERROR_GENERAL_FAILURE on failure.
     * ERROR_NOT_IMPLEMENTED if not implemented.
     *
     * @param uid The Android user identifier
     *
     * @param currentPasswordHandle The currently enrolled password handle the user
     *    wants to replace. May be empty only if there's no currently enrolled
     *    password. Otherwise must be non-empty.
     *
     * @param currentPassword The user's current password in plain text.
     *    it MUST verify against current_password_handle if the latter is not-empty
     *
     * @param desiredPassword The new password the user wishes to enroll in
     *    plaintext.
     *
     * @return
     *    On success, data buffer must contain the new password handle referencing
     *    the password provided in desiredPassword.
     *    This buffer can be used on subsequent calls to enroll or
     *    verify. response.statusCode must contain either ERROR_RETRY_TIMEOUT or
     *    STATUS_OK. On error, this buffer must be empty. This method may return
     *    ERROR_GENERAL_FAILURE on failure.
     *    If ERROR_RETRY_TIMEOUT is returned, response.timeout must be non-zero.
     */
    GatekeeperEnrollResponse enroll(in int uid, in byte[] currentPasswordHandle,
            in byte[] currentPassword, in byte[] desiredPassword);

    /**
     * Verifies that providedPassword matches enrolledPasswordHandle.
     *
     * Implementations of this module may retain the result of this call
     * to attest to the recency of authentication.
     *
     * On success, returns verification token in response.data, which shall be
     * usable to attest password verification to other trusted services.
     *
     * Service status return:
     *
     * OK if password is enrolled successfully.
     * ERROR_GENERAL_FAILURE on failure.
     * ERROR_NOT_IMPLEMENTED if not implemented.
     *
     * @param uid The Android user identifier
     *
     * @param challenge An optional challenge to authenticate against, or 0.
     *    Used when a separate authenticator requests password verification,
     *    or for transactional password authentication.
     *
     * @param enrolledPasswordHandle The currently enrolled password handle that
     *    user wishes to verify against. Must be non-empty.
     *
     * @param providedPassword The plaintext password to be verified against the
     *    enrolledPasswordHandle
     *
     * @return
     *    On success, a HardwareAuthToken resulting from this verification is returned.
     *    response.statusCode must contain either ERROR_RETRY_TIMEOUT or
     *    or STATUS_REENROLL or STATUS_OK.
     *    On error, data buffer must be empty.
     *    This method may return ERROR_GENERAL_FAILURE on failure.
     *    If password re-enrollment is necessary, it must return STATUS_REENROLL.
     *    If ERROR_RETRY_TIMEOUT is returned, response.timeout must be non-zero.
     */
    GatekeeperVerifyResponse verify(in int uid, in long challenge, in byte[] enrolledPasswordHandle,
            in byte[] providedPassword);
}
