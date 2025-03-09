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
package android.hardware.security.see.hwcrypto.types;

/*
 * Enum describing all supported key types. Key types are strongly bound to the algorithm to
 * prevent reusing the same key on different algorithms (e.g. using the same key for 2 different AES
 * 128 Cipher modes).
 */
@VintfStability
enum KeyType {
    /*
     * AES with key size 128 bits using CBC mode of operation and no padding.
     */
    AES_128_CBC_NO_PADDING,

    /*
     * AES with key size 128 bits using CBC mode of operation and PKCS7 padding.
     */
    AES_128_CBC_PKCS7_PADDING,

    /*
     * AES with key size 128 bits using counter mode.
     */
    AES_128_CTR,

    /*
     * AES with key size 128 bits using GCM mode for authenticated encryption.
     */
    AES_128_GCM,

    /*
     * AES with key size 128 bits for CMAC calculation.
     */
    AES_128_CMAC,

    /*
     * AES with key size 256 bits using CBC mode of operation and no padding.
     */
    AES_256_CBC_NO_PADDING,

    /*
     * AES with key size 256 bits using CBC mode of operation and PKCS7 padding.
     */
    AES_256_CBC_PKCS7_PADDING,

    /*
     * AES with key size 128 bits using counter mode.
     */
    AES_256_CTR,

    /*
     * AES with key size 128 bits using GCM mode for authenticated encryption.
     */
    AES_256_GCM,

    /*
     * AES with key size 128 bits for CMAC calculation.
     */
    AES_256_CMAC,

    /*
     * Key of length of 32 bytes for HMAC operations using SHA256.
     */
    HMAC_SHA256,

    /*
     * Key of length of 64 bytes for HMAC operations using SHA512.
     */
    HMAC_SHA512,

    /*
     * RSA of key size of 2048 bits for signing using PSS.
     */
    RSA2048_PSS_SHA256,

    /*
     * RSA of key size of 2048 bits for signing with padding PKCS 1.5 and SHA256 as the digest
     * algorithm.
     */
    RSA2048_PKCS1_5_SHA256,

    /*
     * ECC key for signing using curve P-256 and no padding.
     */
    ECC_NIST_P256_SIGN_NO_PADDING,

    /*
     * ECC key for signing using curve P-256 and SHA256 as hashing algorithm.
     */
    ECC_NIST_P256_SIGN_SHA256,

    /*
     * ECC key for signing using curve P-521 and no padding.
     */
    ECC_NIST_P521_SIGN_NO_PADDING,

    /*
     * ECC key for signing using curve P-512 and SHA512 as hashing algorithm.
     */
    ECC_NIST_P521_SIGN_SHA512,

    /*
     * ECC key for signing using EdDSA.
     */
    ECC_ED25519_SIGN,
}
