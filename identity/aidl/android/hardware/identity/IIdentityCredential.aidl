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
import android.hardware.keymaster.HardwareAuthToken;

@VintfStability
interface IIdentityCredential {
    /**
     * Delete a credential.
     *
     * This method returns a COSE_Sign1 data structure signed by CredentialKey
     * with payload set to the ProofOfDeletion CBOR below:
     *
     *     ProofOfDeletion = [
     *          "ProofOfDeletion",            ; tstr
     *          tstr,                         ; DocType
     *          bool                          ; true if this is a test credential, should
     *                                        ; always be false.
     *     ]
     *
     * After this method has been called, the persistent storage used for credentialData should
     * be deleted.
     *
     * @return a COSE_Sign1 signature described above.
     */
    byte[] deleteCredential();

    /**
     * Creates an ephemeral EC key pair, for use in establishing a seceure session with a reader.
     * This method returns the private key so the caller can perform an ECDH key agreement operation
     * with the reader.  The reason for generating the key pair in the secure environment is so that
     * the secure environment knows what public key to expect to find in the session transcript.
     *
     * The generated key must be an EC key using the P-256 curve.
     *
     * This method may only be called once per instance. If called more than once, STATUS_FAILED
     * will be returned.
     *
     * @return the unencrypted key-pair in PKCS#8 format.
     */
    byte[] createEphemeralKeyPair();

    /**
     * Sets the public part of the reader's ephemeral key pair.
     *
     * This method may only be called once per instance. If called more than once, STATUS_FAILED
     * will be returned.
     *
     * @param publicKey contains the reader's ephemeral public key, in uncompressed
     *        form (e.g. 0x04 || X || Y).
     */
    void setReaderEphemeralPublicKey(in byte[] publicKey);

    /**
     * Creates a challenge value to be used for proving successful user authentication. This
     * is included in the authToken passed to the startRetrieval() method.
     *
     * This method may only be called once per instance. If called more than once, STATUS_FAILED
     * will be returned.
     *
     * @return challenge, a non-zero number.
     */
    long createAuthChallenge();

    /**
     * Start an entry retrieval process.
     *
     * This method be called after createEphemeralKeyPair(), setReaderEphemeralPublicKey(),
     * createAuthChallenge() and before startRetrieveEntry(). This method call is followed by
     * multiple calls of startRetrieveEntryValue(), retrieveEntryValue(), and finally
     * finishRetrieval().
     *
     * It is permissible to perform data retrievals multiple times using the same instance (e.g.
     * startRetrieval(), then multiple calls of startRetrieveEntryValue(), retrieveEntryValue(),
     * then finally finishRetrieval()) but if this is done, the sessionTranscript parameter
     * must be identical for each startRetrieval() invocation. If this is not the case, this call
     * fails with the STATUS_SESSION_TRANSCRIPT_MISMATCH error.
     *
     * If the provided authToken is not valid this method fails with STATUS_INVALID_AUTH_TOKEN.
     *
     * Each of the provided accessControlProfiles is checked in this call. If they are not
     * all valid, the call fails with STATUS_INVALID_DATA.
     *
     * For the itemsRequest parameter, the content can be defined in the way appropriate for
     * the credential, but there are three requirements that must be met to work with this HAL:
     *
     *  1. The content must be a CBOR-encoded structure.
     *  2. The CBOR structure must be a map.
     *  3. The map must contain a tstr key "nameSpaces" whose value contains a map, as described in
     *     the example below.
     *
     * If these requirements are not met the startRetrieval() call fails with
     * STATUS_INVALID_ITEMS_REQUEST_MESSAGE.
     *
     * Here's an example of ItemsRequest CBOR which conforms to this requirement:
     *
     *   ItemsRequest = {
     *     ? "docType" : DocType,
     *       "nameSpaces" : NameSpaces,
     *     ? "requestInfo" : {* tstr => any}   ; Additional info the reader wants to provide
     *   }
     *
     *   DocType = tstr
     *
     *   NameSpaces = {
     *     + NameSpace => DataElements    ; Requested data elements for each NameSpace
     *   }
     *
     *   NameSpace = tstr
     *
     *   DataElements = {
     *     + DataElement => IntentToRetain
     *   }
     *
     *   DataElement = tstr
     *   IntentToRetain = bool
     *
     * For the readerSignature parameter, this can either be empty or if non-empty it
     * must be a COSE_Sign1 structure with an ECDSA signature over the content of the
     * CBOR conforming to the following CDDL:
     *
     *     ReaderAuthentication = [
     *       "ReaderAuthentication",
     *       SessionTranscript,
     *       ItemsRequestBytes
     *     ]
     *
     *     SessionTranscript = [
     *       DeviceEngagementBytes,
     *       EReaderKeyBytes
     *     ]
     *
     *     DeviceEngagementBytes = #6.24(bstr .cbor DeviceEngagement)
     *     EReaderKeyBytes = #6.24(bstr .cbor EReaderKey.Pub)
     *     ItemsRequestBytes = #6.24(bstr .cbor ItemsRequest)
     *
     *     EReaderKey.Pub = COSE_Key    ; Ephemeral public key provided by reader
     *
     * The public key corresponding to the key used to made signature, can be found in the
     * 'x5chain' unprotected header element of the COSE_Sign1 structure (as as described
     * in 'draft-ietf-cose-x509-04'). There will be at least one certificate in said element
     * and there may be more (and if so, each certificate must be signed by its successor).
     * This is checked and if the check fails the call fails with
     * STATUS_READER_SIGNATURE_CHECK_FAILED.
     *
     * The SessionTranscript CBOR is conveyed in the sessionTranscript parameter. It
     * is permissible for this to be empty in which case the readerSignature parameter
     * must also be empty. If this is not the case, the call fails with STATUS_FAILED.
     *
     * If the SessionTranscript CBOR is not empty, the X and Y coordinates of the public
     * part of the key-pair previously generated by createEphemeralKeyPair() must appear
     * somewhere in the bytes of DeviceEngagement structure. Both X and Y should be in
     * uncompressed form. If this is not satisfied, the call fails with
     * STATUS_EPHEMERAL_PUBLIC_KEY_NOT_FOUND.
     *
     * @param accessControlProfiles
     *   Access control profiles that are required to retrieve the entries that are going to be
     *   requested with IIdentityCredential.retrieveEntryValue(). See above.
     *
     * @param authToken
     *   The authentication token that proves the user was authenticated, as required
     *   by one or more of the provided accessControlProfiles. See above.
     *
     * @param itemsRequest
     *   If non-empty, contains request data that is signed by the reader. See above.
     *
     * @param signingKeyBlob is either empty or a signingKeyBlob (see generateSigningKeyPair(),
     *    below) containing the signing key to use to sign the data retrieved. If this
     *    is not in the right format the call fails with STATUS_INVALID_DATA.
     *
     * @param sessionTranscript
     *   Either empty or the CBOR of the SessionTranscript. See above.
     *
     * @param readerSignature
     *   readerSignature is either empty or contains a CBOR_Sign1 structure. See above.
     *
     * @param requestCounts
     *   requestCounts specifies the number of data items that are going to be requested, per
     *   namespace.  The number of elements in the vector must be the number of namespaces for which
     *   data items will be requested in retrieveEntryValue() calls, and the values of the elments
     *   must be the number of items from each namespace, in order, that will be requested in
     *   retrieveEntryValue() calls.
     *   Note that it's the caller's responsibility to ensure that the counts correspond to the
     *   retrieveEntryValue() calls that will be made, and that every retrieveEntryValue() call
     *   will succeed (i.e. that the access control profile checks will succeed).  This means that
     *   it's the responsibility of the caller to determine which access control checks will fail
     *   and remove the corresponding requests from the counts.
     */
    void startRetrieval(in SecureAccessControlProfile[] accessControlProfiles,
        in HardwareAuthToken authToken, in byte[] itemsRequest, in byte[] signingKeyBlob,
        in byte[] sessionTranscript, in byte[] readerSignature, in int[] requestCounts);

    /**
     * Starts retrieving an entry, subject to access control requirements.  Entries must be
     * retrieved in namespace groups as specified in the requestCounts parameter.
     *
     * If the requestData parameter as passed to startRetrieval() was non-empty
     * this method must only be called with entries specified in that field. If this
     * requirement is not met, the call fails with STATUS_NOT_IN_REQUEST_MESSAGE.
     *
     * If nameSpace or name is empty this call fails with STATUS_INVALID_DATA.
     *
     * Each access control profile for the entry is checked. If user authentication
     * is required and the supplied auth token doesn't provide it the call fails
     * with STATUS_USER_AUTHENTICATION_FAILED. If reader authentication is required and
     * a suitable reader certificate chain isn't presented, the call fails with
     * STATUS_READER_AUTHENTICATION_FAILED.
     *
     * It is permissible to keep retrieving values if an access control check fails.
     *
     * @param nameSpace is the namespace of the element, e.g. "org.iso.18013.5.1"
     *
     * @param name is the name of the element, e.g. "driving_privileges".
     *
     * @param entrySize is the size of the entry value encoded in CBOR.
     *
     * @param accessControlProfileIds specifies the set of access control profiles that can
     *     authorize access to the provisioned element. If an identifier of a profile
     *     is given and this profile wasn't passed to startRetrieval() this call fails
     *     with STATUS_INVALID_DATA.
     */
    void startRetrieveEntryValue(in @utf8InCpp String nameSpace, in @utf8InCpp String name,
                                 in int entrySize, in int[] accessControlProfileIds);

    /**
     * Retrieves an entry value, or part of one, if the entry value is larger than gcmChunkSize.
     * May only be called after startRetrieveEntry().
     *
     * If the passed in data is not authentic, can't be decrypted, is of the wrong size, or can't
     * be decoded, this call fails with STATUS_INVALID_DATA.
     *
     * @param encryptedContent contains the encrypted and MACed content.
     *
     * @return the entry value as CBOR, or part of the entry value in the case the
     *    content exceeds gcmChunkSize in length.
     */
    byte[] retrieveEntryValue(in byte[] encryptedContent);

    /**
     * End retrieval of data, optionally returning a message authentication code over the
     * returned data.
     *
     * If signingKeyBlob or the sessionTranscript parameter passed to startRetrieval() is
     * empty then the returned MAC will be empty.
     *
     * @param out mac is empty if signingKeyBlob or the sessionTranscript passed to
     *    startRetrieval() is empty. Otherwise it is a COSE_Mac0 with empty payload
     *    and the detached content is set to DeviceAuthentication as defined below.
     *    This code is produced by using the key agreement and key derivation function
     *    from the ciphersuite with the authentication private key and the reader
     *    ephemeral public key to compute a shared message authentication code (MAC)
     *    key, then using the MAC function from the ciphersuite to compute a MAC of
     *    the authenticated data. See section 9.2.3.5 of ISO/IEC 18013-5 for details
     *    of this operation.
     *
     *        DeviceAuthentication = [
     *            "DeviceAuthentication",
     *            SessionTranscript,
     *            DocType,
     *            DeviceNameSpaceBytes,
     *        ]
     *
     *        DocType = tstr
     *
     *        SessionTranscript = [
     *            DeviceEngagementBytes,
     *            EReaderKeyBytes
     *        ]
     *
     *        DeviceEngagementBytes = #6.24(bstr .cbor DeviceEngagement)
     *        EReaderKeyBytes = #6.24(bstr .cbor EReaderKey.Pub)
     *        DeviceNameSpacesBytes = #6.24(bstr .cbor DeviceNameSpaces)
     *
     *    where
     *
     *        DeviceNameSpaces = {
     *            * NameSpace => DeviceSignedItems
     *        }
     *        DeviceSignedItems = {
     *            + DataItemName => DataItemValue
     *        }
     *
     *        Namespace = tstr
     *        DataItemName = tstr
     *        DataItemValue = any
     *
     *
     * @param out deviceNameSpaces the bytes of DeviceNameSpaces.
     */
    void finishRetrieval(out byte[] mac, out byte[] deviceNameSpaces);

    /**
     * Generate a key pair to be used for signing session data and retrieved data items.
     *
     * The generated key must be an EC key using the P-256 curve.
     *
     * This method shall return just a single X.509 certificate which is signed by CredentialKey.
     * When combined with the certificate chain returned at provisioning time by
     * getAttestationCertificate() on IWritableIdentityCredential (for the credential key), this
     * forms a chain all the way from the root of trust to the generated key.
     *
     * The public part of a signing key is usually included in issuer-signed data and is
     * used for anti-cloning purposes or as a mechanism for the issuer to attest to data
     * generated on the device.
     *
     * The following non-optional fields for the X.509 certificate shall be set as follows:
     *
     *  - version: INTEGER 2 (means v3 certificate).
     *
     *  - serialNumber: INTEGER 1 (fixed value: same on all certs).
     *
     *  - signature: must be set to ECDSA.
     *
     *  - subject: CN shall be set to "Android Identity Credential Authentication Key".
     *
     *  - issuer: shall be set to "credentialStoreName (credentialStoreAuthorName)" using the
     *    values returned in HardwareInformation.
     *
     *  - validity: should be from current time and one year in the future.
     *
     *  - subjectPublicKeyInfo: must contain attested public key.
     *
     * @param out signingKeyBlob contains an encrypted copy of the newly-generated private
     *     signing key.
     *
     * @return an X.509 certificate for the new signing key, signed by the credential key.
     */
    Certificate generateSigningKeyPair(out byte[] signingKeyBlob);
}
