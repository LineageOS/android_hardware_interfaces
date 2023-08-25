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

package android.hardware.wifi;

/**
 * Cipher suite flags. Wi-Fi Aware Specification 4.0 section 9.5.21.1.
 */
@VintfStability
@Backing(type="int")
enum NanCipherSuiteType {
    NONE = 0,
    /**
     *  NCS-SK-128
     */
    SHARED_KEY_128_MASK = 1 << 0,
    /**
     *  NCS-SK-256
     */
    SHARED_KEY_256_MASK = 1 << 1,
    /**
     *  NCS-PK-2WDH-128
     */
    PUBLIC_KEY_2WDH_128_MASK = 1 << 2,
    /**
     *  NCS-PK-2WDH-256
     */
    PUBLIC_KEY_2WDH_256_MASK = 1 << 3,
    /**
     * bit 4 and bit 5 are reserved for NCS-GTK-CCMP-128 and NCS-GTK-CCMP-256. Which are not used
     * from framework
     */
    /**
     *  NCS-PK-PASN-128
     */
    PUBLIC_KEY_PASN_128_MASK = 1 << 6,
    /**
     *  NCS-PK-PASN-256
     */
    PUBLIC_KEY_PASN_256_MASK = 1 << 7,
}
