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

package android.hardware.security.keymint;

import android.hardware.security.keymint.Certificate;
import android.hardware.security.keymint.KeyCharacteristics;

/**
 * This structure is returned when a new key is created with generateKey(), importKey() or
 * importWrappedKey().
 * @hide
 */
@VintfStability
parcelable KeyCreationResult {
    /**
     * `keyBlob` is an descriptor of the generated/imported key key.
     */
    byte[] keyBlob;

    /**
     * `keyCharacteristics` is a description of the generated key in the form of authorization lists
     * associated with security levels.  The rules that IKeyMintDevice implementations must use for
     * deciding whether a given tag from `keyParams` argument to the generation/import method should
     * be returned in `keyCharacteristics` are:
     *
     * - If the semantics of the tag are fully enforced by the IKeyMintDevice, without any
     *   assistance from components running at other security levels, it should be included in an
     *   entry with the SecurityLevel of the IKeyMintDevice.
     * - If the semantics of the tag are fully enforced, but with the assistance of components
     *   running at another SecurityLevel, it should be included in an entry with the minimum
     *   SecurityLevel of the involved components.  For example if a StrongBox IKeyMintDevice relies
     *   on a TEE to validate biometric authentication, biometric authentication tags go in an entry
     *   with SecurityLevel::TRUSTED_ENVIRONMENT.
     * - If the semantics are not enforced by KeyMint at all, SecurityLevel::KEYSTORE is used to
     *   indicate that Keystore should enforce.  Note that in Keymaster (predecessor to KeyMint),
     *   these tags would have been in SecurityLevel::SOFTWARE.
     */
    KeyCharacteristics[] keyCharacteristics;

    /**
     * If the generated/imported key is an asymmetric key, `certificateChain` will contain a chain
     * of one or more certificates.
     *
     * There are a few variations in what is contained in `certificateChain`, depending on whether
     * the caller requested attestation, whether they provided an attestation key (via the
     * `attestationKey` parameter of `generateKey()`, `importKey()` or `importWrappedKey()`), and in
     * the non-attestaion case, whether the key can self-sign.
     *
     * 1.  Asymmetric key attestation with factory key.  If Tag::ATTESTATION_CHALLENGE is provided
     *     and the `attestationKey` parameter on the generate/import call is null, the returned
     *     certificate chain must contain an attestation certificate signed with a factory-
     *     provisioned attestation key, and the full certificate chain for that factory-provisioned
     *     attestation key.  Tag::ATTESTATION_APPLICATION_ID must also be provided when the
     *     ATTESTATION_CHALLENGE is provided, otherwise ATTESTATION_APPLICATION_ID_MISSING will be
     *     returned.
     *
     * 2.  Asymmetric key attestation with caller-provided key.  If Tag::ATTESTATION_CHALLENGE is
     *     provided and the `attestationKey` parameter on the generat/import call is non-null and
     *     contains the key blob of a key with KeyPurpose::ATTEST_KEY, the returned certificate
     *     chain must contain only an attestation certificate signed with the specified key.  The
     *     caller must know the certificate chain for the provided key.  Tag::
     *     ATTESTATION_APPLICATION_ID must also be provided when the ATTESTATION_CHALLENGE is
     *     provided, otherwise ATTESTATION_APPLICATION_ID_MISSING will be returned.
     *
     * 3.  Asymmetric key non-attestation with signing key.  If Tag::ATTESTATION_CHALLENGE is not
     *     provided and the generated/imported key has KeyPurpose::SIGN, then the returned
     *     certificate chain must contain only a single self-signed certificate with no attestation
     *     extension.  Tag::ATTESTATION_APPLICATION_ID will be ignored if provided.
     *
     * 4.  Asymmetric key non-attestation with non-signing key.  If TAG::ATTESTATION_CHALLENGE is
     *     not provided and the generated/imported key does not have KeyPurpose::SIGN, then the
     *     returned certificate chain must contain only a single certificate with an empty signature
     *     and no attestation extension.  Tag::ATTESTATION_APPLICATION_ID will be ignored if
     *     provided.
     *
     * 5.  Symmetric key.  If the generated/imported key is symmetric, the certificate chain must
     *     return empty, any Tag::ATTESTATION_CHALLENGE or Tag::ATTESTATION_APPLICATION_ID inputs,
     *     if provided, are ignored.
     */
    Certificate[] certificateChain;
}
