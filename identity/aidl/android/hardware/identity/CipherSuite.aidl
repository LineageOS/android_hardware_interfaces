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
     * and the prover is using the following primitives
     *
     *  - ECKA-DH (Elliptic Curve Key Agreement Algorithm - Diffie-Hellman, see BSI TR-03111)
     *  - HKDF-SHA-256 (see RFC 5869)
     *  - AES-256-GCM (see NIST SP 800-38D)
     *  - HMAC-SHA-256 (see RFC 2104)
     *
     * The exact way these primitives are combined to derive the session key is specified in
     * section 9.2.1.4 of ISO/IEC 18013-5 (see description of cipher suite '1').
     *
     * At present this is the only supported cipher suite and it is mandatory for all
     * implementations to support it.
     */
    CIPHERSUITE_ECDHE_HKDF_ECDSA_WITH_AES_256_GCM_SHA256 = 1,
}
