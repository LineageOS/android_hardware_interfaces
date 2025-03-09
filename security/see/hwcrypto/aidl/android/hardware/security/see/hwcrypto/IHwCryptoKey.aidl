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

import android.hardware.security.see.hwcrypto.IHwCryptoOperations;
import android.hardware.security.see.hwcrypto.IOpaqueKey;
import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.types.ExplicitKeyMaterial;
import android.hardware.security.see.hwcrypto.types.OpaqueKeyToken;

/*
 * Higher level interface to access and generate keys.
 */
@VintfStability
interface IHwCryptoKey {
    /*
     * Identifier for the requested device provided key. The currently supported identifiers are:
     *
     */
    enum DeviceKeyId {
        /*
         * This is a key unique to the device.
         */
        DEVICE_BOUND_KEY,
    }

    /*
     * Identifier for the requested key slot. The currently supported identifiers are:
     *
     */
    enum KeySlot {
        /*
         * This is the shared HMAC key that will now be computed by HwCryptoKey after participating
         * in the ISharedSecret protocol that can be shared with KeyMint and authenticators. See
         * ISharedSecret.aidl for more information.
         */
        KEYMINT_SHARED_HMAC_KEY,
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
         * Current dice policy which was used to generate the returned key. This policy is opaque
         * from this service perspective (it will be sent to an Authentication Manager Service to be
         * verified). It follows the structure defined on DicePolicy.cddl, located under
         * hardware/interfaces/security/authgraph/aidl/android/hardware/security/authgraph/ with the
         * caveat that it could be encrypted if the client does not have enough permissions to see
         * the device dice policy information.
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
         * If used we will derive a clear key and pass it back as an array of bytes on
         * <code>HwCryptoKeyMaterial::explicitKey</code>.
         */
        ClearKeyPolicy clearKey;

        /*
         * Policy for the newly derived opaque key. Defines how the key can be used and its type.
         */
        byte[] opaqueKey;
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
         * An arbitrary set of bytes incorporated into the key derivation. May have an
         * implementation-specific maximum length, but it is guaranteed to accept at least 32 bytes.
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
     * Derives a versioned key tied to the caller's current DICE policy. It will return this current
     * policy back to the caller along with the generated key.
     *
     * @param derivationKey:
     *     Key to be used to derive the new key using HKDF.
     *
     * @return:
     *     A DiceCurrentBoundKeyResult containint the versioned key tied the current client version
     *     on success.
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs.
     */
    DiceCurrentBoundKeyResult deriveCurrentDicePolicyBoundKey(
            in DiceBoundDerivationKey derivationKey);

    /*
     * Derive a versioned key by checking the provided DICE policy against the caller and then using
     * it as a context for deriving the returned key.
     *
     * @param derivationKey:
     *     Key to be used to derive the new key using HKDF.
     *
     * @param dicePolicyForKeyVersion:
     *     Policy used to derive keys tied to specific versions. Using this parameter the caller can
     *     tie a derived key to a minimum version of itself, so in the future only itself or a more
     *     recent version can derive the same key. This parameter is opaque to the caller and it
     *     could be encrypted in the case the client doesn't have permission to know the dice chain.
     *     When implementing this function, this parameter shall be one of the components fed to the
     *     KDF context and it needs to be checked against the caller DICE certificate before being
     *     used.
     *
     * @return:
     *      A DiceBoundKeyResult containing the versioned key tied to the provided DICE policy on
     *      success.
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs.
     */
    DiceBoundKeyResult deriveDicePolicyBoundKey(
            in DiceBoundDerivationKey derivationKey, in byte[] dicePolicyForKeyVersion);

    /*
     * Derive a new key based on the given key, policy and context.
     *
     * @param parameters:
     *      Parameters used for the key derivation. See <code>DerivedKeyParameters</code> on this
     *      file for more information.
     *
     * @return:
     *      A HwCryptoKeyMaterial containing the derived key on success.
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs.
     */
    DerivedKey deriveKey(in DerivedKeyParameters parameters);

    /*
     * Returns an interface used to work on opaque keys. This interface can also be used to operate
     * on any opaque key generated by hwkeyDeriveVersioned, even if this key has been generated
     * after retrieving a IHwCryptoOperations binder object, as long as the parent
     * IHwCryptoDeviceKeyAccess is not dropped between retrieving the IHwCryptoOperations binder
     * object and deriving the key. IHwCryptoOperations can also be used to create opaque keys that
     * are not bound to the device.
     *
     * @return:
     *      IHwCryptoOperations on success
     */
    IHwCryptoOperations getHwCryptoOperations();

    /*
     * Imports a SW clear key into the secure environment.
     *
     * @param keyMaterial:
     *     key to be imported.
     *
     * @param newKeyPolicy:
     *      Policy of the new key. Defines how the newly created key can be used. Because any clear
     *      key imported into the system is considered to have a <code>KeyLifetime::PORTABLE</code>
     *      lifetime, a call to this function will return an error if
     *      <code>newKeyPolicy.newKeyPolicy</code> is not set to portable.
     *
     * @return:
     *      IOpaqueKey on success.
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs.
     */
    IOpaqueKey importClearKey(in ExplicitKeyMaterial keyMaterial, in KeyPolicy newKeyPolicy);

    /*
     * Returns the client current DICE policy. This policy is encrypted and considered opaque from
     * the client perspective. This policy is the same used to create DICE bound keys and will also
     * be used to seal secrets that can only be retrieved by the DICE policy owner. The first use of
     * this seal operation will be <code>IOpaqueKey::getShareableToken</code> and will call this
     * <code>IHwCryptoKey::keyTokenImport</code>. To start this process, the intended key receiver
     * function and then pass the generated DICE policy to the owner of the key that the receiver
     * wants to import. The key owner will then call <code>IOpaqueKey::getShareableToken</code>
     * passing the receiver DICE policy to insure that only that receiver can import the key.
     *
     * @return:
     *      byte[] on success, which is the caller encrypted DICE policy.
     */
    byte[] getCurrentDicePolicy();

    /*
     * Imports a key from a different client service instance. Because IOpaqueKey are binder objects
     * that cannot be directly shared between binder rpc clients, this method provide a way to send
     * a key to another client. Keys to be imported by the receiver are represented by a token
     * created using <code>IOpaqueKey::getShareableToken</code>. The flow to create this token is
     * described in <code>IHwCryptoKey::getCurrentDicePolicy</code>.
     *
     * @param requested_key:
     *      Handle to the key to be imported to the caller service.
     *
     * @param sealingDicePolicy:
     *      DICE policy used to seal the exported key.
     *
     * @return:
     *      An IOpaqueKey that can be directly be used on the local HWCrypto service on success.
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs.
     */
    IOpaqueKey keyTokenImport(in OpaqueKeyToken requestedKey, in byte[] sealingDicePolicy);

    /*
     * Gets the keyslot key material referenced by slotId. This interface is used to access device
     * specific keys with known types and uses. Because the returned key is opaque, it can only be
     * used through the different HwCrypto interfaces. Because the keys live in a global namespace
     * the identity of the caller needs to be checked to verify that it has permission to access the
     * requested key.
     *
     * @param slotId:
     *      Identifier for the requested keyslot
     *
     * @return:
     *      An IOpaqueKey corresponding to the requested key slot on success.
     *
     * @throws:
     *      ServiceSpecificException <code>UNAUTHORIZED</code> if the caller cannot access the
     *      requested key, another specific error based on <code>HalErrorCode</code> otherwise.
     */
    IOpaqueKey getKeyslotData(KeySlot slotId);
}
