/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.identity;

import android.hardware.identity.Certificate;

@VintfStability
parcelable SecureAccessControlProfile {
    /**
     * id is a numeric identifier that must be unique within the context of a Credential and may be
     * used to reference the profile.
     */
    int id;

    /**
     * readerCertificate, if non-empty, specifies a single X.509 certificate (not a chain
     * of certificates) that must be used to authenticate requests. For details about how
     * this is done, see the readerSignature parameter of IIdentityCredential.startRetrieval.
     */
    Certificate readerCertificate;

    /**
     * if true, the user is required to authenticate to allow requests.  Required authentication
     * fressness is specified by timeout below.
     *
     */
    boolean userAuthenticationRequired;

    /**
     * Timeout specifies the amount of time, in milliseconds, for which a user authentication (see
     * above) is valid, if userAuthenticationRequired is set to true.  If userAuthenticationRequired
     * is true and timout is zero then authentication is required for each reader session.
     *
     * If userAuthenticationRequired is false, timeout must be zero.
     */
    long timeoutMillis;

    /**
     * secureUserId must be non-zero if userAuthenticationRequired is true.
     * It is not related to any Android user ID or UID, but is created in the
     * Gatekeeper application in the secure environment.
     */
    long secureUserId;

    /**
     * The mac is used to authenticate the access control profile.  It contains:
     *
     *      AES-GCM-ENC(storageKey, R, {}, AccessControlProfile)
     *
     *  where AccessControlProfile is the CBOR map:
     *
     *      AccessControlProfile = {
     *          "id": uint,
     *          ? "readerCertificate" : bstr,
     *          ? (
     *              "userAuthenticationRequired" : bool,
     *              "timeoutMillis" : uint,
     *              "secureUserId" : uint
     *          )
     *      }
     */
    byte[] mac;
}

