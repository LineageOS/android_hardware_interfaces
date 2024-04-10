/*
 * Copyright 2023 The Android Open Source Project
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
package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.IOpaqueKey;
import android.hardware.security.see.hwcrypto.KeyPolicy;

/*
 * Higher level interface to access and generate keys.
 */
interface IHwCryptoKey {
    /*
     * Identifier for the requested device provided key. The currently supported identifiers are:
     *
     * DEVICE_BOUND_KEY:
     *      This is a key unique to the device.
     * BATCH_KEY:
     *      This is a shared by a set of devices.
     */
    enum DeviceKeyId {
        DEVICE_BOUND_KEY,
        BATCH_KEY,
    }
    union DiceBoundDerivationKey {
        /*
         * Opaque to be used to derive the DICE bound key.
         */
        IOpaqueKey opaqueKey;

        /*
         * Device provided key to be used to derive the DICE bound key.
         */
        DeviceKeyId keyId;
    }

    parcelable DiceCurrentBoundKeyResult {
        /*
         * Key cryptographically bound to a DICE policy.
         */
        IOpaqueKey diceBoundKey;

        /*
         * Current dice policy which was used to generate the returned key. This policy is
         * opaque from this service perspective (it will be sent to an Authentication Manager
         * Service to be verified). It follows the structure defined on DicePolicy.cddl, located
         * under hardware/interfaces/security/authgraph/aidl/android/hardware/security/authgraph/
         * with the caveat that it could be encrypted if the client does not have enough permissions
         * to see the device dice policy information.
         */
        byte[] dicePolicyForKeyVersion;
    }

    parcelable DiceBoundKeyResult {
        /*
         * Key cryptographically bound to a DICE policy.
         */
        IOpaqueKey diceBoundKey;

        /*
         * Indicates if the diceBoundKey returned was created using a current DICE policy. The
         * caller can use this to detect if an old policy was provided and rotate its keys if so
         * desired. Old, valid policies remain usable, but care needs to be taken to not continue to
         * use a potentially compromised key.
         */
        boolean dicePolicyWasCurrent;
    }

    parcelable ClearKeyPolicy {
        /*
         * Indicates the desired key size. It will be used to calculate how many bytes of key
         * material should be returned.
         */
        int keySizeBytes;
    }

    union DerivedKeyPolicy {
        /*
         * Policy for the newly derived opaque key. Defines how the key can be used and its type.
         */
        KeyPolicy opaqueKey;

        /*
         * If used we will derive a clear key and pass it back as an array of bytes on
         * <code>HwCryptoKeyMaterial::explicitKey</code>.
         */
        ClearKeyPolicy clearKey;
    }

    parcelable DerivedKeyParameters {
        /*
         * Key to be used to derive the new key using HKDF.
         */
        IOpaqueKey derivationKey;

        /*
         * Policy for the newly derived key. Depending on its type, either a clear or opaque key
         * will be derived.
         */
        DerivedKeyPolicy keyPolicy;

        /*
         * An arbitrary set of bytes incorporated into the key derivation. May have
         * an implementation-specific maximum length, but it is guaranteed to accept
         * at least 32 bytes.
         */
        byte[] context;
    }

    union DerivedKey {
        /*
         * Derived key in clear format.
         */
        byte[] explicitKey = {};

        /*
         * Derived key as a key token to be used only through the HWCrypto service.
         */
        IOpaqueKey opaque;
    }

    /*
     * deriveCurrentDicePolicyBoundKey() - Derives a versioned key tied to the caller's current DICE
     *                              policy. It will return this current policy back to the caller
     *                              along with the generated key.
     *
     * @derivationKey:
     *     Key to be used to derive the new key using HKDF.
     *
     * Return:
     *      Ok(DiceCurrentBoundKeyResult) on success, service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     */
    DiceCurrentBoundKeyResult deriveCurrentDicePolicyBoundKey(
            in DiceBoundDerivationKey derivationKey);

    /*
     * deriveDicePolicyBoundKey() - Derive a versioned key by checking the provided DICE policy
     *                              against the caller and then using it as a context for deriving
     *                              the returned key.
     *
     * @derivationKey:
     *     Key to be used to derive the new key using HKDF.
     *
     * @dicePolicyForKeyVersion:
     *     Policy used to derive keys tied to specific versions. Using this parameter
     *     the caller can tie a derived key to a minimum version of itself, so in the future only
     *     itself or a more recent version can derive the same key. This parameter is opaque to the
     *     caller and it could be encrypted in the case the client doesn't have permission to know
     *     the dice chain.
     *     When implementing this function, this parameter shall be one of the components fed
     *     to the KDF context and it needs to be checked against the caller DICE certificate before
     *     being used.
     *
     * Return:
     *      Ok(DiceBoundKeyResult) on success, service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     */
    DiceBoundKeyResult deriveDicePolicyBoundKey(
            in DiceBoundDerivationKey derivationKey, in byte[] dicePolicyForKeyVersion);

    /*
     * deriveKey() - Derive a new key based on the given key, policy and context.
     *
     * @parameters:
     *      Parameters used for the key derivation. See <code>DerivedKeyParameters</code> on this
     *      file for more information.
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     */
    DerivedKey deriveKey(in DerivedKeyParameters parameters);
}
