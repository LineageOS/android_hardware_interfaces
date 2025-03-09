/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.wifi;

/**
 * Pre-Association Security Negotiation (PASN) configuration.
 */
@VintfStability
parcelable PasnConfig {
    /**
     * Base Authentication and Key Management (AKM) protocol used for PASN. Represented as
     * at bitmap of |Akm|.
     */
    long baseAkm;
    /**
     * Pairwise cipher suite used for the PTKSA (Pairwise Transient Key Security Association).
     * Represented as a bitmap of |CipherSuite|.
     */
    long cipherSuite;
    /**
     * Passphrase for the base AKM. This can be null based on the AKM type.
     */
    @nullable byte[] passphrase;
    /**
     * PMKID corresponding to the cached PMK from the base AKM. PMKID can be null if no cached PMK
     * is present.
     */
    @nullable byte[] pmkid;
}
