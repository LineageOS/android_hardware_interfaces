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
import android.hardware.identity.SecureAccessControlProfile;

/**
 * IWritableIdentityCredential is used to personalize a new identity credential.  Credentials cannot
 * be updated or modified after creation; any changes require deletion and re-creation.
 */
@VintfStability
interface IWritableIdentityCredential {
    /**
     * Gets the certificate chain for credentialKey which can be used to prove the hardware
     * characteristics to an issuing authority.  Must not be called more than once.
     *
     * The certificate chain must be generated using Keymaster Attestation
     * (see https://source.android.com/security/keystore/attestation) with the
     * following additional requirements:
     *
     *  - The attestationVersion field in the attestation extension must be at least 3.
     *
     *  - The attestationSecurityLevel field must be set to either Software (0),
     *    TrustedEnvironment (1), or StrongBox (2) depending on how attestation is
     *    implemented. Only the default AOSP implementation of this HAL may use
     *    value 0 (additionally, this implementation must not be used on production
     *    devices).
     *
     *  - The keymasterVersion field in the attestation extension must be set to (10*major + minor)
     *    where major and minor are the Identity Credential interface major and minor versions.
     *    Specifically for this version of the interface (1.0) this value is 10.
     *
     *  - The keymasterSecurityLevel field in the attestation extension must be set to
     *    either Software (0), TrustedEnvironment (1), or StrongBox (2) depending on how
     *    the Trusted Application backing the HAL implementation is implemented. Only
     *    the default AOSP implementation of this HAL may use value 0 (additionally, this
     *    implementation must not be used on production devices)
     *
     *  - The attestationChallenge field must be set to the passed-in challenge.
     *
     *  - The uniqueId field must be empty.
     *
     *  - The softwareEnforced field in the attestation extension must include
     *    Tag::ATTESTATION_APPLICATION_ID which must be set to the bytes of the passed-in
     *    attestationApplicationId.
     *
     *  - The teeEnforced field in the attestation extension must include
     *
     *    - Tag::IDENTITY_CREDENTIAL_KEY which indicates that the key is an Identity
     *      Credential key (which can only sign/MAC very specific messages) and not an Android
     *      Keystore key (which can be used to sign/MAC anything).
     *
     *    - Tag::PURPOSE must be set to SIGN
     *
     *    - Tag::KEY_SIZE must be set to the appropriate key size, in bits (e.g. 256)
     *
     *    - Tag::ALGORITHM must be set to EC
     *
     *    - Tag::NO_AUTH_REQUIRED must be set
     *
     *    - Tag::DIGEST must be set to SHA_2_256
     *
     *    - Tag::EC_CURVE must be set to P_256
     *
     * Additional authorizations may be needed in the softwareEnforced and teeEnforced
     * fields - the above is not an exhaustive list. Specifically, authorizations containing
     * information about the root of trust, OS version, verified boot state, and so on should
     * be included.
     *
     * Since the chain is required to be generated using Keymaster Attestation, the returned
     * certificate chain has the following properties:
     *
     *  - The certificate chain is of at least length three.
     *
     *  - The root of trust is the same as for Keymaster Attestation. This is usually
     *    a certificate owned by Google but depending on the specific Android device it may
     *    be another certificate.
     *
     * As with any user of attestation, the Issuing Authority (as a relying party) wishing
     * to issue a credential to a device using these APIs, must carefully examine the
     * returned certificate chain for all of the above (and more). In particular, the Issuing
     * Authority should check the root of trust, verified boot state, patch level,
     * application id, etc.
     *
     * This all depends on the needs of the Issuing Authority and the kind of credential but
     * in general an Issuing Authority should never issue a credential to a device without
     * verified boot enabled, to an unrecognized application, or if it appears the device
     * hasn't been updated for a long time.
     *
     * See https://github.com/google/android-key-attestation for an example of how to
     * examine attestations generated from Android devices.
     *
     * @param attestationApplicationId is the DER encoded value to be stored
     *     in Tag::ATTESTATION_APPLICATION_ID. This schema is described in
     *     https://developer.android.com/training/articles/security-key-attestation#certificate_schema_attestationid
     *
     * @param attestationChallenge a challenge set by the issuer to ensure freshness.
     *
     * @return the X.509 certificate chain for the credentialKey
     */
    Certificate[] getAttestationCertificate(in byte[] attestationApplicationId, in byte[] attestationChallenge);

    /**
     * Start the personalization process.
     *
     * startPersonalization must not be called more than once.
     *
     * @param accessControlProfileCount specifies the number of access control profiles that will
     *     be provisioned with addAccessControlProfile().
     *
     * @param entryCounts specifies the number of data entries that will be provisioned with
     *     beginAddEntry() and addEntry(). Each item in the array specifies how many entries
     *     will be added for each name space.
     */
    void startPersonalization(in int accessControlProfileCount, in int[] entryCounts);

    /**
     * Add an access control profile, which defines the requirements or retrieval of one or more
     * entries.  If both readerCertificate and userAuthenticationRequired are empty/false,
     * associated entries are open access, requiring no authentication to read (though the caller
     * is free to require other authentication above this HAL).
     *
     * This method must be called exactly as many times as specified in the startPersonalization()
     * accessControlProfileCount parameter. If this is requirement is not met, the method fails
     * with STATUS_INVALID_DATA.
     *
     * @param id a numeric identifier that must be unique within the context of a Credential and may
     *     be used to reference the profile. If this is not satisfied the call fails with
     *     STATUS_INVALID_DATA.
     *
     * @param readerCertificate if non-empty, specifies a single X.509 certificate (not a chain of
     *     certificates) that must be used to authenticate requests (see the readerSignature
     *     parameter in IIdentityCredential.startRetrieval).
     *
     * @param userAuthenticationRequired if true, specifies that the user is required to
     *     authenticate to allow requests.  Required authentication freshness is specified by
     *     timeout below.
     *
     * @param timeoutMillis specifies the amount of time, in milliseconds, for which a user
     *     authentication (see userAuthenticationRequired above) is valid, if
     *     userAuthenticationRequired is true. If the timout is zero then authentication is
     *     required for each reader session. If userAuthenticationRequired is false, the timeout
     *     must be zero. If this requirement is not met the call fails with STATUS_INVALID_DATA.
     *
     * @param secureUserId must be non-zero if userAuthenticationRequired is true. It is not
     *     related to any Android user ID or UID, but is created in the Gatekeeper application
     *     in the secure environment. If this requirement is not met the call fails with
     *     STATUS_INVALID_DATA.
     *
     * @return a structure with the passed-in data and MAC created with storageKey for authenticating
     *     the data at a later point in time.
     */
    SecureAccessControlProfile addAccessControlProfile(in int id, in Certificate readerCertificate,
        in boolean userAuthenticationRequired, in long timeoutMillis, in long secureUserId);

    /**
     * Begins the process of adding an entry to the credential.  All access control profiles must be
     * added before calling this method.  Entries must be added in namespace "groups", meaning all
     * entries of one namespace must be added before adding entries from another namespace.
     *
     * This method must be called exactly as many times as the sum of the items in the entryCounts
     * parameter specified in the startPersonalization(), and must be followed by one or more calls
     * to addEntryValue(). If this requirement is not met the method fails with STATUS_INVALID_DATA.
     *
     * @param accessControlProfileIds specifies the set of access control profiles that can
     *     authorize access to the provisioned element.
     *
     * @param nameSpace is the namespace of the element, e.g. "org.iso.18013.5.1"
     *
     * @param name is the name of the element.
     *
     * @param entrySize is the size of the entry value. If this requirement
     *     is not met this method fails with STATUS_INVALID_DATA.
     */
    void beginAddEntry(in int[] accessControlProfileIds, in @utf8InCpp String nameSpace,
        in @utf8InCpp String name, in int entrySize);

    /**
     * Continues the process of adding an entry, providing a value or part of a value.
     *
     * In the common case, this method will be called only once per entry added.  In the case of
     * values that are larger than the secure environment's GCM chunk size
     * (see IIdentityCredentialStore.getHardwareInformation()), the caller must provide the
     * value in chunks.  All chunks must be exactly gcmChunkSize except the last and the sum of all
     * chunk sizes must equal the value of the beginAddEntry() entrySize argument. If this
     * requirement is not met the call fails with STATUS_INVALID_DATA.
     *
     * @param content is the entry value, encoded as CBOR. In the case the content exceeds gcmChunkSize,
     *     this may be partial content up to gcmChunkSize bytes long.
     *
     * @return the encrypted and MACed content.  For directly-available credentials the contents are
     *     implementation-defined. For other credentials, the result contains
     *
     *         AES-GCM-ENC(storageKey, R, Data, AdditionalData)
     *
     *     where:
     *
     *         Data = any   ; value
     *
     *         AdditionalData = {
     *             "Namespace" : tstr,
     *             "Name" : tstr,
     *             "AccessControlProfileIds" : [ + uint ],
     *         }
     */
    byte[] addEntryValue(in byte[] content);

    /**
     * Finishes adding entries and returns a signature that an issuing authority may use to
     * validate that all data was provisioned correctly.
     *
     * After this method is called, the IWritableIdentityCredential is no longer usable.
     *
     * @param out credentialData is a CBOR-encoded structure (in CDDL notation):
     *
     *         CredentialData = [
     *              tstr,   ; docType, an optional name that identifies the type of credential
     *              bool,   ; testCredential, indicates if this is a test credential
     *              bstr    ; an opaque byte vector with encrypted data, see below
     *         ]
     *
     *     The last element is an opaque byte vector which contains encrypted copies of the
     *     secrets used to secure the new credential's data and to authenticate the credential to
     *     the issuing authority.  It contains:
     *
     *         AES-GCM-ENC(HBK, R, CredentialKeys, docType)
     *
     *     where HBK is a unique hardware-bound key that has never existed outside of the secure
     *     environment (except it's all zeroes if testCredential is True) and CredentialKeys is
     *     the CBOR-encoded structure (in CDDL notation):
     *
     *         CredentialKeys = [
     *              bstr,   ; storageKey, a 128-bit AES key
     *              bstr    ; credentialPrivKey, the private key for credentialKey
     *         ]
     *
     * @param out proofOfProvisioningSignature proves to the IA that the credential was imported
     *     into the secure hardware without alteration or error.  When the final addEntry() call is
     *     made (when the number of provisioned entries equals the sum of the items in
     *     startPersonalization() entryCounts parameter), a COSE_Sign1 structure
     *     signed by CredentialKey with payload set to the ProofOfProvisioning CBOR below:
     *
     *          ProofOfProvisioning = [
     *              "ProofOfProvisioning",
     *              tstr,                         ; DocType
     *              [ * AccessControlProfile ],
     *              ProvisionedData,
     *              bool                          ; true if this is a test credential, should
     *                                            ; always be false.
     *          ]
     *
     *          AccessControlProfile = {
     *              "id" : uint,
     *              ? "readerCertificate" : bstr,
     *              ? (
     *                  "userAuthenticationRequired" : bool,
     *                  "timeoutMillis" : uint,
     *              )
     *          }
     *
     *          ProvisionedData = {
     *              * Namespace => [ + Entry ]
     *          },
     *
     *          Namespace = tstr
     *
     *          Entry = {
     *              "name" : tstr,
     *              "value" : any,
     *              "accessControlProfiles" : [ * uint ],
     *          }
     */
    void finishAddingEntries(out byte[] credentialData,
        out byte[] proofOfProvisioningSignature);
}
