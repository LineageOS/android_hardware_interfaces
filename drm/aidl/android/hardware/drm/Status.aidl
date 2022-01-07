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

package android.hardware.drm;

@VintfStability
@Backing(type="int")
enum Status {
    /**
     * The DRM plugin must return OK when an operation completes without any
     * errors.
     */
    OK,
    /**
     * The DRM plugin must return ERROR_DRM_NO_LICENSE, when decryption is
     * attempted and no license keys have been provided.
     */
    ERROR_DRM_NO_LICENSE,
    /**
     * ERROR_DRM_LICENSE_EXPIRED must be returned when an attempt is made
     * to use a license and the keys in that license have expired.
     */
    ERROR_DRM_LICENSE_EXPIRED,
    /**
     * The DRM plugin must return ERROR_DRM_SESSION_NOT_OPENED when an
     * attempt is made to use a session that has not been opened.
     */
    ERROR_DRM_SESSION_NOT_OPENED,
    /**
     * The DRM plugin must return ERROR_DRM_CANNOT_HANDLE when an unsupported
     * data format or operation is attempted.
     */
    ERROR_DRM_CANNOT_HANDLE,
    /**
     * ERROR_DRM_INVALID_STATE must be returned when the device is in a state
     * where it is not able to perform decryption.
     */
    ERROR_DRM_INVALID_STATE,
    /**
     * The DRM plugin must return BAD_VALUE whenever an illegal parameter is
     * passed to one of the interface functions.
     */
    BAD_VALUE,
    /**
     * The DRM plugin must return ERROR_DRM_NOT_PROVISIONED from getKeyRequest,
     * openSession or provideKeyResponse when the device has not yet been
     * provisioned.
     */
    ERROR_DRM_NOT_PROVISIONED,
    /**
     * ERROR_DRM_RESOURCE_BUSY must be returned when resources, such as drm
     * sessions or secure buffers are not available to perform a requested
     * operation because they are already in use.
     */
    ERROR_DRM_RESOURCE_BUSY,
    /**
     * The DRM Plugin must return ERROR_DRM_INSUFFICIENT_OUTPUT_PROTECTION
     * when the output protection level enabled on the device is not
     * sufficient to meet the requirements in the license policy.  HDCP is an
     * example of a form of output protection.
     */
    ERROR_DRM_INSUFFICIENT_OUTPUT_PROTECTION,
    /**
     * The DRM Plugin must return ERROR_DRM_DEVICE_REVOKED from
     * provideProvisionResponse and provideKeyResponse if the response indicates
     * that the device has been revoked. Device revocation means that the device
     * is no longer permitted to play content.
     */
    ERROR_DRM_DEVICE_REVOKED,
    /**
     * The DRM Plugin must return ERROR_DRM_DECRYPT if the CryptoPlugin
     * decrypt operation fails.
     */
    ERROR_DRM_DECRYPT,
    /**
     * ERROR_DRM_UNKNOWN must be returned when a fatal failure occurs and no
     * other defined error is appropriate.
     */
    ERROR_DRM_UNKNOWN,
    /**
     * The drm HAL module must return ERROR_DRM_INSUFFICIENT_SECURITY
     * from the crypto plugin decrypt method when the security level
     * of the device is not sufficient to meet the requirements in the
     * license policy.
     */
    ERROR_DRM_INSUFFICIENT_SECURITY,
    /**
     * The drm HAL module must return ERROR_FRAME_TOO_LARGE from the
     * decrypt method when the frame being decrypted into the secure
     * output buffer exceeds the size of the buffer.
     */
    ERROR_DRM_FRAME_TOO_LARGE,
    /**
     * This error must be returned from any session method when an
     * attempt is made to use the session after the crypto hardware
     * state has been invalidated. Some devices are not able to
     * retain crypto session state across device suspend/resume which
     * results in invalid session state.
     */
    ERROR_DRM_SESSION_LOST_STATE,
    /**
     * The drm HAL module must return this error if client
     * applications using the hal are temporarily exceeding the
     * capacity of available crypto resources such that a retry of
     * the operation is likely to succeed.
     */
    ERROR_DRM_RESOURCE_CONTENTION,
    /**
     * queueSecureInput buffer called with 0 subsamples.
     */
    CANNOT_DECRYPT_ZERO_SUBSAMPLES,
    /**
     * An error happened within the crypto library used by the drm plugin.
     */
    CRYPTO_LIBRARY_ERROR,
    /**
     * Non-specific error reported by the device OEM subsystem.
     */
    GENERAL_OEM_ERROR,
    /**
     * Unexpected internal failure in the drm/crypto plugin.
     */
    GENERAL_PLUGIN_ERROR,
    /**
     * The init data parameter passed to getKeyRequest is empty or invalid.
     */
    INIT_DATA_INVALID,
    /**
     * Either the key was not loaded from the license before attempting the
     * operation, or the key ID parameter provided by the app is incorrect.
     */
    KEY_NOT_LOADED,
    /**
     * The license response was empty, fields are missing or otherwise unable
     * to be parsed.
     */
    LICENSE_PARSE_ERROR,
    /**
     * The operation (e.g. to renew or persist a license) is prohibited by the
     * license policy.
     */
    LICENSE_POLICY_ERROR,
    /**
     * Failed to generate a release request because a field in the stored
     * license is empty or malformed.
     */
    LICENSE_RELEASE_ERROR,
    /**
     * The license server detected an error in the license request.
     */
    LICENSE_REQUEST_REJECTED,
    /**
     * Failed to restore an offline license because a field is empty or
     * malformed.
     */
    LICENSE_RESTORE_ERROR,
    /**
     * License is in an invalid state for the attempted operation.
     */
    LICENSE_STATE_ERROR,
    /**
     * Certificate is malformed or is of the wrong type.
     */
    MALFORMED_CERTIFICATE,
    /**
     * Failure in the media framework.
     */
    MEDIA_FRAMEWORK_ERROR,
    /**
     * Certificate has not been set.
     */
    MISSING_CERTIFICATE,
    /**
     * There was an error loading the provisioned certificate.
     */
    PROVISIONING_CERTIFICATE_ERROR,
    /**
     * Required steps where not performed before provisioning was attempted.
     */
    PROVISIONING_CONFIGURATION_ERROR,
    /**
     * The provisioning response was empty, fields are missing or otherwise
     * unable to be parsed.
     */
    PROVISIONING_PARSE_ERROR,
    /**
     * The provisioning server detected an error in the provisioning request.
     */
    PROVISIONING_REQUEST_REJECTED,
    /**
     * Provisioning failed in a way that is likely to succeed on a subsequent
     * attempt.
     */
    RETRYABLE_PROVISIONING_ERROR,
    /**
     * Failed to generate a secure stop request because a field in the stored
     * license is empty or malformed.
     */
    SECURE_STOP_RELEASE_ERROR,
    /**
     * The plugin was unable to read data from the filesystem.
     */
    STORAGE_READ_FAILURE,
    /**
     * The plugin was unable to write data to the filesystem.
     */
    STORAGE_WRITE_FAILURE,
}
