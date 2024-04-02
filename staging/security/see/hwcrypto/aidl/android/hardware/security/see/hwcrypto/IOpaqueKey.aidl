/*
 * Copyright 2024 The Android Open Source Project
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

import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.types.OperationType;

interface IOpaqueKey {
    /*
     * exportWrappedKey() - Exports this key as a wrapped (encrypted) blob.
     *
     * @wrapping_key:
     *     wrapping key. It needs to be an opaque key and its policy needs to indicate that it can
     *     be used for key wrapping.
     *
     * Return:
     *      Wrapped key blob as a byte array on success. Format of the blob is opaque to the service
     *      but has to match the command accepted by
     *      <code>IHwCryptoKeyGeneration::importWrappedKey</code>, service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     */
    byte[] exportWrappedKey(in IOpaqueKey wrappingKey);

    /*
     * getKeyPolicy() - Returns the key policy.
     *
     * Return:
     *      A <code>KeyPolicy</code> on success, service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     */
    KeyPolicy getKeyPolicy();

    /*
     * getPublicKey() - Returns the public key portion of this OpaqueKey. This operation is only
     *                  valid for asymmetric keys
     *
     * Return:
     *      public key as a byte array on success, service specific error based on
     *      <code>HalErrorCode</code> otherwise. Format used for the returned public key is COSE.
     */
    byte[] getPublicKey();
}
