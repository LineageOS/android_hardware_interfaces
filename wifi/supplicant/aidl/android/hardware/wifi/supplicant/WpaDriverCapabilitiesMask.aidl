/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.wifi.supplicant;

/**
 * WPA Driver capability.
 */
@VintfStability
@Backing(type="int")
enum WpaDriverCapabilitiesMask {
    /**
     * Multi Band Operation.
     */
    MBO = 1 << 0,
    /**
     * Optimized Connectivity Experience.
     */
    OCE = 1 << 1,
    /**
     * WPA3 SAE Public-Key.
     */
    SAE_PK = 1 << 2,
    /**
     * Wi-Fi Display R2
     */
    WFD_R2 = 1 << 3,
    /**
     * Trust On First Use
     */
    TRUST_ON_FIRST_USE = 1 << 4,
    /**
     * TLS minimum version
     */
    SET_TLS_MINIMUM_VERSION = 1 << 5,
    /**
     * TLS V1.3
     */
    TLS_V1_3 = 1 << 6,
}
