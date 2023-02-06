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

package android.hardware.cas;

/**
 * @hide
 */
@VintfStability
parcelable Status {
    /**
     * The CAS plugin must return OK when an operation completes without any
     * errors.
     */
    const int OK = 0;

    /**
     * The CAS plugin must return ERROR_CAS_NO_LICENSE, when descrambling is
     * attempted and no license keys have been provided.
     */
    const int ERROR_CAS_NO_LICENSE = 1;

    /**
     * ERROR_CAS_LICENSE_EXPIRED must be returned when an attempt is made
     * to use a license and the keys in that license have expired.
     */
    const int ERROR_CAS_LICENSE_EXPIRED = 2;

    /**
     * The CAS plugin must return ERROR_CAS_SESSION_NOT_OPENED when an
     * attempt is made to use a session that has not been opened.
     */
    const int ERROR_CAS_SESSION_NOT_OPENED = 3;

    /**
     * The CAS plugin must return ERROR_CAS_CANNOT_HANDLE when an unsupported
     * data format or operation is attempted.
     */
    const int ERROR_CAS_CANNOT_HANDLE = 4;

    /**
     * ERROR_CAS_INVALID_STATE must be returned when the device is in a state
     * where it is not able to perform descrambling.
     */
    const int ERROR_CAS_INVALID_STATE = 5;

    /**
     * The CAS plugin must return BAD_VALUE whenever an illegal parameter is
     * passed to one of the interface functions.
     */
    const int BAD_VALUE = 6;

    /**
     * The CAS plugin must return ERROR_CAS_NOT_PROVISIONED when the device
     * has not yet been provisioned.
     */
    const int ERROR_CAS_NOT_PROVISIONED = 7;

    /**
     * ERROR_CAS_RESOURCE_BUSY must be returned when resources, such as CAS
     * sessions or secure buffers are not available to perform a requested
     * operation because they are already in use.
     */
    const int ERROR_CAS_RESOURCE_BUSY = 8;

    /**
     * The CAS Plugin must return ERROR_CAS_INSUFFICIENT_OUTPUT_PROTECTION
     * when the output protection level enabled on the device is not
     * sufficient to meet the requirements in the license policy. HDCP is an
     * example of a form of output protection.
     */
    const int ERROR_CAS_INSUFFICIENT_OUTPUT_PROTECTION = 9;

    /**
     * The CAS Plugin must return ERROR_CAS_TAMPER_DETECTED if an attempt to
     * tamper with the CAS system is detected.
     */
    const int ERROR_CAS_TAMPER_DETECTED = 10;

    /**
     * The CAS Plugin must return ERROR_CAS_DEVICE_REVOKED if the response
     * indicates that the device has been revoked. Device revocation means
     * that the device is no longer permitted to play content.
     */
    const int ERROR_CAS_DEVICE_REVOKED = 11;

    /**
     * The CAS plugin must return ERROR_CAS_DECRYPT_UNIT_NOT_INITIALIZED when
     * descrambling is failing because the session is not initialized properly.
     */
    const int ERROR_CAS_DECRYPT_UNIT_NOT_INITIALIZED = 12;

    /**
     * The CAS Plugin must return ERROR_CAS_DECRYPT if the DescramblerPlugin's
     * descramble operation fails.
     */
    const int ERROR_CAS_DECRYPT = 13;

    /**
     * ERROR_CAS_UNKNOWN must be returned when a fatal failure occurs and no
     * other defined error is appropriate.
     */
    const int ERROR_CAS_UNKNOWN = 14;

    /**
     * ERROR_CAS_NEED_ACTIVATION is used to trigger device activation process.
     */
    const int ERROR_CAS_NEED_ACTIVATION = 15;

    /**
     * ERROR_CAS_NEED_PAIRING is used to trigger pairing process.
     */
    const int ERROR_CAS_NEED_PAIRING = 16;

    /**
     * ERROR_CAS_NO_CARD is used to report no smart card for descrambling.
     */
    const int ERROR_CAS_NO_CARD = 17;

    /**
     * ERROR_CAS_CARD_MUTE is used to report smart card is muted for
     * descrambling.
     */
    const int ERROR_CAS_CARD_MUTE = 18;

    /**
     *  ERROR_CAS_CARD_INVALID is used to report smart card isn't valid.
     */
    const int ERROR_CAS_CARD_INVALID = 19;

    /**
     *  ERROR_CAS_BLACKOUT is used to report geographical blackout.
     */
    const int ERROR_CAS_BLACKOUT = 20;

    /**
     * ERROR_CAS_REBOOTING is used to report CAS is during rebooting.
     */
    const int ERROR_CAS_REBOOTING = 21;
}
