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
 * Enum describing the allowed operations that can be performed with the given key.
 */
@VintfStability
@Backing(type="int")
enum KeyUse {
    /* Key can be used to encrypt */
    ENCRYPT = 1,

    /* Key can be used to decrypt */
    DECRYPT = 2,

    /* Key can be used to encrypt or decrypt */
    ENCRYPT_DECRYPT = ENCRYPT | DECRYPT,

    /* Key can be used to sign */
    SIGN = 4,

    /* Key can be used to derive other keys */
    DERIVE = 8,

    /* Key can be used to wrap other keys */
    WRAP = 16,
}
