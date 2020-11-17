/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.keymint;

/**
 * Key derivation functions, mostly used in ECIES.
 */
@VintfStability
@Backing(type="int")
enum KeyDerivationFunction {
    /** Do not apply a key derivation function; use the raw agreed key */
    NONE = 0,
    /** HKDF defined in RFC 5869 with SHA256 */
    RFC5869_SHA256 = 1,
    /** KDF1 defined in ISO 18033-2 with SHA1 */
    ISO18033_2_KDF1_SHA1 = 2,
    /** KDF1 defined in ISO 18033-2 with SHA256 */
    ISO18033_2_KDF1_SHA256 = 3,
    /** KDF2 defined in ISO 18033-2 with SHA1 */
    ISO18033_2_KDF2_SHA1 = 4,
    /** KDF2 defined in ISO 18033-2 with SHA256 */
    ISO18033_2_KDF2_SHA256 = 5,
}
