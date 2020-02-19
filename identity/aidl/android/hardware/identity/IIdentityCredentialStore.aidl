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

import android.hardware.identity.IIdentityCredential;
import android.hardware.identity.IWritableIdentityCredential;
import android.hardware.identity.HardwareInformation;
import android.hardware.identity.CipherSuite;

/**
 * IIdentityCredentialStore provides an interface to a secure store for user identity documents.
 * This HAL is deliberately fairly general and abstract.  To the extent possible, specification of
 * the message formats and semantics of communication with credential verification devices and
 * issuing authorities (IAs) is out of scope for this HAL.  It provides the interface with secure
 * storage but a credential-specific Android application will be required to implement the
 * presentation and verification protocols and processes appropriate for the specific credential
 * type.
 *
 * The design of this HAL makes few assumptions about the underlying secure hardware.  In particular
 * it does not assume that the secure hardware has any storage beyond that needed for a persistent,
 * hardware-bound AES-128 key.  However, its design allows the use of secure hardware that does have
 * storage, specifically to enable "direct access".  Most often credentials will be accessed through
 * this HAL and through the Android layers above it but that requires that the Android device be
 * powered up and fully functional.  It is desirable to allow identity credential usage when the
 * Android device's battery is too low to boot the Android operating system, so direct access to the
 * secure hardware via NFC may allow data retrieval, if the secure hardware chooses to implement it.
 * Definition of how data is retrieved in low power mode is explicitly out of scope for this HAL
 * specification; it's up to the relevant identity credential standards to define.
 *
 * The 'default' HAL instance is explicitly not for direct access and the 'direct_access' HAL
 * instance - if available - is for direct access. Applications are expected to provision their
 * credential to both instances (and the contents may differ), not just one of them.
 *
 * Multiple credentials can be created.  Each credential comprises:
 *
 * - A document type, which is a UTF-8 string of at most 256 bytes.
 *
 * - A set of namespaces, which serve to disambiguate value names.  Namespaces are UTF-8 strings of
 *   up to 256 bytes in length (most should be much shorter).  It is recommended that namespaces be
 *   structured as reverse domain names so that IANA effectively serves as the namespace registrar.
 *
 * - For each namespase, a set of name/value pairs, each with an associated set of access control
 *   profile IDs.  Names are UTF-8 strings of up to 256 bytes in length (most should be much
 *   shorter).  Values stored must be encoed as valid CBOR (https://tools.ietf.org/html/rfc7049) and
 *   the encoeded size is is limited to at most 512 KiB.
 *
 * - A set of access control profiles, each with a profile ID and a specification of the
 *   conditions which satisfy the profile's requirements.
 *
 * - An asymmetric key pair which is used to authenticate the credential to the IA, called the
 *   CredentialKey. This key is attested to by the secure hardware using Android Keystore
 *   attestation (https://source.android.com/security/keystore/attestation). See
 *   getAttestationCertificate() in the IWritableIdentityCredential for more information.
 *
 * - A set of zero or more named reader authentication public keys, which are used to authenticate
 *   an authorized reader to the credential.
 *
 * - A set of named signing keys, which are used to sign collections of values and session
 *   transcripts.
 *
 * Cryptographic notation:
 *
 * Throughout this HAL, cryptographic operations are specified in detail.  To avoid repeating the
 * definition of the notation, it's specified here.  It is assumed that the reader is familiar with
 * standard cryptographic algorithms and constructs.
 *
 *     AES-GCM-ENC(K, N, D, A) represents AES-GCM encryption with key 'K', nonce 'N', additional
 *         authenticated data 'A' and data 'D'.  The nonce is usually specified as 'R', meaning 12
 *         random bytes.  The nonce is always 12 bytes and is prepended to the ciphertext. The GCM
 *         tag is 16 bytes, appended to the ciphertext.  AES-GCM-DEC with the same argument notation
 *         represents the corresponding decryption operation.
 *
 *    ECDSA(K, D) represents ECDSA signing of data 'D' with key 'K'.
 *
 *    || represents concatenation
 *
 *    {} represents an empty input; 0 bytes of data.
 *
 *    HBK represents a device-unique, hardware-bound AES-128 key which exists only in secure
 *        hardware, except for "test" credential stores (see createCredential(), below).  For test
 *        stores, an all-zero value is used in place of the HBK.
 *
 * Data encoding notation:
 *
 * Various fields need to be encoded as precisely-specified byte arrays.  Where existing standards
 * define appropriate encodings, those are used.  For example, X.509 certificates.  Where new
 * encodings are needed, CBOR is used.  CBOR maps are described in CDDL notation
 * (https://tools.ietf.org/html/draft-ietf-cbor-cddl-06).
 *
 * All binder calls in the HAL may return a ServiceSpecificException with statuses from the
 * STATUS_* integers defined in this interface. Each method states which status can be returned
 * and under which circumstances.
 */
@VintfStability
interface IIdentityCredentialStore {
    /**
     * Success.
     */
    const int STATUS_OK = 0;

    /**
     * The operation failed. This is used as a generic catch-all for errors that don't belong
     * in other categories, including memory/resource allocation failures and I/O errors.
     */
    const int STATUS_FAILED = 1;

    /**
     * Unsupported cipher suite.
     */
    const int STATUS_CIPHER_SUITE_NOT_SUPPORTED = 2;

    /**
     * The passed data was invalid. This is a generic catch all for errors that don't belong
     * in other categories related to parameter validation.
     */
    const int STATUS_INVALID_DATA = 3;

    /**
     * The authToken parameter passed to IIdentityCredential.startRetrieval() is not valid.
     */
    const int STATUS_INVALID_AUTH_TOKEN = 4;

    /**
     * The itemsRequest parameter passed to IIdentityCredential.startRetrieval() does not meet
     * the requirements described in the documentation for that method.
     */
    const int STATUS_INVALID_ITEMS_REQUEST_MESSAGE = 5;

    /**
     * The readerSignature parameter in IIdentityCredential.startRetrieval() is invalid,
     * doesn't contain an embedded certificate chain, or the signature failed to
     * validate.
     */
    const int STATUS_READER_SIGNATURE_CHECK_FAILED = 6;

    /**
     * The sessionTranscript passed to startRetrieval() did not contain the ephmeral public
     * key returned by createEphemeralPublicKey().
     */
    const int STATUS_EPHEMERAL_PUBLIC_KEY_NOT_FOUND = 7;

    /**
     * An access condition related to user authentication was not satisfied.
     */
    const int STATUS_USER_AUTHENTICATION_FAILED = 8;

    /**
     * An access condition related to reader authentication was not satisfied.
     */
    const int STATUS_READER_AUTHENTICATION_FAILED = 9;

    /**
     * The request data element has no access control profiles associated so it cannot be accessed.
     */
    const int STATUS_NO_ACCESS_CONTROL_PROFILES = 10;

    /**
     * The requested data element is not in the provided non-empty itemsRequest message.
     */
    const int STATUS_NOT_IN_REQUEST_MESSAGE = 11;

    /**
     * The passed-in sessionTranscript doesn't match the previously passed-in sessionTranscript.
     */
    const int STATUS_SESSION_TRANSCRIPT_MISMATCH = 12;

    /**
     * Returns information about hardware.
     *
     * @return a HardwareInformation with information about the hardware.
     */
    HardwareInformation getHardwareInformation();

    /**
     * createCredential creates a new Credential.  When a Credential is created, two cryptographic
     * keys are created: StorageKey, an AES key used to secure the externalized Credential
     * contents, and CredentialKeyPair, an EC key pair used to authenticate the store to the IA.  In
     * addition, all of the Credential data content is imported and a certificate for the
     * CredentialKeyPair and a signature produced with the CredentialKeyPair are created.  These
     * latter values may be checked by an issuing authority to verify that the data was imported
     * into secure hardware and that it was imported unmodified.
     *
     * @param docType is an optional name (may be an empty string) that identifies the type of
     *     credential being created, e.g. "org.iso.18013-5.2019.mdl" (the doc type of the ISO
     *     driving license standard).
     *
     * @param testCredential indicates if this is a test store.  Test credentials must use an
     *     all-zeros hardware-bound key (HBK) and must set the test bit in the
     *     personalizationReceipt (see finishAddingEntries(), in IWritableIdentityCredential).
     *
     * @return an IWritableIdentityCredential interface that provides operations to
     *     provision a credential.
     */
    IWritableIdentityCredential createCredential(in @utf8InCpp String docType,
                                                 in boolean testCredential);

    /**
     * getCredential retrieves an IIdentityCredential interface which allows use of a stored
     * Credential.
     *
     * The cipher suite used to communicate with the remote verifier must also be specified. Currently
     * only a single cipher-suite is supported and the details of this are as follow:
     *
     *  - ECDHE with HKDF-SHA-256 for key agreement.
     *  - AES-256 with GCM block mode for authenticated encryption (nonces are incremented by one
     *    for every message).
     *  - ECDSA with SHA-256 for signing (used for signing session transcripts to defeat
     *    man-in-the-middle attacks), signing keys are not ephemeral.
     *
     * Support for other cipher suites may be added in a future version of this HAL.
     *
     * This method fails with STATUS_INVALID_DATA if the passed in credentialData cannot be
     * decoded or decrypted.
     *
     * @param cipherSuite is the cipher suite to use.
     *
     * @param credentialData is a CBOR-encoded structure containing metadata about the credential
     *     and an encrypted byte array that contains data used to secure the credential.  See the
     *     return argument of the same name in finishAddingEntries(), in
     *     IWritableIdentityCredential.
     *
     * @return an IIdentityCredential HIDL interface that provides operations on the Credential.
     */
    IIdentityCredential getCredential(in CipherSuite cipherSuite, in byte[] credentialData);
}
