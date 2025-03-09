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
import android.hardware.security.see.hwcrypto.types.OpaqueKeyToken;
import android.hardware.security.see.hwcrypto.types.OperationType;
import android.hardware.security.see.hwcrypto.types.ProtectionId;

@VintfStability
interface IOpaqueKey {
    /*
     * Exports this key as a wrapped (encrypted) blob.
     *
     * @param wrapping_key:
     *     wrapping key. It needs to be an opaque key and its policy needs to indicate that it can
     *     be used for key wrapping.
     *
     * @return:
     *      Wrapped key blob as a byte array on success. Format of the blob is opaque to the service
     *      but has to match the command accepted by
     *      <code>IHwCryptoKeyGeneration::importWrappedKey</code>
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs.
     */
    byte[] exportWrappedKey(in IOpaqueKey wrappingKey);

    /*
     * Returns the key policy.
     *
     * @return:
     *      A <code>KeyPolicy</code> on success
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs.
     */
    KeyPolicy getKeyPolicy();

    /*
     * Returns the public key portion of this OpaqueKey. This operation is only valid for asymmetric
     * keys.
     *
     * @return:
     *      public key as a byte array on success. Format used for the returned public key is COSE.
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs.
     */
    byte[] getPublicKey();

    /*
     * Returns a token that can shared with another HWCrypto client.
     *
     * @param sealingDicePolicy:
     *      Token to be used to protect the returned OpaqueKeyToken. It will be used so only
     *      the owner of the sealingDicePolicy can import the key.
     *
     * @return:
     *      <code>OpaqueKeyMaterial</code> token on success.
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs.
     */
    OpaqueKeyToken getShareableToken(in byte[] sealingDicePolicy);

    /*
     * Sets the protectionID associated with the buffers where the operation will be performed. A
     * protection ID serves as a limitation on the key so it can only operate on buffers with a
     * matching protection ID. The client calling this functions needs to have the necessary
     * permissions to read and/or write to this buffer. Setting this parameter means that if the key
     * is shared with a different client, the client receiving the key will be limited in which
     * buffers can be used to read/write data for this operation.
     *
     * @param protectionId:
     *      ID of the given use case to provide protection for. The method of protecting the buffer
     *      will be platform dependent.
     *
     * @param allowedOperations:
     *      array of allowed operations. Allowed operations are either READ or WRITE.
     *
     * @throws:
     *      ServiceSpecificException based on <code>HalErrorCode</code> if any error occurs.
     */
    void setProtectionId(in ProtectionId protectionId, in OperationType[] allowedOperations);
}
