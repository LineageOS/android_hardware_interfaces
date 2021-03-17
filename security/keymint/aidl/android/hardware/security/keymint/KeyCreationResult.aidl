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
     * of one or more certificates.  If the key parameters provided to the generate/import method
     * contains Tag::ATTESTATION_CHALLENGE the first certificate will contain an attestation
     * extension, and will be signed by a factory-installed attestation key and followed by a chain
     * of certificates leading to an authoritative root.  If there is no attestation challenge, only
     * one certificate will be returned, and it will be self-signed or contain a fake signature,
     * depending on whether the key has KeyPurpose::SIGN.  If the generated key is symmetric,
     * certificateChain will be empty.
     */
    Certificate[] certificateChain;
}
