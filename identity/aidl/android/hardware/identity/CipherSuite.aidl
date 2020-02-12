/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.identity;

/**
 * Cipher suites that can be used for communication between holder and reader devices.
 */
@VintfStability
@Backing(type="int")
enum CipherSuite {
    /**
     * Specifies that the cipher suite that will be used to secure communications between the reader
     * is:
     *
     * - ECDHE with HKDF-SHA-256 for key agreement.
     * - AES-256 with GCM block mode for authenticated encryption (nonces are incremented by
     *   one for every message).
     * - ECDSA with SHA-256 for signing (used for signing session transcripts to defeat
     *   man-in-the-middle attacks), signing keys are not ephemeral.
     *
     * At present this is the only supported cipher suite and it is mandatory for all
     * implementations to support it.
     */
    CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256 = 1,
}
