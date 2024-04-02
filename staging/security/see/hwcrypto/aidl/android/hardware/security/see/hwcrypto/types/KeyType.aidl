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
enum KeyType {
    AES_128_CBC_NO_PADDING,
    AES_128_CBC_PKCS7_PADDING,
    AES_128_CTR,
    AES_128_GCM,
    AES_128_CMAC,
    AES_256_CBC_NO_PADDING,
    AES_256_CBC_PKCS7_PADDING,
    AES_256_CTR,
    AES_256_GCM,
    AES_256_CMAC,
    HMAC_SHA256,
    HMAC_SHA512,
    RSA2048_PSS_SHA256,
    RSA2048_PKCS1_5_SHA256,
    ECC_NIST_P256_SIGN_NO_PADDING,
    ECC_NIST_P256_SIGN_SHA256,
    ECC_NIST_P521_SIGN_NO_PADDING,
    ECC_NIST_P521_SIGN_SHA512,
    ECC_ED25519_SIGN,
}
