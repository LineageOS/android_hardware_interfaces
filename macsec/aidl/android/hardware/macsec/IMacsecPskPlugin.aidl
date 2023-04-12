/*
 * Copyright (C) 2023 The Android Open Source Project
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
package android.hardware.macsec;

/**
 * MACSEC (IEEE 802.1AE) pre-shared key plugin for wpa_supplicant
 *
 * The goal of this service is to provide function for using the MACSEC CAK
 *
 */
@VintfStability
interface IMacsecPskPlugin {
    /**
     * For xTS test only inject a key to verify implementation correctness, not called in production
     *
     * @param keyId is key id to add
     * @param Connectivity Association Keys (CAK) to set
     * @param Connectivity Association Key Name (CKN) to set
     * @throws EX_ILLEGAL_ARGUMENT If CAK size is not 16 or 32 or keyID size not equals to CAK size
     */
    void addTestKey(in byte[] keyId, in byte[] CAK, in byte[] CKN);

    /**
     * Use ICV key do AES CMAC
     * same as ieee802_1x_icv_aes_cmac in wpa_supplicant
     *
     * @param keyId is key id to be used for AES CMAC
     * @param data, a data pointer to the buffer for calculate the ICV
     *
     * @return Integrity check value (ICV).
     * @throws EX_ILLEGAL_ARGUMENT If keyId does not exist
     */
    byte[] calcIcv(in byte[] keyId, in byte[] data);

    /**
     * KDF with CAK key to generate Secure Association Key (SAK)
     * same as ieee802_1x_sak_aes_cmac in wpa_supplicant
     *
     * @param keyId is key id to be used for KDF
     * @param data is key seed (random number)
     * @param sakLength generated SAK length (16 or 32)
     *
     * @return Secure Association Key (SAK).
     * @throws EX_ILLEGAL_ARGUMENT In the following cases:
     *                             - If keyId does not exist
     *                             - sakLength != 16 or 32
     *                             - data length < sakLength
     */
    byte[] generateSak(in byte[] keyId, in byte[] data, in int sakLength);

    /**
     * Encrypt using KEK key, this is same as aes_wrap with kek.key in wpa_supplicant
     * which used to wrap a SAK key
     *
     * @param keyId is key id to be used for encryption
     * @param sak is the SAK key (16 or 32 bytes) to be wrapped.
     *
     * @return wrapped data using Key Encrypting Key (KEK).
     * @throws EX_ILLEGAL_ARGUMENT In the following cases:
     *                             - If keyId does not exist
     *                             - sak size eqauls to 0 or not multiples of 8
     */
    byte[] wrapSak(in byte[] keyId, in byte[] sak);

    /**
     * Decrypt using KEK key, this is same as aes_unwrap with kek.key in wpa_supplicant
     * which used to unwrap a SAK key
     *
     * @param keyId is key id to be used for decryption
     * @param sak is wrapped SAK key.
     *
     * @return unwrapped data using KEK key.
     * @throws EX_ILLEGAL_ARGUMENT In the following cases:
     *                             - If keyId does not exist
     *                             - sak size <= 8 or not multiples of 8
     */
    byte[] unwrapSak(in byte[] keyId, in byte[] sak);
}
