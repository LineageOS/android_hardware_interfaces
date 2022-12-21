/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.cas;

/**
 * The Scrambling Mode.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum ScramblingMode {
    RESERVED = 0,

    /**
     * DVB (Digital Video Broadcasting) CSA1 (Common Scrambling Algorithm 1) is
     * the default mode and shall be used when the scrambling descriptor
     * is not present in the program map section. DVB scrambling mode is
     * specified in ETSI EN 300 468 specification.
     */
    DVB_CSA1,

    DVB_CSA2,

    /**
     * DVB-CSA3 in standard mode.
     */
    DVB_CSA3_STANDARD,

    /**
     * DVB-CSA3 in minimally enhanced mode.
     */
    DVB_CSA3_MINIMAL,

    /**
     * DVB-CSA3 in fully enhanced mode.
     */
    DVB_CSA3_ENHANCE,

    /**
     * DVB-CISSA version 1.
     */
    DVB_CISSA_V1,

    /**
     * ATIS-0800006 IIF Default Scrambling Algorithm (IDSA).
     */
    DVB_IDSA,

    /**
     * a symmetric key algorithm.
     */
    MULTI2,

    /**
     * Advanced Encryption System (AES) 128-bit Encryption mode.
     */
    AES128,

    /**
     * Advanced Encryption System (AES) Electronic Code Book (ECB) mode.
     */
    AES_ECB,

    /**
     * Advanced Encryption System (AES) Society of Cable Telecommunications
     * Engineers (SCTE) 52 mode.
     */
    AES_SCTE52,

    /**
     * Triple Data Encryption Algorithm (TDES) Electronic Code Book (ECB) mode.
     */
    TDES_ECB,

    /**
     * Triple Data Encryption Algorithm (TDES) Society of Cable Telecommunications
     * Engineers (SCTE) 52 mode.
     */
    TDES_SCTE52,

    /**
     * Advanced Encryption System (AES) Cipher Block Chaining (CBC) mode.
     */
    AES_CBC,
}
